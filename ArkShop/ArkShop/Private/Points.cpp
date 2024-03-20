#include <Points.h>
#include <DBHelper.h>
#include "ArkShop.h"
#include "ShopLog.h"
#include "ArkShopUIHelper.h"

namespace ArkShop::Points
{
	// Public functions

	bool AddPoints(int amount, const FString& eos_id)
	{
		if (amount <= 0)
		{
			if (config.value("General", nlohmann::json::object()).value("TimedPointsReward", nlohmann::json::object()).value("AlwaysSendNotifications", false))
			{
				AShooterPlayerController* player = AsaApi::GetApiUtils().FindPlayerFromEOSID(eos_id);
				if (player != nullptr)
					AsaApi::GetApiUtils().SendChatMessage(player, GetText("Sender"), *GetText("ReceivedNoPoints"));
			}

			return false;
		}

		const bool is_added = database->AddPoints(eos_id, amount);
		if (!is_added)
			return false;

		int points = GetPoints(eos_id);
		ArkShopUI::UpdatePoints(eos_id, points);

		AShooterPlayerController* player = AsaApi::GetApiUtils().FindPlayerFromEOSID(eos_id);
		if (player != nullptr)
			AsaApi::GetApiUtils().SendChatMessage(player, GetText("Sender"), *GetText("ReceivedPoints"), amount, GetPoints(eos_id));

		return true;
	}

	bool SpendPoints(int amount, const FString& eos_id)
	{
		if (amount <= 0)
			return false;

		const bool is_spend = database->SpendPoints(eos_id, amount);
		if (!is_spend)
			return false;

		int points = GetPoints(eos_id);
		ArkShopUI::UpdatePoints(eos_id, points);

		return true;
	}

	int GetPoints(const FString& eos_id)
	{
		return database->GetPoints(eos_id);
	}

	int GetTotalSpent(const FString& eos_id)
	{
		return database->GetTotalSpent(eos_id);
	}

	bool SetPoints(const FString& eos_id, int new_amount)
	{
		const bool is_spend = database->SetPoints(eos_id, new_amount);
		if (!is_spend)
			return false;

		int points = GetPoints(eos_id);
		ArkShopUI::UpdatePoints(eos_id, points);

		return true;
	}

	// Chat callbacks

	/**
	 * \brief Send points to the other player (using character name)
	 */
	void Trade(AShooterPlayerController* player_controller, FString* message, int, int)
	{
		const FString& sender_eos_id = AsaApi::IApiUtils::GetEOSIDFromController(player_controller);

		if (DBHelper::IsPlayerExists(sender_eos_id))
		{
			TArray<FString> parsed;
			FString in_param = ""; //CharacterName or EOSID
			int amount = 0;
			AShooterPlayerController* receiver_player = nullptr;
			FString receiver_eos_id = "";

			message->ParseIntoArray(parsed, L" ", true);
			if (parsed.IsValidIndex(2) == false)
			{
				AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *GetText("TradeSyntax"));
				return;
			}

			try
			{
				in_param = parsed[1];
				amount = std::stoi(*parsed[2]);
			}
			catch (const std::exception& exception)
			{
				Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
				return;
			}

			if (AsaApi::Tools::IsPluginLoaded("ArkShopUI") && !config["General"].value("UseOriginalTradeCommandWithUI", false))
			{
				receiver_player = AsaApi::GetApiUtils().FindPlayerFromEOSID(in_param);
			}
			else
			{
				TArray<AShooterPlayerController*> receiver_players = AsaApi::GetApiUtils().FindPlayerFromCharacterName(in_param, ESearchCase::IgnoreCase, false);
				if (receiver_players.Num() > 1)
				{
					AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *GetText("FoundMorePlayers"));
					return;
				}

				if (receiver_players.Num() < 1)
				{
					AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *GetText("NoPlayer"));
					return;
				}

				receiver_player = receiver_players[0];
			}

			if (receiver_player == nullptr)
			{
				AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *GetText("NoPlayer"));
				return;
			}
			else
				receiver_eos_id = AsaApi::IApiUtils::GetEOSIDFromController(receiver_player);

			if (GetPoints(sender_eos_id) < amount)
			{
				AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *GetText("NoPoints"));
				return;
			}

			if (receiver_eos_id == sender_eos_id)
			{
				AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *GetText("CantGivePoints"));
				return;
			}

			if (DBHelper::IsPlayerExists(receiver_eos_id)==false)
				database->TryAddNewPlayer(receiver_eos_id);

			bool pointsSubtracted = SpendPoints(amount, sender_eos_id);
			bool pointsAdded = AddPoints(amount, receiver_eos_id);

			if (pointsSubtracted && pointsAdded)
			{
				FString receiver_name = AsaApi::GetApiUtils().GetCharacterName(receiver_player);
				AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *GetText("SentPoints"), amount, *receiver_name);

				const FString sender_name = AsaApi::IApiUtils::GetCharacterName(player_controller);
				AsaApi::GetApiUtils().SendChatMessage(receiver_player, GetText("Sender"), *GetText("GotPoints"), amount, *sender_name);

				const std::wstring log = fmt::format(TEXT("[{}] {}({}) Traded points with: {}({}) Amount: {}"),
					*ArkShop::SetMapName(),
					*AsaApi::IApiUtils::GetSteamName(player_controller), sender_eos_id.ToString(),
					*AsaApi::IApiUtils::GetSteamName(receiver_player), receiver_eos_id.ToString(),
					amount);

				ShopLog::GetLog()->info(AsaApi::Tools::Utf8Encode(log));
				ArkShop::PostToDiscord(log);
			}
			else //if one of the operations failed, try to revert the other one
			{
				if (pointsSubtracted == true) //undo the transaction
				{
					AddPoints(amount, sender_eos_id);
					AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *GetText("RefundError"));
				}
				
				if (pointsAdded == true) //undo the transaction
				{
					SpendPoints(amount, receiver_eos_id);
					AsaApi::GetApiUtils().SendChatMessage(receiver_player, GetText("Sender"), *GetText("RefundError"));
				}
			}
		}
	}

	void PrintPoints(AShooterPlayerController* player_controller, FString* /*unused*/, int, int)
	{
		const FString& eos_id = AsaApi::IApiUtils::GetEOSIDFromController(player_controller);

		if (DBHelper::IsPlayerExists(eos_id))
		{
			int points = GetPoints(eos_id);

			if (AsaApi::Tools::IsPluginLoaded("ArkShopUI"))
			{
				ArkShopUI::UpdatePoints(eos_id, points);
				return;
			}

			AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *GetText("HavePoints"), points);
		}
	}

	// Callbacks

	bool AddPointsCbk(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(2))
		{
			FString eos_id;
			int amount;

			try
			{
				eos_id = *parsed[1];
				amount = std::stoi(*parsed[2]);
			}
			catch (const std::exception& exception)
			{
				Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
				return false;
			}

			return DBHelper::IsPlayerExists(eos_id) && AddPoints(amount, eos_id);
		}

		return false;
	}

	bool SetPointsCbk(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(2))
		{
			FString eos_id;
			int amount;

			try
			{
				eos_id = *parsed[1];
				amount = std::stoi(*parsed[2]);
			}
			catch (const std::exception& exception)
			{
				Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
				return false;
			}

			return DBHelper::IsPlayerExists(eos_id) && SetPoints(eos_id, amount);
		}

		return false;
	}

	bool ChangePointsAmountCbk(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(2))
		{
			FString eos_id;
			int amount;

			try
			{
				eos_id = *parsed[1];
				amount = std::stoi(*parsed[2]);
			}
			catch (const std::exception& exception)
			{
				Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
				return false;
			}

			if (DBHelper::IsPlayerExists(eos_id))
			{
				return amount >= 0
					? AddPoints(amount, eos_id)
					: SpendPoints(std::abs(amount), eos_id);
			}
		}

		return false;
	}

	int GetPlayerPointsCbk(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(1))
		{
			FString eos_id;

			try
			{
				eos_id = *parsed[1];
			}
			catch (const std::exception& exception)
			{
				Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
				return -1;
			}

			if (DBHelper::IsPlayerExists(eos_id))
			{
				return GetPoints(eos_id);
			}
		}

		return -1;
	}

	// Console commands

	void AddPointsCmd(APlayerController* player_controller, FString* cmd, bool /*unused*/)
	{
		auto* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		const bool result = AddPointsCbk(*cmd);
		if (result)
		{
			AsaApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully added points");
		}
		else
		{
			AsaApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Couldn't add points");
		}
	}

	void SetPointsCmd(APlayerController* player_controller, FString* cmd, bool /*unused*/)
	{
		auto* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		const bool result = SetPointsCbk(*cmd);
		if (result)
		{
			AsaApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully set points");
		}
		else
		{
			AsaApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Couldn't set points");
		}
	}

	void ChangePointsAmountCmd(APlayerController* player_controller, FString* cmd, bool /*unused*/)
	{
		auto* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		const bool result = ChangePointsAmountCbk(*cmd);
		if (result)
		{
			AsaApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully set points");
		}
		else
		{
			AsaApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Couldn't set points");
		}
	}

	/**
	 * \brief Reset points for all players
	 */
	void ResetPointsCmd(APlayerController* player_controller, FString* cmd, bool /*unused*/)
	{
		TArray<FString> parsed;
		cmd->ParseIntoArray(parsed, L" ", true);

		auto* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		if (parsed.IsValidIndex(1))
		{
			if (parsed[1].ToString() == "confirm")
			{
				database->DeleteAllPoints();

				AsaApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green,
					"Successfully reset points");
			}
		}
		else
		{
			AsaApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Yellow,
				"You are going to reset points for ALL players\nType 'ResetPoints confirm' in console if you want to continue");
		}
	}

	void GetPlayerPointsCmd(APlayerController* player_controller, FString* cmd, bool /*unused*/)
	{
		auto* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		const int points = GetPlayerPointsCbk(*cmd);
		if (points != -1)
		{
			AsaApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Player has {} points",
				points);
		}
		else
		{
			AsaApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Couldn't get points amount");
		}
	}

	// Rcon callbacks

	void AddPointsRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld* /*unused*/)
	{
		FString reply;

		const bool result = AddPointsCbk(rcon_packet->Body);
		if (result)
		{
			reply = "Successfully added points\n";
		}
		else
		{
			reply = "Couldn't add points\n";
		}

		rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
	}

	void SetPointsRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld* /*unused*/)
	{
		FString reply;

		const bool result = SetPointsCbk(rcon_packet->Body);
		if (result)
		{
			reply = "Successfully set points\n";
		}
		else
		{
			reply = "Couldn't set points\n";
		}

		rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
	}

	void ChangePointsAmountRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld* /*unused*/)
	{
		FString reply;

		const bool result = ChangePointsAmountCbk(rcon_packet->Body);
		if (result)
		{
			reply = "Successfully set points\n";
		}
		else
		{
			reply = "Couldn't set points\n";
		}

		rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
	}

	void GetPlayerPointsRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld* /*unused*/)
	{
		FString reply;

		const int points = GetPlayerPointsCbk(rcon_packet->Body);
		if (points != -1)
		{
			reply = FString::Format("Player has {} points\n", points);
		}
		else
		{
			reply = "Couldn't get points amount\n";
		}

		rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
	}

	void Init()
	{
		auto& commands = AsaApi::GetCommands();

		commands.AddChatCommand(GetText("PointsCmd"), &PrintPoints);
		commands.AddChatCommand(GetText("TradeCmd"), &Trade);

		commands.AddConsoleCommand("AddPoints", &AddPointsCmd);
		commands.AddConsoleCommand("SetPoints", &SetPointsCmd);
		commands.AddConsoleCommand("ChangePoints", &ChangePointsAmountCmd);
		commands.AddConsoleCommand("GetPlayerPoints", &GetPlayerPointsCmd);
		commands.AddConsoleCommand("ResetPoints", &ResetPointsCmd);

		commands.AddRconCommand("AddPoints", &AddPointsRcon);
		commands.AddRconCommand("SetPoints", &SetPointsRcon);
		commands.AddRconCommand("ChangePoints", &ChangePointsAmountRcon);
		commands.AddRconCommand("GetPlayerPoints", &GetPlayerPointsRcon);
	}

	void Unload()
	{
		auto& commands = AsaApi::GetCommands();

		commands.RemoveChatCommand(GetText("PointsCmd"));
		commands.RemoveChatCommand(GetText("TradeCmd"));

		commands.RemoveConsoleCommand("AddPoints");
		commands.RemoveConsoleCommand("SetPoints");
		commands.RemoveConsoleCommand("ChangePoints");
		commands.RemoveConsoleCommand("GetPlayerPoints");
		commands.RemoveConsoleCommand("ResetPoints");

		commands.RemoveRconCommand("AddPoints");
		commands.RemoveRconCommand("SetPoints");
		commands.RemoveRconCommand("ChangePoints");
		commands.RemoveRconCommand("GetPlayerPoints");
	}
} // namespace Points // namespace ArkShop
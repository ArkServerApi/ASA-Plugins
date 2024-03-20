#include <Kits.h>

#include <DBHelper.h>
#include <Permissions.h>
#include <Points.h>

#include "ArkShop.h"
#include "ShopLog.h"
#include "ArkShopUIHelper.h"

namespace ArkShop::Kits
{
	DECLARE_HOOK(AShooterCharacter_AuthPostSpawnInit, void, AShooterCharacter*);

	/**
	 * \brief Returns kits info of specific player
	 */
	nlohmann::basic_json<> GetPlayerKitsConfig(const FString& eos_id)
	{
		const std::string kits_config = database->GetPlayerKits(eos_id);

		nlohmann::json conf = nlohmann::json::object();

		try
		{
			if (kits_config.length() >= 2 && (kits_config.substr(0, 1) == "{" || kits_config.substr(kits_config.length() - 1, 1) == "}"))
				conf = nlohmann::json::parse(kits_config);
			else
				conf = nlohmann::json::parse("{}");
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Couldn't parse config: {}", __FILE__, __FUNCTION__, exception.what());
		}

		return conf;
	}

	/**
	 * \brief Saves kits info of specific player
	 */
	bool SaveConfig(const std::string& dump, const FString& eos_id)
	{
		return database->UpdatePlayerKits(eos_id, dump);
	}

	/**
	 * \brief Checks if kit exists in server config
	 */
	bool IsKitExists(const FString& kit_name)
	{
		auto kits_list = config["Kits"];

		std::string kit_name_str = kit_name.ToString();

		const auto kit_entry_iter = kits_list.find(kit_name_str);

		return kit_entry_iter != kits_list.end();
	}

	/**
	 * \brief Adds or reduces kits of the specific player
	 */
	bool ChangeKitAmount(const FString& kit_name, int amount, const FString& eos_id)
	{
		if (amount == 0)
		{
			// We got nothing to change
			return true;
		}

		std::string kit_name_str = kit_name.ToString();

		int new_amount;

		// Kits json config
		auto player_kit_json = GetPlayerKitsConfig(eos_id);

		auto kit_json_iter = player_kit_json.find(kit_name_str);
		if (kit_json_iter == player_kit_json.end()) // If kit doesn't exists in player's config
		{
			auto kits_list = config["Kits"];

			auto kit_entry_iter = kits_list.find(kit_name_str);
			if (kit_entry_iter == kits_list.end())
			{
				return false;
			}

			auto kit_entry = kit_entry_iter.value();

			const int default_amount = kit_entry.value("DefaultAmount", 0);

			new_amount = default_amount + amount;
		}
		else
		{
			auto kit_json_entry = kit_json_iter.value();

			const int current_amount = kit_json_entry.value("Amount", 0);

			new_amount = current_amount + amount;
		}

		player_kit_json[kit_name_str]["Amount"] = new_amount >= 0 ? new_amount : 0;

		bool returnValue = SaveConfig(player_kit_json.dump(), eos_id);

		if (returnValue && AsaApi::Tools::IsPluginLoaded("ArkShopUI"))
		{
			FString kitData(database->GetPlayerKits(eos_id));
			ArkShopUI::PlayerKits(eos_id, kitData);
		}

		return returnValue;
	}

	/**
	* \brief Checks if player has permissions to use this kit
	*/
	bool CanUseKit(AShooterPlayerController* player_controller, const FString& eos_id, const FString& kit_name)
	{
		if (player_controller == nullptr || AsaApi::IApiUtils::IsPlayerDead(player_controller))
		{
			return false;
		}

		auto kits_list = config["Kits"];

		std::string kit_name_str = kit_name.ToString();

		const auto kit_entry_iter = kits_list.find(kit_name_str);
		if (kit_entry_iter == kits_list.end())
		{
			return false;
		}

		auto kit_entry = kit_entry_iter.value();

		const int min_level = kit_entry.value("MinLevel", 1);
		const int max_level = kit_entry.value("MaxLevel", 999);

		auto* primal_character = static_cast<APrimalCharacter*>(player_controller->CharacterField().Get());
		UPrimalCharacterStatusComponent* char_component = primal_character->MyCharacterStatusComponentField();
		if (char_component == nullptr)
		{
			return false;
		}

		const int level = char_component->BaseCharacterLevelField() + char_component->ExtraCharacterLevelField();
		if (level < min_level || level > max_level)
		{
			return false;
		}

		const std::string permissions = kit_entry.value("Permissions", "");
		if (permissions.empty())
		{
			return true;
		}

		const FString fpermissions(permissions);

		TArray<FString> groups;
		fpermissions.ParseIntoArray(groups, L",", true);

		for (const auto& group : groups)
		{
			if (Permissions::IsPlayerInGroup(eos_id, group))
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * \brief Returns amount of kits player has
	 */
	int GetKitAmount(const FString& eos_id, const FString& kit_name)
	{
		std::string kit_name_str = kit_name.ToString();

		auto player_kit_json = GetPlayerKitsConfig(eos_id);

		auto kit_json_iter = player_kit_json.find(kit_name_str);
		if (kit_json_iter != player_kit_json.end())
		{
			auto kit_json_entry = kit_json_iter.value();

			return kit_json_entry.value("Amount", 0);
		}

		// Return default amount if player didn't use this kit yet

		auto kits_list = config["Kits"];

		const auto kit_entry_iter = kits_list.find(kit_name_str);
		if (kit_entry_iter != kits_list.end())
		{
			return kit_entry_iter.value().value("DefaultAmount", 0);
		}

		return 0;
	}

	/**
	 * \brief Redeem the kit from the given config entry
	 */
	void GiveKitFromJson(AShooterPlayerController* player_controller, const nlohmann::basic_json<>& kit_entry)
	{
		// Give items
		auto items_map = kit_entry.value("Items", nlohmann::json::array());
		for (const auto& item : items_map)
		{
			const float quality = item.value("Quality", 0);
			const bool force_blueprint = item.value("ForceBlueprint", false);
			const int amount = item.value("Amount", 1);
			std::string blueprint = item.value("Blueprint", "");
			int armor = item.value("Armor", 0);
			int durability = item.value("Durability", 0);
			int damage = item.value("Damage", 0);

			FString fblueprint(blueprint.c_str());

			TArray<UPrimalItem*> out_items;
			player_controller->GiveItem(&out_items, &fblueprint, amount, quality, force_blueprint, false, 0);
			ApplyItemStats(out_items, armor, durability, damage);
		}

		// Give dinos
		auto dinos_map = kit_entry.value("Dinos", nlohmann::json::array());
		for (const auto& dino : dinos_map)
		{
			const int level = dino.value("Level", 1);
			const bool neutered = dino.value("Neutered", false);
			std::string gender = dino.value("Gender", "random");
			std::string saddleblueprint = dino.value("SaddleBlueprint", "");
			std::string blueprint = dino.value("Blueprint", "");
			bool preventCryo = dino.value("PreventCryo", false);
			const int stryderhead = dino.value("StryderHead", -1);
			const int stryderchest = dino.value("StryderChest", -1);
			nlohmann::json resourceoverrides = dino.value("GachaResources", nlohmann::json());

			bool success = ArkShop::GiveDino(player_controller, level, neutered, gender, blueprint, saddleblueprint, preventCryo, stryderhead, stryderchest, resourceoverrides);
		}

		// Give commands
		FString eos_id = AsaApi::GetApiUtils().GetEOSIDFromController(player_controller);
		auto commands_map = kit_entry.value("Commands", nlohmann::json::array());
		for (const auto& command_entry : commands_map)
		{
			const std::string command = command_entry.value("Command", "");

			const bool exec_as_admin = command_entry.value("ExecuteAsAdmin", false);

			FString fcommand = fmt::format(
				command, 
				fmt::arg("eosid", eos_id.ToString()),
				fmt::arg("eos_id", eos_id.ToString()),
				fmt::arg("playerid", AsaApi::GetApiUtils().GetPlayerID(player_controller)),
				fmt::arg("tribeid", AsaApi::GetApiUtils().GetTribeID(player_controller))
			).c_str();

			const bool was_admin = player_controller->bIsAdmin()();

			if (!was_admin && exec_as_admin)
				player_controller->bIsAdmin() = true;

			FString result;
			player_controller->ConsoleCommand(&result, &fcommand, false);

			if (!was_admin && exec_as_admin)
				player_controller->bIsAdmin() = false;
		}
	}

	/**
	 * \brief Redeem the kit for the specific player
	 */
	void RedeemKit(AShooterPlayerController* player_controller, const FString& kit_name, bool should_log, bool from_spawn)
	{
		if (AsaApi::IApiUtils::IsPlayerDead(player_controller))
		{
			return;
		}

		const FString& eos_id = AsaApi::IApiUtils::GetEOSIDFromController(player_controller);

		if (DBHelper::IsPlayerExists(eos_id))
		{
			if (!CanUseKit(player_controller, eos_id, kit_name))
			{
				AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
					*GetText("NoPermissionsKit"));
				return;
			}

			if (player_controller->GetPlayerInventoryComponent() && player_controller->GetPlayerInventoryComponent()->IsAtMaxInventoryItems())
			{
				AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
					*GetText("InventoryIsFull"));
				return;
			}

			std::string kit_name_str = kit_name.ToString();

			auto kits_list = config["Kits"];

			auto kit_entry_iter = kits_list.find(kit_name_str);
			if (kit_entry_iter == kits_list.end())
			{
				AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
					*GetText("WrongId"));
				return;
			}

			const auto kit_entry = kit_entry_iter.value();

			if (!from_spawn && kit_entry.value("OnlyFromSpawn", false))
			{
				AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
					*GetText("OnlyOnSpawnKit"));
				return;
			}

			if (const int kit_amount = GetKitAmount(eos_id, kit_name);
				kit_amount > 0 && ChangeKitAmount(kit_name, -1, eos_id))
			{
				GiveKitFromJson(player_controller, kit_entry);

				AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
					*GetText("KitsLeft"), kit_amount - 1, *kit_name);

				// Log
				if (should_log)
				{
					const std::wstring log = fmt::format(TEXT("[{}] {}({}) Used kit '{}'"),
						*ArkShop::SetMapName(),
						*AsaApi::IApiUtils::GetSteamName(player_controller), eos_id.ToString(),
						*kit_name);

					ShopLog::GetLog()->info(AsaApi::Tools::Utf8Encode(log));
					ArkShop::PostToDiscord(log);
				}
			}
			else if (should_log)
			{
				AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
					*GetText("NoKitsLeft"), *kit_name);
			}
		}
	}

	/**
	 * \brief Lists all available kits using notification
	 */
	void ListKits(AShooterPlayerController* player_controller)
	{
		const float display_time = config["General"].value("ShopDisplayTime", 15.0f);
		const float text_size = config["General"].value("ShopTextSize", 1.3f);

		const FString& eos_id = AsaApi::IApiUtils::GetEOSIDFromController(player_controller);

		FString kits_str = "";

		auto kits_map = config["Kits"];
		for (auto iter = kits_map.begin(); iter != kits_map.end(); ++iter)
		{
			const std::string kit_name_str = iter.key();
			const FString kit_name(kit_name_str.c_str());

			auto iter_value = iter.value();

			const int price = iter_value.value("Price", -1);

			if (const int amount = GetKitAmount(eos_id, kit_name);
				(amount > 0 || price != -1) && CanUseKit(player_controller, eos_id, kit_name))
			{
				const std::wstring description = AsaApi::Tools::Utf8Decode(
					iter_value.value("Description", "No description"));

				std::wstring price_str = price != -1 ? fmt::format(*GetText("KitsListPrice"), price) : L"";

				kits_str += FString::Format(*GetText("KitsListFormat"), *kit_name, description, amount, price_str);
			}
		}

		if (kits_str.IsEmpty())
		{
			kits_str = GetText("NoKits");
		}

		const FString kits_list_str = FString::Format(TEXT("{}\n{}\n{}"), *GetText("AvailableKits"), *kits_str,
			*GetText("KitUsage"));

		AsaApi::GetApiUtils().SendNotification(player_controller, FColorList::Green, text_size, display_time, nullptr,
			*kits_list_str);
	}

	void InitKitData(const FString& eos_id)
	{
		//Kits json config
		auto player_kit_json = GetPlayerKitsConfig(eos_id);

		auto kits_map = config["Kits"];
		for (auto iter = kits_map.begin(); iter != kits_map.end(); ++iter)
		{
			const std::string kit_name_str = iter.key();
			int new_amount;

			auto kit_json_iter = player_kit_json.find(kit_name_str);
			if (kit_json_iter == player_kit_json.end()) // If kit doesn't exists in player's config
			{
				auto kits_list = config["Kits"];

				auto kit_entry_iter = kits_list.find(kit_name_str);
				if (kit_entry_iter == kits_list.end())
				{
					continue;
				}

				auto kit_entry = kit_entry_iter.value();

				const int default_amount = kit_entry.value("DefaultAmount", 0);

				new_amount = default_amount;
			}
			else
			{
				auto kit_json_entry = kit_json_iter.value();

				const int current_amount = kit_json_entry.value("Amount", 0);

				new_amount = current_amount;
			}

			player_kit_json[kit_name_str]["Amount"] = new_amount >= 0 ? new_amount : 0;
		}

		bool returnValue = SaveConfig(player_kit_json.dump(), eos_id);

		if (returnValue && AsaApi::Tools::IsPluginLoaded("ArkShopUI"))
		{
			FString kitData(database->GetPlayerKits(eos_id));
			ArkShopUI::PlayerKits(eos_id, kitData);
		}
	}

	// Chat callbacks

	void Kit(AShooterPlayerController* player_controller, FString* message, int, int)
	{
		if (!IsStoreEnabled(player_controller))
			return;

		if (ShouldPreventStoreUse(player_controller))
			return;

		TArray<FString> parsed;
		message->ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(1))
		{
			RedeemKit(player_controller, parsed[1], true, false);
		}
		else
		{
			if (AsaApi::Tools::IsPluginLoaded("ArkShopUI"))
			{
				const FString& eos_id = AsaApi::GetApiUtils().GetEOSIDFromController(player_controller);
				if (!eos_id.IsEmpty())
				{
					FString kitData(database->GetPlayerKits(eos_id));
					ArkShopUI::PlayerKits(eos_id, kitData);
				}
				return;
			}
			else
				ListKits(player_controller);
		}
	}

	void BuyKit(AShooterPlayerController* player_controller, FString* message, int, int)
	{
		if (AsaApi::IApiUtils::IsPlayerDead(player_controller))
			return;

		TArray<FString> parsed;
		message->ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(2))
		{
			const FString& eos_id = AsaApi::IApiUtils::GetEOSIDFromController(player_controller);

			if (DBHelper::IsPlayerExists(eos_id))
			{
				FString kit_name = parsed[1];
				std::string kit_name_str = kit_name.ToString();

				if (!CanUseKit(player_controller, eos_id, kit_name))
				{
					AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
						*GetText("CantBuyKit"));
					return;
				}

				int amount;

				try
				{
					amount = std::stoi(*parsed[2]);
				}
				catch (const std::exception& exception)
				{
					Log::GetLog()->warn("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
					return;
				}

				if (amount <= 0)
				{
					return;
				}

				auto kits_list = config["Kits"];

				auto kit_entry_iter = kits_list.find(kit_name_str);
				if (kit_entry_iter == kits_list.end())
				{
					AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
						*GetText("WrongId"));
					return;
				}

				auto kit_entry = kit_entry_iter.value();

				const int price = kit_entry.value("Price", 0);
				if (price == 0)
				{
					AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
						*GetText("CantBuyKit"));
					return;
				}

				const int final_price = price * amount;
				if (final_price <= 0)
					return;

				if (Points::GetPoints(eos_id) >= final_price && Points::SpendPoints(final_price, eos_id))
				{
					ChangeKitAmount(kit_name, amount, eos_id);

					AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
						*GetText("BoughtKit"), *kit_name);

					// Log
					const std::wstring log = fmt::format(TEXT("[{}] {}({}) Bought kit: '{}' Amount: {} Total Spent Points: {}"),
						*ArkShop::SetMapName(),
						*AsaApi::IApiUtils::GetSteamName(player_controller), eos_id.ToString(),
						*kit_name,
						amount,
						final_price);

					ShopLog::GetLog()->info(AsaApi::Tools::Utf8Encode(log));
					ArkShop::PostToDiscord(log);
				}
				else
				{
					AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
						*GetText("NoPoints"));
				}
			}
		}
		else
		{
			AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
				*GetText("BuyKitUsage"));
		}
	}

	bool ChangeKitAmountCbk(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(3))
		{
			const FString kit_name = parsed[2];

			FString eos_id;
			int amount;

			try
			{
				eos_id = *parsed[1];
				amount = std::stoi(*parsed[3]);
			}
			catch (const std::exception& exception)
			{
				Log::GetLog()->warn("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
				return false;
			}

			if (DBHelper::IsPlayerExists(eos_id))
			{
				return ChangeKitAmount(kit_name, amount, eos_id);
			}
		}

		return false;
	}

	// Console callbacks

	void ChangeKitAmountCmd(APlayerController* controller, FString* cmd, bool /*unused*/)
	{
		const auto shooter_controller = static_cast<AShooterPlayerController*>(controller);

		const bool result = ChangeKitAmountCbk(*cmd);
		if (result)
		{
			AsaApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green,
				"Successfully changed kit amount");
		}
		else
		{
			AsaApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Couldn't change kit amount");
		}
	}

	void ResetKitsCmd(APlayerController* player_controller, FString* cmd, bool /*unused*/)
	{
		TArray<FString> parsed;
		cmd->ParseIntoArray(parsed, L" ", true);

		auto* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		if (parsed.IsValidIndex(1))
		{
			if (parsed[1].ToString() == "confirm")
			{
				database->DeleteAllKits();

				AsaApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green,
					"Successfully reset kits");
			}
		}
		else
		{
			AsaApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Yellow,
				"You are going to reset kits for ALL players\nType 'ResetKits confirm' in console if you want to continue");
		}
	}

	// Rcon

	void ChangeKitAmountRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld* /*unused*/)
	{
		FString reply;

		const bool result = ChangeKitAmountCbk(rcon_packet->Body);
		if (result)
		{
			reply = "Successfully changed kit amount";
		}
		else
		{
			reply = "Couldn't change kit amount";
		}

		rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
	}

	// Hook

	void Hook_AShooterCharacter_AuthPostSpawnInit(AShooterCharacter* _this)
	{
		AShooterCharacter_AuthPostSpawnInit_original(_this);

		AShooterPlayerController* player = AsaApi::GetApiUtils().FindControllerFromCharacter(static_cast<AShooterCharacter*>(_this));
		if (player != nullptr)
		{
			const FString& eos_id = AsaApi::IApiUtils::GetEOSIDFromController(player);
			if (!eos_id.IsEmpty() && !ArkShop::DBHelper::IsPlayerExists(eos_id))
				database->TryAddNewPlayer(eos_id);

			const std::string default_kit = config["General"].value("DefaultKit", "");
			if (!default_kit.empty())
			{
				try
				{
					const FString fdefault_kit(default_kit);

					TArray<FString> kits;
					fdefault_kit.ParseIntoArray(kits, L",", true);

					for (const auto& kit : kits)
					{
						if (const int kit_amount = GetKitAmount(eos_id, kit);
							kit_amount > 0 && CanUseKit(player, eos_id, kit))
						{
							RedeemKit(player, kit, false, true);
							break;
						}
					}
				}
				catch (const std::exception& exception)
				{
					Log::GetLog()->error("({} {}) Unexpected error {}", __FILE__, __FUNCTION__, exception.what());
				}
			}
		}
	}

	void Init()
	{
		auto& commands = AsaApi::GetCommands();

		commands.AddChatCommand(GetText("KitCmd"), &Kit);
		commands.AddChatCommand(GetText("BuyKitCmd"), &BuyKit);

		commands.AddConsoleCommand("ChangeKitAmount", &ChangeKitAmountCmd);
		commands.AddConsoleCommand("ResetKits", &ResetKitsCmd);

		commands.AddRconCommand("ChangeKitAmount", &ChangeKitAmountRcon);

		AsaApi::GetHooks().SetHook("AShooterCharacter.AuthPostSpawnInit()", &Hook_AShooterCharacter_AuthPostSpawnInit, &AShooterCharacter_AuthPostSpawnInit_original);
	}

	void Unload()
	{
		auto& commands = AsaApi::GetCommands();

		commands.RemoveChatCommand(GetText("KitCmd"));
		commands.RemoveChatCommand(GetText("BuyKitCmd"));

		commands.RemoveConsoleCommand("ChangeKitAmount");
		commands.RemoveConsoleCommand("ResetKits");

		commands.RemoveRconCommand("ChangeKitAmount");

		AsaApi::GetHooks().DisableHook("AShooterCharacter.AuthPostSpawnInit()", &Hook_AShooterCharacter_AuthPostSpawnInit);
	}
} // namespace Kits // namespace ArkShop
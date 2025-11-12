#include <API/ARK/Ark.h>
#include <fstream>

#pragma comment(lib, "AsaApi.lib")

namespace WelcomePlugin
{
	// Configuration storage
	nlohmann::json config;

	// Helper function to replace placeholders in messages
	FString FormatMessage(const FString& message, const FString& playerName, const FString& serverName)
	{
		FString result = message;
		result = result.Replace(L"{player}", *playerName, ESearchCase::IgnoreCase);
		result = result.Replace(L"{server}", *serverName, ESearchCase::IgnoreCase);
		return result;
	}

	// Hook for when a player joins the server
	DECLARE_HOOK(AShooterGameMode_HandleNewPlayer, bool, AShooterGameMode* _this, AShooterPlayerController* new_player,
		UPrimalPlayerData* player_data, AShooterCharacter* player_character, bool is_from_login)
	{
		// Call original function
		bool result = AShooterGameMode_HandleNewPlayer_original(_this, new_player, player_data, player_character, is_from_login);

		if (new_player && is_from_login)
		{
			try
			{
				// Check if welcome message is enabled
				if (config["General"]["WelcomeEnabled"].get<bool>())
				{
					// Get player name
					FString playerName;
					new_player->GetPlayerCharacterName(&playerName);

					// Get server name from config
					FString serverName = FString(ArkApi::Tools::Utf8Decode(config["General"]["ServerName"].get<std::string>()).c_str());

					// Get welcome message template
					FString welcomeMsg = FString(ArkApi::Tools::Utf8Decode(config["General"]["WelcomeMessage"].get<std::string>()).c_str());

					// Format the message
					FString formattedMsg = FormatMessage(welcomeMsg, playerName, serverName);

					// Get display settings
					float textSize = config["General"]["TextSize"].get<float>();
					float displayTime = config["General"]["DisplayTime"].get<float>();

					// Send welcome notification to player
					AsaApi::GetApiUtils().SendNotification(new_player, FColorList::Green, textSize, displayTime, nullptr, *formattedMsg);

					// Log to server console
					Log::GetLog()->info("Player {} joined the server", playerName.ToString());
				}
			}
			catch (const std::exception& ex)
			{
				Log::GetLog()->error("Error in welcome message: {}", ex.what());
			}
		}

		return result;
	}

	// Chat command: /serverinfo - Shows server information
	void ServerInfoCommand(AShooterPlayerController* player_controller, FString* message, int mode)
	{
		if (!player_controller)
			return;

		try
		{
			// Get server info from config
			std::string description = config["ServerInfo"]["Description"].get<std::string>();
			std::string website = config["ServerInfo"]["Website"].get<std::string>();
			std::string discord = config["ServerInfo"]["Discord"].get<std::string>();
			std::string serverName = config["General"]["ServerName"].get<std::string>();

			// Build info message
			FString infoMsg = FString::Format(
				TEXT("=== {} ===\n{}\nWebsite: {}\nDiscord: {}"),
				ArkApi::Tools::Utf8Decode(serverName).c_str(),
				ArkApi::Tools::Utf8Decode(description).c_str(),
				ArkApi::Tools::Utf8Decode(website).c_str(),
				ArkApi::Tools::Utf8Decode(discord).c_str()
			);

			float textSize = config["General"]["TextSize"].get<float>();
			float displayTime = config["General"]["DisplayTime"].get<float>();

			// Send server info to player
			AsaApi::GetApiUtils().SendNotification(player_controller, FColorList::Cyan, textSize, displayTime, nullptr, *infoMsg);
		}
		catch (const std::exception& ex)
		{
			Log::GetLog()->error("Error in serverinfo command: {}", ex.what());
			AsaApi::GetApiUtils().SendServerMessage(player_controller, FColorList::Red, L"Error retrieving server info");
		}
	}

	// RCON command: welcomeplugin.reload - Reloads the plugin configuration
	void ReloadConfigRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
	{
		try
		{
			// Read config file
			const std::string config_path = AsaApi::Tools::GetCurrentDir() + "/AsaApi/Plugins/WelcomePlugin/config.json";
			std::ifstream file(config_path);
			if (!file.is_open())
			{
				rcon_connection->SendMessageW(rcon_packet->Id, 0, L"Failed to open config file");
				return;
			}

			file >> config;
			file.close();

			rcon_connection->SendMessageW(rcon_packet->Id, 0, L"Configuration reloaded successfully");
			Log::GetLog()->info("WelcomePlugin configuration reloaded");
		}
		catch (const std::exception& ex)
		{
			rcon_connection->SendMessageW(rcon_packet->Id, 0, L"Failed to reload configuration");
			Log::GetLog()->error("Error reloading config: {}", ex.what());
		}
	}

	// Load plugin configuration
	void LoadConfig()
	{
		try
		{
			const std::string config_path = AsaApi::Tools::GetCurrentDir() + "/AsaApi/Plugins/WelcomePlugin/config.json";
			std::ifstream file(config_path);
			if (!file.is_open())
			{
				Log::GetLog()->error("Failed to open config file: {}", config_path);
				return;
			}

			file >> config;
			file.close();

			Log::GetLog()->info("WelcomePlugin configuration loaded successfully");
		}
		catch (const std::exception& ex)
		{
			Log::GetLog()->error("Failed to load config: {}", ex.what());
		}
	}

	// Plugin initialization
	void Load()
	{
		Log::GetLog()->info("Loading WelcomePlugin...");

		// Load configuration
		LoadConfig();

		// Register hooks
		AsaApi::GetHooks().SetHook("AShooterGameMode.HandleNewPlayer_Implementation",
			&AShooterGameMode_HandleNewPlayer,
			&AShooterGameMode_HandleNewPlayer_original);

		// Register chat commands
		AsaApi::GetCommands().AddChatCommand("/serverinfo", &ServerInfoCommand);

		// Register RCON commands
		AsaApi::GetCommands().AddRconCommand("welcomeplugin.reload", &ReloadConfigRcon);

		Log::GetLog()->info("WelcomePlugin loaded successfully!");
	}

	// Plugin cleanup
	void Unload()
	{
		Log::GetLog()->info("Unloading WelcomePlugin...");

		// Remove hooks
		AsaApi::GetHooks().DisableHook("AShooterGameMode.HandleNewPlayer_Implementation",
			&AShooterGameMode_HandleNewPlayer);

		// Remove commands
		AsaApi::GetCommands().RemoveChatCommand("/serverinfo");
		AsaApi::GetCommands().RemoveRconCommand("welcomeplugin.reload");

		Log::GetLog()->info("WelcomePlugin unloaded successfully!");
	}
}

// Plugin entry points
extern "C" __declspec(dllexport) void Plugin_Init()
{
	WelcomePlugin::Load();
}

extern "C" __declspec(dllexport) void Plugin_Unload()
{
	WelcomePlugin::Unload();
}

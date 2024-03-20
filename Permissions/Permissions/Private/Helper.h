#pragma once

namespace Permissions
{
	inline std::string GetDbPath()
	{
		return AsaApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/Permissions/ArkDB.db";
	}

	inline std::string GetConfigPath()
	{
		return AsaApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/Permissions/config.json";
	}

	inline void SendRconReply(RCONClientConnection* rcon_connection, int packet_id, const FString& msg)
	{
		FString reply = msg + "\n";
		rcon_connection->SendMessageW(packet_id, 0, &reply);
	}
}

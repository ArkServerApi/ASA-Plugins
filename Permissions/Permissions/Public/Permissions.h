#pragma once

#include <API/Ark/Ark.h>

#ifdef PERMISSIONS_EXPORTS
#define PERMISSIONS_API __declspec(dllexport)
#else
#define PERMISSIONS_API __declspec(dllimport)
#endif

namespace Permissions
{
	PERMISSIONS_API TArray<FString> GetPlayerGroups(const FString& eos_id);
	PERMISSIONS_API TArray<FString> GetGroupPermissions(const FString& group);
	PERMISSIONS_API TArray<FString> GetGroupMembers(const FString& group);

	PERMISSIONS_API bool IsPlayerInGroup(const FString& eos_id, const FString& group);

	PERMISSIONS_API std::optional<std::string> AddPlayerToGroup(const FString& eos_id, const FString& group);
	PERMISSIONS_API std::optional<std::string> RemovePlayerFromGroup(const FString& eos_id, const FString& group);

	PERMISSIONS_API std::optional<std::string> AddPlayerToTimedGroup(const FString& eos_id, const FString& group, int secs, int delaySecs);
	PERMISSIONS_API std::optional<std::string> RemovePlayerFromTimedGroup(const FString& eos_id, const FString& group);

	PERMISSIONS_API std::optional<std::string> AddGroup(const FString& group);
	PERMISSIONS_API std::optional<std::string> RemoveGroup(const FString& group);

	PERMISSIONS_API bool IsGroupHasPermission(const FString& group, const FString& permission);
	PERMISSIONS_API bool IsPlayerHasPermission(const FString& eos_id, const FString& permission);

	PERMISSIONS_API std::optional<std::string> GroupGrantPermission(const FString& group, const FString& permission);
	PERMISSIONS_API std::optional<std::string> GroupRevokePermission(const FString& group, const FString& permission);


	PERMISSIONS_API std::optional<std::string> AddTribeToGroup(int tribeId, const FString& group);
	PERMISSIONS_API std::optional<std::string> RemoveTribeFromGroup(int tribeId, const FString& group);

	PERMISSIONS_API std::optional<std::string> AddTribeToTimedGroup(int tribeId, const FString& group, int secs, int delaySecs);
	PERMISSIONS_API std::optional<std::string> RemoveTribeFromTimedGroup(int tribeId, const FString& group);

	PERMISSIONS_API bool IsTribeInGroup(int tribeId, const FString& group);
	PERMISSIONS_API bool IsTribeHasPermission(int tribeId, const FString& permission);
	PERMISSIONS_API TArray<FString> GetTribeGroups(int tribeId);

	PERMISSIONS_API void AddPlayerPermissionCallback(FString CallbackName, bool onlyCheckOnline, bool cacheBySteamId, bool cacheByTribe, const std::function<TArray<FString>(const FString&, int*)>& callback);
	PERMISSIONS_API void RemovePlayerPermissionCallback(FString CallbackName);
}

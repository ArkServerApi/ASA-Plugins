#pragma once

#include "../CachedPermission.h"
#include "../Public/Permissions.h"
#include "API/ARK/Other.h"

class IDatabase
{
protected:
	std::unordered_map<std::string, std::string> permissionGroups;
	std::unordered_map<FString, CachedPermission, FStringHash, FStringEqual> permissionPlayers;
	std::unordered_map<int, CachedPermission> permissionTribes;
	std::mutex playersMutex, groupsMutex, tribesMutex;

public:
	virtual ~IDatabase() = default;

	virtual bool IsFieldExists(std::string tableName, std::string fieldName) = 0;

	virtual bool AddPlayer(const FString& eos_id) = 0;
	virtual bool IsPlayerExists(const FString& eos_id) = 0;
	virtual bool IsGroupExists(const FString& group) = 0;
	virtual TArray<FString> GetPlayerGroups(const FString& eos_id, bool includeTimed = true) = 0;
	virtual CachedPermission HydratePlayerGroups(const FString& eos_id) = 0;
	virtual TArray<FString> GetGroupPermissions(const FString& group) = 0;
	virtual TArray<FString> GetAllGroups() = 0;
	virtual TArray<FString> GetGroupMembers(const FString& group) = 0;
	virtual std::optional<std::string> AddPlayerToGroup(const FString& eos_id, const FString& group) = 0;
	virtual std::optional<std::string> RemovePlayerFromGroup(const FString& eos_id, const FString& group) = 0;
	virtual std::optional<std::string> AddGroup(const FString& group) = 0;
	virtual std::optional<std::string> RemoveGroup(const FString& group) = 0;
	virtual std::optional<std::string> GroupGrantPermission(const FString& group, const FString& permission) = 0;
	virtual std::optional<std::string> GroupRevokePermission(const FString& group, const FString& permission) = 0;
	virtual std::optional<std::string> AddPlayerToTimedGroup(const FString& eos_id, const FString& group, int secs, int delaySecs) = 0;
	virtual std::optional<std::string> RemovePlayerFromTimedGroup(const FString& eos_id, const FString& group) = 0;

	virtual bool IsTribeExists(int tribeId) = 0;
	virtual bool AddTribe(int tribeId) = 0;
	virtual TArray<FString> GetTribeGroups(int tribeId, bool includeTimed = true) = 0;
	virtual CachedPermission HydrateTribeGroups(int tribeId) = 0;
	virtual std::optional<std::string> AddTribeToGroup(int tribeId, const FString& group) = 0;
	virtual std::optional<std::string> RemoveTribeFromGroup(int tribeId, const FString& group) = 0;
	virtual std::optional<std::string> AddTribeToTimedGroup(int tribeId, const FString& group, int secs, int delaySecs) = 0;
	virtual std::optional<std::string> RemoveTribeFromTimedGroup(int tribeId, const FString& group) = 0;

	virtual void Init() = 0;
	// Each returns nullopt if the read failed, so a failed sync keeps the current cache instead of emptying it.
	virtual std::optional<std::unordered_map<std::string, std::string>> InitGroups() = 0;
	virtual std::optional<std::unordered_map<FString, CachedPermission, FStringHash, FStringEqual>> InitPlayers() = 0;
	virtual std::optional<std::unordered_map<int, CachedPermission>> InitTribes() = 0;

	// Permission-callback cache. Shared by both backends; entries are dropped on the next sync.
	bool GetCachedPlayerCallbackGroups(const FString& eos_id, const FString& callbackName, TArray<FString>& out)
	{
		std::lock_guard<std::mutex> lg(playersMutex);
		return FindCallbackGroups(permissionPlayers, eos_id, callbackName, out);
	}

	bool GetCachedTribeCallbackGroups(int tribeId, const FString& callbackName, TArray<FString>& out)
	{
		std::lock_guard<std::mutex> lg(tribesMutex);
		return FindCallbackGroups(permissionTribes, tribeId, callbackName, out);
	}

	void UpdatePlayerGroupCallbacks(const FString& eos_id, const FString& callbackName, const TArray<FString>& groups)
	{
		std::lock_guard<std::mutex> lg(playersMutex);
		if (auto it = permissionPlayers.find(eos_id); it != permissionPlayers.end())
			it->second.CallbackGroups[callbackName] = groups;
	}

	void UpdateTribeGroupCallbacks(int tribeId, const FString& callbackName, const TArray<FString>& groups)
	{
		std::lock_guard<std::mutex> lg(tribesMutex);
		if (auto it = permissionTribes.find(tribeId); it != permissionTribes.end())
			it->second.CallbackGroups[callbackName] = groups;
	}

protected:
	// Group names are stored inside comma-separated lists and "delay;expire;name" timed entries,
	// so a ',' or ';' in a name would corrupt every row that references it.
	static std::optional<std::string> ValidateGroupName(const FString& group)
	{
		if (group.IsEmpty())
			return "Group name can't be empty";
		const std::string name = group.ToString();
		if (name.find_first_of(",;") != std::string::npos)
			return "Group name can't contain ',' or ';'";
		return {};
	}

	// Swaps in freshly read maps. Called with maps built outside the locks, so permission checks
	// on the game thread never wait for a database read.
	void ApplySync(std::optional<std::unordered_map<std::string, std::string>> groups,
		std::optional<std::unordered_map<FString, CachedPermission, FStringHash, FStringEqual>> players,
		std::optional<std::unordered_map<int, CachedPermission>> tribes)
	{
		if (groups)
		{
			std::lock_guard<std::mutex> lg(groupsMutex);
			permissionGroups.swap(*groups);
		}
		if (players)
		{
			std::lock_guard<std::mutex> lg(playersMutex);
			permissionPlayers.swap(*players);
		}
		if (tribes)
		{
			std::lock_guard<std::mutex> lg(tribesMutex);
			permissionTribes.swap(*tribes);
		}
	}

private:
	template <typename Map, typename Key>
	static bool FindCallbackGroups(Map& map, const Key& key, const FString& callbackName, TArray<FString>& out)
	{
		auto it = map.find(key);
		if (it == map.end())
			return false;
		auto cached = it->second.CallbackGroups.find(callbackName);
		if (cached == it->second.CallbackGroups.end())
			return false;
		out = cached->second;
		return true;
	}
};

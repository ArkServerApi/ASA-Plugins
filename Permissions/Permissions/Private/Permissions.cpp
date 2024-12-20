#include "../Public/Permissions.h"

#include "Main.h"

struct PermissionCallback
{
	PermissionCallback(FString command, bool onlyCheckOnline, bool cacheBySteamId, bool cacheByTribe, std::function<TArray<FString>(const FString&, int*)> callback)
		: command(std::move(command)),
		cacheBySteamId(std::move(cacheBySteamId)),
		cacheByTribe(std::move(cacheByTribe)),
		onlyCheckOnline(std::move(onlyCheckOnline)),
		callback(std::move(callback))
	{
	}

	FString command;
	bool cacheBySteamId, cacheByTribe, onlyCheckOnline;
	std::function<TArray<FString>(const FString&, int*)> callback;
};

struct PermissionGroupUpdatedCallback
{
	PermissionGroupUpdatedCallback(FString CallbackName, std::function<void(const FString&, int)> callback)
		: SubscriberUID(std::move(CallbackName)),
		callback(std::move(callback))
	{
	}

	FString SubscriberUID;
	std::function<void(const FString&, int)> callback;
};

namespace Permissions
{
#pragma region Subscribers
	std::vector<std::shared_ptr<PermissionGroupUpdatedCallback>> permissionGroupUpdatedSubscribers;

	/// <summary>
	/// Subscribes to the PermissionGroupUpdatedCallback
	/// 
	/// CallbackName is a unique identifier for the subscriber to be able to unsubscribe using a combination of PluginName and ServerID is recommended
	/// </summary>
	/// <param name="CallbackName"></param>
	/// <param name="callback"></param>
	void SubscribePermissionGroupUpdatedCallback(FString CallbackName, const std::function<void(const FString&, int)>& callback)
	{
		permissionGroupUpdatedSubscribers.push_back(std::make_shared<PermissionGroupUpdatedCallback>(CallbackName, callback));
	}

	/// <summary>
	/// Removes the subscriber from the list of subscribers
	/// </summary>
	/// <param name="CallbackName"></param>
	void UnSubscribePermissionGroupUpdatedCallback(FString CallbackName)
	{
		auto iter = std::find_if(permissionGroupUpdatedSubscribers.begin(), permissionGroupUpdatedSubscribers.end(),
			[&CallbackName](const std::shared_ptr<PermissionGroupUpdatedCallback>& data) -> bool {return data->SubscriberUID == CallbackName; });

		if (iter != permissionGroupUpdatedSubscribers.end())
			permissionGroupUpdatedSubscribers.erase(std::remove(permissionGroupUpdatedSubscribers.begin(), permissionGroupUpdatedSubscribers.end(), *iter), permissionGroupUpdatedSubscribers.end());
	}

	/// <summary>
	/// Processes the list of subscribers and notifies them of the change
	/// </summary>
	/// <param name="eos_id"></param>
	/// <param name="tribeid"></param>
	void NotifySubscribers(const FString& eos_id, int tribeid)
	{
		for (const auto& subscriber : permissionGroupUpdatedSubscribers)
		{
			subscriber->callback(eos_id, tribeid);
		}
	}
#pragma endregion Subscribers

	std::vector<std::shared_ptr<PermissionCallback>> playerPermissionCallbacks;
	void AddPlayerPermissionCallback(FString CallbackName, bool onlyCheckOnline, bool cacheBySteamId, bool cacheByTribe, const std::function<TArray<FString>(const FString&, int*)>& callback) {
		playerPermissionCallbacks.push_back(std::make_shared<PermissionCallback>(CallbackName, onlyCheckOnline, cacheBySteamId, cacheByTribe, callback));
	}
	void RemovePlayerPermissionCallback(FString CallbackName) {
		auto iter = std::find_if(playerPermissionCallbacks.begin(), playerPermissionCallbacks.end(),
			[&CallbackName](const std::shared_ptr<PermissionCallback>& data) -> bool {return data->command == CallbackName; });

		if (iter != playerPermissionCallbacks.end())
		{
			playerPermissionCallbacks.erase(std::remove(playerPermissionCallbacks.begin(), playerPermissionCallbacks.end(), *iter), playerPermissionCallbacks.end());
		}
	}
	TArray<FString> GetCallbackGroups(const FString& eos_id, int tribeId, bool isOnline) {
		TArray<FString> groups;
		for (const auto& permissionCallback : playerPermissionCallbacks)
		{
			bool cache = false;
			if (permissionCallback->onlyCheckOnline) continue;
			if (permissionCallback->cacheBySteamId && database->IsPlayerExists(eos_id))
			{
				CachedPermission permissions = database->HydratePlayerGroups(eos_id);

				if (permissions.hasCheckedCallbacks)
				{
					for (auto group : permissions.CallbackGroups)
					{
						if (!groups.Contains(group))
							groups.Add(group);
					}
					continue;
				}
				cache = true;
			}
			if (permissionCallback->cacheByTribe && database->IsTribeExists(tribeId))
			{
				CachedPermission permissions = database->HydrateTribeGroups(tribeId);

				if (permissions.hasCheckedCallbacks)
				{
					for (auto group : permissions.CallbackGroups)
					{
						if (!groups.Contains(group))
							groups.Add(group);
					}
					continue;
				}
				cache = true;
			}
			auto callbackGroups = permissionCallback->callback(eos_id, &tribeId);
			if (cache && callbackGroups.Num() > 0)
			{
				if (permissionCallback->cacheBySteamId && database->IsPlayerExists(eos_id))
				{
					database->UpdatePlayerGroupCallbacks(eos_id, callbackGroups);
				}
				if (permissionCallback->cacheByTribe && database->IsTribeExists(tribeId))
				{
					database->UpdateTribeGroupCallbacks(tribeId, callbackGroups);
				}
			}
			for (auto group : callbackGroups)
			{
				if (!groups.Contains(group))
					groups.Add(group);
			}
		}
		return groups;
	}
	
	TArray<FString> GetPlayerGroups(const FString& eos_id)
	{
		auto world = AsaApi::GetApiUtils().GetWorld();
		TArray<FString> groups = database->GetPlayerGroups(eos_id);
		auto shooter_pc = AsaApi::GetApiUtils().FindPlayerFromEOSID(eos_id);
		int tribeId = -1;
		bool isOnline = false;
		if (shooter_pc) {
			auto tribeData = GetTribeData(shooter_pc);
			isOnline = true;
			if (tribeData) {
				tribeId = tribeData->TribeIDField();
				auto tribeGroups = GetTribeGroups(tribeId);
				for (auto tribeGroup : tribeGroups)
				{
					if (!groups.Contains(tribeGroup)) 
					{
						groups.Add(tribeGroup);
					}
				}
				auto defaultTribeGroups = GetTribeDefaultGroups(tribeData);
				for (auto tribeGroup : defaultTribeGroups) 
				{
					if (!groups.Contains(tribeGroup)) 
					{
						groups.Add(tribeGroup);
					}
				}
			}
		}
		auto callbackGroups = GetCallbackGroups(eos_id, tribeId, isOnline);
		for (auto group : callbackGroups)
		{
			if (!groups.Contains(group))
				groups.Add(group);
		}
		return groups;
	}

	TArray<FString> GetTribeGroups(int tribeId)
	{
		return database->GetTribeGroups(tribeId);
	}
	
	TArray<FString> GetGroupPermissions(const FString& group)
	{
		if (group.IsEmpty())
			return {};
		return database->GetGroupPermissions(group);
	}

	TArray<FString> GetAllGroups()
	{
		return database->GetAllGroups();
	}

	TArray<FString> GetGroupMembers(const FString& group)
	{
		return database->GetGroupMembers(group);
	}

	bool IsPlayerInGroup(const FString& eos_id, const FString& group)
	{
		TArray<FString> groups = GetPlayerGroups(eos_id);

		for (const auto& current_group : groups)
		{
			if (current_group == group)
				return true;
		}

		return false;
	}
	
	bool IsTribeInGroup(int tribeId, const FString& group)
	{
		TArray<FString> groups = GetTribeGroups(tribeId);

		for (const auto& current_group : groups)
		{
			if (current_group == group)
				return true;
		}

		return false;
	}

	std::optional<std::string> AddPlayerToGroup(const FString& eos_id, const FString& group)
	{
		auto returnvalue = database->AddPlayerToGroup(eos_id, group);
		NotifySubscribers(eos_id, 0);
		return returnvalue;
	}

	std::optional<std::string> RemovePlayerFromGroup(const FString& eos_id, const FString& group)
	{
		NotifySubscribers(eos_id, 0);
		auto returnvalue = database->RemovePlayerFromGroup(eos_id, group);
		NotifySubscribers(eos_id, 0);
		return returnvalue;
	}

	std::optional<std::string> AddPlayerToTimedGroup(const FString& eos_id, const FString& group, int secs, int delaySecs)
	{
		NotifySubscribers(eos_id, 0);
		auto returnvalue = database->AddPlayerToTimedGroup(eos_id, group, secs, delaySecs);
		NotifySubscribers(eos_id, 0);
		return returnvalue;
	}

	std::optional<std::string> RemovePlayerFromTimedGroup(const FString& eos_id, const FString& group)
	{
		auto returnvalue = database->RemovePlayerFromTimedGroup(eos_id, group);
		NotifySubscribers(eos_id, 0);
		return returnvalue;
	}

	std::optional<std::string> AddTribeToGroup(int tribeId, const FString& group)
	{
		auto returnvalue = database->AddTribeToGroup(tribeId, group);
		NotifySubscribers(L"", tribeId);
		return returnvalue;
	}

	std::optional<std::string> RemoveTribeFromGroup(int tribeId, const FString& group)
	{
		auto returnvalue = database->RemoveTribeFromGroup(tribeId, group);
		NotifySubscribers(L"", tribeId);
		return returnvalue;
	}

	std::optional<std::string> AddTribeToTimedGroup(int tribeId, const FString& group, int secs, int delaySecs)
	{
		auto returnvalue = database->AddTribeToTimedGroup(tribeId, group, secs, delaySecs);
		NotifySubscribers(L"", tribeId);
		return returnvalue;
	}

	std::optional<std::string> RemoveTribeFromTimedGroup(int tribeId, const FString& group)
	{
		auto returnvalue = database->RemoveTribeFromTimedGroup(tribeId, group);
		NotifySubscribers(L"", tribeId);
		return returnvalue;
	}
	
	std::optional<std::string> AddGroup(const FString& group)
	{
		return database->AddGroup(group);
	}

	std::optional<std::string> RemoveGroup(const FString& group)
	{
		return database->RemoveGroup(group);
	}

	bool IsGroupHasPermission(const FString& group, const FString& permission)
	{
		if (!database->IsGroupExists(group))
			return false;

		TArray<FString> permissions = GetGroupPermissions(group);

		for (const auto& current_perm : permissions)
		{
			if (current_perm == permission)
				return true;
		}

		return false;
	}

	bool IsPlayerHasPermission(const FString& eos_id, const FString& permission)
	{
		TArray<FString> groups = GetPlayerGroups(eos_id);

		for (const auto& current_group : groups)
		{
			if (IsGroupHasPermission(current_group, permission) || IsGroupHasPermission(current_group, "*"))
				return true;
		}

		return false;
	}
	
		bool IsTribeHasPermission(int tribeId, const FString& permission)
	{
		TArray<FString> groups = GetTribeGroups(tribeId);

		for (const auto& current_group : groups)
		{
			if (IsGroupHasPermission(current_group, permission) || IsGroupHasPermission(current_group, "*"))
				return true;
		}

		return false;
	}

	std::optional<std::string> GroupGrantPermission(const FString& group, const FString& permission)
	{
		return database->GroupGrantPermission(group, permission);
	}

	std::optional<std::string> GroupRevokePermission(const FString& group, const FString& permission)
	{
		return database->GroupRevokePermission(group, permission);
	}
}
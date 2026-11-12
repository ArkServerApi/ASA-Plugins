#pragma once
#include "../Public/Permissions.h"
#include "API/ARK/Other.h"

struct TimedGroup {
	FString GroupName;
	long long DelayUntilTime = 0, ExpireAtTime = 0;

	bool operator==(const TimedGroup& other) const {
		return GroupName == other.GroupName;
	}

	bool operator==(const FString& groupName) const {
		return GroupName == groupName;
	}
};
class CachedPermission {
public:
	explicit CachedPermission() {}
	CachedPermission(FString Permissions, FString TimedPermissions) {
		Permissions.ParseIntoArray(Groups, L",", true);
		TArray<FString> TimedGroupStrs;
		TimedPermissions.ParseIntoArray(TimedGroupStrs, L",", true);
		for (auto GroupStr : TimedGroupStrs) {
			TArray<FString> GroupParts;
			GroupStr.ParseIntoArray(GroupParts, L";", true);
			// Each entry is "delayUntil;expireAt;groupName". Skip anything else rather than read past the array.
			if (GroupParts.Num() < 3)
			{
				Log::GetLog()->error("({} {}) Skipping malformed timed group entry '{}'", __FILE__, __FUNCTION__, GroupStr.ToString());
				continue;
			}
			TimedGroup Group;
			Group.GroupName = GroupParts[2];
			try
			{
				Group.DelayUntilTime = std::stoll(*GroupParts[0]);
				Group.ExpireAtTime = std::stoll(*GroupParts[1]);
			}
			catch (const std::exception& exception)
			{
				Log::GetLog()->error("({} {}) Skipping timed group '{}': parsing error {}", __FILE__, __FUNCTION__, Group.GroupName.ToString(), exception.what());
				continue;
			}
			TimedGroups.Add(Group);
		}
	}
	TArray<FString> Groups;
	TArray<TimedGroup> TimedGroups;
	// Groups returned by permission callbacks, keyed by callback name. A key being present means that
	// callback has already been asked for this player or tribe; the entry lives until the next database sync.
	std::unordered_map<FString, TArray<FString>, FStringHash, FStringEqual> CallbackGroups;

	TArray<FString> getGroups(long long now) 
	{
		TArray<FString> result{ "Default" };
		for (auto group : Groups) result.AddUnique(group);
		for (auto group : TimedGroups) {
			if (group.DelayUntilTime > 0 && now < group.DelayUntilTime) {
				continue;
			}
			if (group.ExpireAtTime > 0 && now < group.ExpireAtTime) {
				result.AddUnique(group.GroupName);
			}
		}

		return result;
	}

	FString getGroupsStr(long long now) 
	{
		FString result;
		auto groups = getGroups(now);
		for (auto group : groups) {
			if (!result.IsEmpty())
				result += ",";
			result += group;
		}
		return result;
	}
};
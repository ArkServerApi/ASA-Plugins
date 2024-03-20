#pragma once

#include <API/Ark/Ark.h>

class IDatabase
{
public:
	virtual ~IDatabase() = default;

	virtual bool TryAddNewPlayer(const FString& eos_id) = 0;
	virtual bool IsPlayerExists(const FString& eos_id) = 0;

	// Kits

	virtual std::string GetPlayerKits(const FString& eos_id) = 0;
	virtual bool UpdatePlayerKits(const FString& eos_id, const std::string& kits_data) = 0;
	virtual bool DeleteAllKits() = 0;

	// Points

	virtual int GetPoints(const FString& eos_id) = 0;
	virtual bool SetPoints(const FString& eos_id, int amount) = 0;
	virtual bool AddPoints(const FString& eos_id, int amount) = 0;
	virtual bool SpendPoints(const FString& eos_id, int amount) = 0;
	virtual bool AddTotalSpent(const FString& eos_id, int amount) = 0;
	virtual int GetTotalSpent(const FString& eos_id) = 0;
	virtual bool DeleteAllPoints() = 0;
};

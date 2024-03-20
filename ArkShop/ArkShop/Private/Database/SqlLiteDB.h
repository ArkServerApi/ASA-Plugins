#pragma once

#include "../hdr/sqlite_modern_cpp.h"

#include <Tools.h>

#include "IDatabase.h"
#include "../ArkShop.h"

class SqlLite : public IDatabase
{
public:
	explicit SqlLite(const std::string& path)
		: db_(path.empty()
			? AsaApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/ArkShop/ArkShop.db"
			: path)
	{
		try
		{
			//db_ << "PRAGMA journal_mode=WAL;";

			db_ << "create table if not exists Players ("
				"Id integer primary key autoincrement not null,"
				"EosId text not null unique,"
				"Kits text default '{}',"
				"Points integer default 0,"
				"TotalSpent integer default 0"
				");";
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}
	}

	bool TryAddNewPlayer(const FString& eos_id) override
	{
		try
		{
			std::string eos_id_str = eos_id.ToString();
			db_ << "INSERT INTO Players (EosId) VALUES (?);" << eos_id_str;
			return true;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	bool IsPlayerExists(const FString& eos_id) override
	{
		int count = 0;

		try
		{
			std::string eos_id_str = eos_id.ToString();
			db_ << "SELECT count(1) FROM Players WHERE EosId = ?;" << eos_id_str >> count;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}

		return count != 0;
	}

	std::string GetPlayerKits(const FString& eos_id) override
	{
		std::string kits_config = "{}";

		try
		{
			std::string eos_id_str = eos_id.ToString();
			db_ << "SELECT Kits FROM Players WHERE EosId = ?;" << eos_id_str >> kits_config;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return kits_config;
	}

	bool UpdatePlayerKits(const FString& eos_id, const std::string& kits_data) override
	{
		try
		{
			std::string eos_id_str = eos_id.ToString();
			db_ << "UPDATE Players SET Kits = ? WHERE EosId = ?;" << kits_data << eos_id_str;
			return true;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	bool DeleteAllKits() override
	{
		try
		{
			db_ << "UPDATE Players SET Kits = \"{}\";";
			return true;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	int GetPoints(const FString& eos_id) override
	{
		int points = 0;

		try
		{
			std::string eos_id_str = eos_id.ToString();
			db_ << "SELECT Points FROM Players WHERE EosId = ?;" << eos_id_str >> points;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return points;
	}

	bool SetPoints(const FString& eos_id, int amount) override
	{
		try
		{
			std::string eos_id_str = eos_id.ToString();
			db_ << "UPDATE Players SET Points = ? WHERE EosId = ?;" << amount << eos_id_str;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}

		return true;
	}

	bool AddPoints(const FString& eos_id, int amount) override
	{
		if (amount < 0)
			return false;

		try
		{
			std::string eos_id_str = eos_id.ToString();
			db_ << "UPDATE Players SET Points = Points + ? WHERE EosId = ?;" << amount << eos_id_str;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}

		return true;
	}

	bool SpendPoints(const FString& eos_id, int amount) override
	{
		if (amount < 0)
			return false;

		try
		{
			std::string eos_id_str = eos_id.ToString();
			db_ << "UPDATE Players SET Points = Points - ? WHERE EosId = ?;" << amount << eos_id_str;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}

		return true;
	}

	bool AddTotalSpent(const FString& eos_id, int amount) override
	{
		if (amount < 0)
			return false;

		try
		{
			std::string eos_id_str = eos_id.ToString();
			db_ << "UPDATE Players SET TotalSpent = ? WHERE EosId = ?;" << amount << eos_id_str;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}

		return true;
	}

	int GetTotalSpent(const FString& eos_id) override
	{
		int points = 0;

		try
		{
			std::string eos_id_str = eos_id.ToString();
			db_ << "SELECT TotalSpent FROM Players WHERE EosId = ?;" << eos_id_str >> points;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return points;
	}

	bool DeleteAllPoints() override
	{
		try
		{
			db_ << "UPDATE Players SET Points = 0;";
			return true;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

private:
	sqlite::database db_;
};

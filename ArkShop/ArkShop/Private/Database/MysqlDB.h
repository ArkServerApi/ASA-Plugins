#pragma once

#include <mysql++11.h>

#include "IDatabase.h"

#pragma comment(lib, "mysqlclient.lib")

class MySql : public IDatabase
{
public:
	explicit MySql(std::string server, std::string username, std::string password, std::string db_name, std::string table_players, const int port)
		: table_players_(move(table_players))
	{
		try
		{
			daotk::mysql::connect_options options;
			options.server = move(server);
			options.username = move(username);
			options.password = move(password);
			options.dbname = move(db_name);
			options.autoreconnect = true;
			options.timeout = 30;
			options.port = port;

			bool result = db_.open(options);
			if (!result)
			{
				Log::GetLog()->critical("Failed to open database connection check your settings!");
			}

			result = db_.query(fmt::format("CREATE TABLE IF NOT EXISTS {} ("
				"Id INT NOT NULL AUTO_INCREMENT,"
				"EosId VARCHAR(50) NOT NULL,"
				"Kits LONGTEXT NOT NULL,"
				"Points INT DEFAULT 0,"
				"TotalSpent INT DEFAULT 0,"
				"PRIMARY KEY(Id),"
				"UNIQUE INDEX EosId_UNIQUE (EosId ASC));", table_players_));

			if (!result)
			{
				Log::GetLog()->critical("({} {}) Failed to create table!", __FILE__, __FUNCTION__);
			}
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
			return db_.query(fmt::format("INSERT INTO {} (EosId, Kits) VALUES ('{}', '{}')", table_players_, eos_id.ToString(), "{}"));
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	bool IsPlayerExists(const FString& eos_id) override
	{
		try
		{
			const auto result = db_.query(fmt::format("SELECT count(1) FROM {} WHERE EosId = '{}';", table_players_, eos_id.ToString())).get_value<int>();

			return result > 0;
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	std::string GetPlayerKits(const FString& eos_id) override
	{
		std::string kits_config = "{}";

		try
		{
			kits_config = db_.query(fmt::format("SELECT Kits FROM {} WHERE EosId = '{}';", table_players_, eos_id.ToString())).get_value<std::string>();
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return kits_config;
	}

	bool UpdatePlayerKits(const FString& eos_id, const std::string& kits_data) override
	{
		try
		{
			return db_.query(fmt::format("UPDATE {} SET Kits = '{}' WHERE EosId = '{}';", table_players_, kits_data.c_str(), eos_id.ToString()));
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	bool DeleteAllKits() override
	{
		try
		{
			return db_.query(fmt::format("UPDATE {} SET Kits = '{{}}';", table_players_));
		}
		catch (const std::exception& exception)
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
			points = db_.query(fmt::format("SELECT Points FROM {} WHERE EosId = '{}';", table_players_, eos_id.ToString())).get_value<int>();
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return points;
	}

	bool SetPoints(const FString& eos_id, int amount) override
	{
		try
		{
			return db_.query(fmt::format("UPDATE {} SET Points = {} WHERE EosId = '{}';", table_players_, amount, eos_id.ToString()));
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	bool AddPoints(const FString& eos_id, int amount) override
	{
		if (amount < 0)
			return false;

		try
		{
			return db_.query(fmt::format("UPDATE {} SET Points = Points + {} WHERE EosId = '{}';", table_players_, amount, eos_id.ToString()));
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	bool SpendPoints(const FString& eos_id, int amount) override
	{
		if (amount < 0)
			return false;

		try
		{
			AddTotalSpent(eos_id, amount);
			return db_.query(fmt::format("UPDATE {} SET Points = Points - {} WHERE EosId = '{}';", table_players_, amount, eos_id.ToString()));
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	bool AddTotalSpent(const FString& eos_id, int amount) override
	{
		if (amount < 0)
			return false;

		try
		{
			return db_.query(fmt::format("UPDATE {} SET TotalSpent = TotalSpent+{} WHERE EosId = '{}'", table_players_, amount, eos_id.ToString()));
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	int GetTotalSpent(const FString& eos_id) override
	{
		int points = 0;

		try
		{
			points = db_.query(fmt::format("SELECT TotalSpent FROM {} WHERE EosId = '{}';", table_players_, eos_id.ToString())).get_value<int>();
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return points;
	}

	bool DeleteAllPoints() override
	{
		try
		{
			return db_.query(fmt::format("UPDATE {} SET Points = 0;", table_players_));
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

private:
	daotk::mysql::connection db_;
	std::string table_players_;
};
#include <DBHelper.h>

#include "ArkShop.h"

namespace ArkShop::DBHelper
{
	bool IsPlayerExists(const FString& eos_id)
	{
		// database is empty while the plugin is inactive (bad config or database unavailable)
		return database && database->IsPlayerExists(eos_id);
	}
} // namespace DBHelper // namespace ArkShop

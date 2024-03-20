#include <DBHelper.h>

#include "ArkShop.h"

namespace ArkShop::DBHelper
{
	bool IsPlayerExists(const FString& eos_id)
	{
		return database->IsPlayerExists(eos_id);
	}
} // namespace DBHelper // namespace ArkShop

#include "../Public/DBHelper.h"

#include "Main.h"

namespace Permissions::DB
{
	bool IsPlayerExists(const FString& eos_id)
	{
		return database->IsPlayerExists(eos_id);
	}

	bool IsGroupExists(const FString& group)
	{
		return database->IsGroupExists(group);
	}
}

#pragma once

#include "Base.h"

namespace ArkShop::DBHelper
{
	/**
	 * \brief Checks if player exists in shop database
	 * \param eos_id Players steam id
	 * \return True if exists, false otherwise
	 */
	SHOP_API bool IsPlayerExists(const FString& eos_id);
} // namespace DBHelper // namespace ArkShop

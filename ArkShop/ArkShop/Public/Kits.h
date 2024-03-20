#pragma once

#include "Base.h"

namespace ArkShop::Kits
{
	void Init();
	void Unload();

	/**
	* \brief Adds or reduces kits of the specific player
	*/
	SHOP_API bool ChangeKitAmount(const FString& kit_name, int amount, const FString& eos_id);

	/**
	 * \brief Checks if player has permissions to use this kit
	 */
	SHOP_API bool CanUseKit(AShooterPlayerController* player_controller, const FString& eos_id, const FString& kit_name);

	/**
	* \brief Checks if kit exists in server config
	*/
	SHOP_API bool IsKitExists(const FString& kit_name);

	void InitKitData(const FString& eos_id);
} // namespace Kits // namespace ArkShop

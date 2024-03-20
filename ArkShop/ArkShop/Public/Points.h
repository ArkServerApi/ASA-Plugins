#pragma once

#include "Base.h"

namespace ArkShop::Points
{
	void Init();
	void Unload();

	/**
	* \brief Add points to the specific player
	* \param amount Amount of points to add
	* \param eos_id Players steam id
	* \return True if success, false otherwise
	*/
	SHOP_API bool AddPoints(int amount, const FString& eos_id);

	/**
	* \brief Subtracts points from the specific player
	* \param amount Amount of points
	* \param eos_id Players steam id
	* \return True if success, false otherwise
	*/
	SHOP_API bool SpendPoints(int amount, const FString& eos_id);

	/**
	* \brief Receives points from the specific player
	* \param eos_id Players steam id
	* \return Amount of points the player has
	*/
	SHOP_API int GetPoints(const FString& eos_id);

	int GetTotalSpent(const FString& eos_id);

	/**
	* \brief Change points amount for the specific player
	* \param eos_id Players steam id
	* \param new_amount New amount of points
	* \return True if success, false otherwise
	*/
	SHOP_API bool SetPoints(const FString& eos_id, int new_amount);
} // namespace Points // namespace ArkShop

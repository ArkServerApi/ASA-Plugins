#pragma once

#include "windows.h"

#include <climits>

namespace ArkShop::Tools
{
	inline int GetRandomNumber(int min, int max)
	{
		const int n = max - min + 1;
		const int remainder = RAND_MAX % n;
		int x;

		do
		{
			x = rand();
		}
		while (x >= RAND_MAX - remainder);

		return min + x % n;
	}

	/**
	 * \brief Multiplies two config/command values without overflowing int.
	 * \return False if the product is negative or does not fit in an int.
	 */
	inline bool MultiplyAmount(long long a, long long b, int& result)
	{
		const long long product = a * b; // inputs come from int or unsigned, so this fits in 64 bits
		if (product < 0 || product > INT_MAX)
			return false;

		result = static_cast<int>(product);
		return true;
	}
} // namespace Tools // namespace ArkShop

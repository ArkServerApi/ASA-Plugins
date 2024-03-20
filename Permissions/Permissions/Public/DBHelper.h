#pragma once

#ifdef PERMISSIONS_EXPORTS
#define PERMISSIONS_API __declspec(dllexport)
#else
#define PERMISSIONS_API __declspec(dllimport)
#endif

class FString;

namespace Permissions::DB
{
	/**
	 * \brief Checks if player exists in database
	 */
	PERMISSIONS_API bool IsPlayerExists(const FString& eos_id);

	/**
	* \brief Checks if group exists in database
	*/
	PERMISSIONS_API bool IsGroupExists(const FString& group);
}

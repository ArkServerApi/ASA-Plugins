#include "Hooks.h"

#include "Main.h"

namespace Permissions::Hooks
{
	DECLARE_HOOK(AShooterGameMode_HandleNewPlayer, bool, AShooterGameMode*, AShooterPlayerController*, UPrimalPlayerData*, AShooterCharacter*, bool);
	DECLARE_HOOK(AShooterPlayerController_ClientNotifyAdmin, void, AShooterPlayerController*);

	bool Hook_AShooterGameMode_HandleNewPlayer(AShooterGameMode* _this, AShooterPlayerController* new_player, UPrimalPlayerData* player_data, AShooterCharacter* player_character, bool is_from_login)
	{
		FString eos_id;
		new_player->GetUniqueNetIdAsString(&eos_id);
		
		if (!database->IsPlayerExists(*eos_id))
		{
			const bool res = database->AddPlayer(*eos_id);
			if (!res)
			{
				Log::GetLog()->error("({} {}) Couldn't add player", __FILE__, __FUNCTION__);
			}
		}

		return AShooterGameMode_HandleNewPlayer_original(_this, new_player, player_data, player_character,
			is_from_login);
	}

	void Hook_AShooterPlayerController_ClientNotifyAdmin(AShooterPlayerController* player_controller)
	{
		FString eos_id;
		player_controller->GetUniqueNetIdAsString(&eos_id);

			if (!IsPlayerInGroup(eos_id, "Admins"))
				database->AddPlayerToGroup(*eos_id, "Admins");

		AShooterPlayerController_ClientNotifyAdmin_original(player_controller);
	}

	void Init()
	{
		AsaApi::GetHooks().SetHook("AShooterGameMode.HandleNewPlayer_Implementation(AShooterPlayerController*,UPrimalPlayerData*,AShooterCharacter*,bool)",&Hook_AShooterGameMode_HandleNewPlayer, &AShooterGameMode_HandleNewPlayer_original);
		AsaApi::GetHooks().SetHook("AShooterPlayerController.ClientNotifyAdmin()", &Hook_AShooterPlayerController_ClientNotifyAdmin, &AShooterPlayerController_ClientNotifyAdmin_original);
	}
}
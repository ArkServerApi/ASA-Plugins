#include <API/Base.h>
#include <API/ARK/Ark.h>
#include <Timer.h>
#include "json.hpp"
#include <fstream>

#pragma comment(lib, "AsaApi.lib")

nlohmann::json config;

void UnstasisBabies()
{
    int counter = 0;
    TArray<AActor*> actors;
    UGameplayStatics::GetAllActorsOfClass(AsaApi::GetApiUtils().GetWorld(), APrimalDinoCharacter::GetPrivateStaticClass(), &actors);
    for (AActor* actor : actors)
    {
        if (actor == nullptr)
            continue;

        const int team_id = actor->TargetingTeamField();
        if (team_id < 50000)
            continue;

        APrimalDinoCharacter* dino = static_cast<APrimalDinoCharacter*>(actor);
        if (dino->IsDeadOrDying() || !dino->IsBaby() || !dino->bStasised()())
            continue;

        counter++;
        dino->Unstasis();
    }
    Log::GetLog()->info("Update tick for {} babies. Next tick in {} seconds.", counter, config["tick"].get<int>());
}

void ReadConfig()
{
    const std::string config_path = AsaApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/BabyStasisFix/config.json";
    std::ifstream file{ config_path };
    if (!file.is_open())
        throw std::runtime_error("Can't open config.json");

    file >> config;

    file.close();
}

// Called when the server is ready, do post-"server ready" initialization here
void OnServerReady()
{
    API::Timer::Get().RecurringExecute("BabyStasisFix.UnstasisBabies", &UnstasisBabies, config["tick"].get<int>(), -1, false);
}

// Hook that triggers once when the server is ready
DECLARE_HOOK(AShooterGameMode_BeginPlay, void, AShooterGameMode*);
void Hook_AShooterGameMode_BeginPlay(AShooterGameMode* _this)
{
    AShooterGameMode_BeginPlay_original(_this);

    OnServerReady();
}

// Called by AsaApi when the plugin is loaded, do pre-"server ready" initialization here
extern "C" __declspec(dllexport) void Plugin_Init()
{
    Log::Get().Init("BabyStasisFix");

    ReadConfig();

    AsaApi::GetHooks().SetHook("AShooterGameMode.BeginPlay()", Hook_AShooterGameMode_BeginPlay,
        &AShooterGameMode_BeginPlay_original);

    if (AsaApi::GetApiUtils().GetStatus() == AsaApi::ServerStatus::Ready)
        OnServerReady();
}

// Called by AsaApi when the plugin is unloaded, do cleanup here
extern "C" __declspec(dllexport) void Plugin_Unload()
{
    AsaApi::GetHooks().DisableHook("AShooterGameMode.BeginPlay()", Hook_AShooterGameMode_BeginPlay);
    API::Timer::Get().UnloadTimer("BabyStasisFix.UnstasisBabies");
}
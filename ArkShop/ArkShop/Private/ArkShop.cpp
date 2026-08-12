#include "Database/MysqlDB.h"
#include "Database/SqlLiteDB.h"

#include "ArkShop.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <random>
#include <sstream>
#include <vector>

#include <Permissions.h>
#include <DBHelper.h>
#include <Kits.h>
#include <Points.h>
#include <Store.h>

#include "StoreSell.h"
#include "TimedRewards.h"
#include <ArkShopUIHelper.h>
#include "Helpers.h"

#include "Requests.h"
#include "Ark/AsaApiUtilsMessagingManager.h"

#pragma comment(lib, "AsaApi.lib")
#pragma comment(lib, "Permissions.lib")

DECLARE_HOOK(AShooterGameMode_HandleNewPlayer, bool, AShooterGameMode*, AShooterPlayerController*, UPrimalPlayerData*, AShooterCharacter*, bool);
DECLARE_HOOK(AShooterGameMode_Logout, void, AShooterGameMode*, AController*);
DECLARE_HOOK(URCONServer_Init, bool, URCONServer*, FString, int, UShooterCheatManager*);

FString closed_store_reason;
bool store_enabled = true;

namespace
{
	struct DinoTraitEntry
	{
		std::string name;
		int tier = 0;
	};

	std::string Trim(std::string value)
	{
		const auto not_space = [](unsigned char ch)
		{
			return !std::isspace(ch);
		};

		value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
		value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
		return value;
	}

	bool ParseDinoTrait(std::string value, DinoTraitEntry& trait)
	{
		value = Trim(value);
		if (value.empty() || value.back() != ']')
			return false;

		const size_t open = value.rfind('[');
		if (open == std::string::npos || open == 0 || open + 1 >= value.size() - 1)
			return false;

		const std::string name = Trim(value.substr(0, open));
		if (name.empty())
			return false;

		int tier = 0;
		for (size_t i = open + 1; i + 1 < value.size(); ++i)
		{
			if (!std::isdigit(static_cast<unsigned char>(value[i])))
				return false;

			tier = (tier * 10) + (value[i] - '0');
		}

		if (tier < 1 || tier > 3)
			return false;

		trait.name = name;
		trait.tier = tier;
		return true;
	}

	FName MakeDinoTraitName(const DinoTraitEntry& trait)
	{
		std::ostringstream name;
		name << trait.name << "[" << (trait.tier - 1) << "]";
		return FName(name.str().c_str(), EFindName::FNAME_Add);
	}

	const nlohmann::json& DefaultDinoTraitsConfig()
	{
		static const nlohmann::json defaults = {
			{"Enabled", false},
			{"TierWeights", {
				{"1", 74},
				{"2", 22},
				{"3", 4}
			}},
			{"Traits", {
				"AberrantCarrier[1]",
				"AberrantCarrier[2]",
				"AberrantCarrier[3]",
				"Aggressive[1]",
				"Aggressive[2]",
				"Angry[1]",
				"Angry[2]",
				"Athletic[1]",
				"Athletic[2]",
				"Athletic[3]",
				"CarcassCarrier[1]",
				"CarcassCarrier[2]",
				"CarcassCarrier[3]",
				"Carefree[1]",
				"Cold[1]",
				"Cold[2]",
				"Cold[3]",
				"Cowardly[1]",
				"Cowardly[2]",
				"Cowardly[3]",
				"Distracting[1]",
				"Diurnal[1]",
				"Diurnal[2]",
				"Diurnal[3]",
				"Excitable[1]",
				"ExoticCarrier[1]",
				"ExoticCarrier[2]",
				"ExoticCarrier[3]",
				"ExtinctionCarrier[1]",
				"ExtinctionCarrier[2]",
				"ExtinctionCarrier[3]",
				"FastLearner[1]",
				"FastLearner[2]",
				"FastLearner[3]",
				"Fatty[1]",
				"Fatty[2]",
				"Fatty[3]",
				"Frenetic[1]",
				"Frenetic[2]",
				"Frenetic[3]",
				"Giantslaying[1]",
				"Giantslaying[2]",
				"HeavyHitting[1]",
				"HeavyHitting[2]",
				"HeavyHitting[3]",
				"InheritFoodFrail[1]",
				"InheritFoodFrail[2]",
				"InheritFoodFrail[3]",
				"InheritFoodMutable[1]",
				"InheritFoodMutable[2]",
				"InheritFoodMutable[3]",
				"InheritFoodRobust[1]",
				"InheritFoodRobust[2]",
				"InheritFoodRobust[3]",
				"InheritHealthFrail[1]",
				"InheritHealthFrail[2]",
				"InheritHealthFrail[3]",
				"InheritHealthMutable[1]",
				"InheritHealthMutable[2]",
				"InheritHealthMutable[3]",
				"InheritHealthRobust[1]",
				"InheritHealthRobust[2]",
				"InheritHealthRobust[3]",
				"InheritMeleeFrail[1]",
				"InheritMeleeFrail[2]",
				"InheritMeleeFrail[3]",
				"InheritMeleeMutable[1]",
				"InheritMeleeMutable[2]",
				"InheritMeleeMutable[3]",
				"InheritMeleeRobust[1]",
				"InheritMeleeRobust[2]",
				"InheritMeleeRobust[3]",
				"InheritOxygenFrail[1]",
				"InheritOxygenFrail[2]",
				"InheritOxygenFrail[3]",
				"InheritOxygenMutable[1]",
				"InheritOxygenMutable[2]",
				"InheritOxygenMutable[3]",
				"InheritOxygenRobust[1]",
				"InheritOxygenRobust[2]",
				"InheritOxygenRobust[3]",
				"InheritStaminaFrail[1]",
				"InheritStaminaFrail[2]",
				"InheritStaminaFrail[3]",
				"InheritStaminaMutable[1]",
				"InheritStaminaMutable[2]",
				"InheritStaminaMutable[3]",
				"InheritStaminaRobust[1]",
				"InheritStaminaRobust[2]",
				"InheritStaminaRobust[3]",
				"InheritWeightFrail[1]",
				"InheritWeightFrail[2]",
				"InheritWeightFrail[3]",
				"InheritWeightMutable[1]",
				"InheritWeightMutable[2]",
				"InheritWeightMutable[3]",
				"InheritWeightRobust[1]",
				"InheritWeightRobust[2]",
				"InheritWeightRobust[3]",
				"Kingslaying[1]",
				"Kingslaying[2]",
				"Kingslaying[3]",
				"MeatCarrier[1]",
				"MeatCarrier[2]",
				"MeatCarrier[3]",
				"MineralCarrier[1]",
				"MineralCarrier[2]",
				"MineralCarrier[3]",
				"Nocturnal[1]",
				"Nocturnal[2]",
				"Nocturnal[3]",
				"Numb[1]",
				"PlantCarrier[1]",
				"PlantCarrier[2]",
				"PlantCarrier[3]",
				"Protective[1]",
				"Protective[2]",
				"QuickHitting[1]",
				"QuickHitting[2]",
				"QuickHitting[3]",
				"ScorchedCarrier[1]",
				"ScorchedCarrier[2]",
				"ScorchedCarrier[3]",
				"SlowMetabolism[1]",
				"SlowMetabolism[2]",
				"SlowMetabolism[3]",
				"Sprinter[1]",
				"Sprinter[2]",
				"Sprinter[3]",
				"Swimmer[1]",
				"Swimmer[2]",
				"Swimmer[3]",
				"Tenacious[1]",
				"Tenacious[2]",
				"Tenacious[3]",
				"Vampiric[1]",
				"Warm[1]",
				"Warm[2]",
				"Warm[3]"
			}}
		};

		return defaults;
	}

	bool EnsureDinoTraitsConfig(nlohmann::json& config)
	{
		bool updated = false;
		if (!config["General"].is_object())
		{
			config["General"] = nlohmann::json::object();
			updated = true;
		}

		auto& general = config["General"];
		const auto& defaults = DefaultDinoTraitsConfig();
		auto dino_traits_iter = general.find("DinoTraits");
		if (dino_traits_iter == general.end() || !dino_traits_iter->is_object())
		{
			general["DinoTraits"] = defaults;
			return true;
		}

		auto& dino_traits = general["DinoTraits"];
		const auto enabled_iter = dino_traits.find("Enabled");
		if (enabled_iter == dino_traits.end() || !enabled_iter->is_boolean())
		{
			dino_traits["Enabled"] = defaults["Enabled"];
			updated = true;
		}

		auto weights_iter = dino_traits.find("TierWeights");
		if (weights_iter == dino_traits.end() || !weights_iter->is_object())
		{
			dino_traits["TierWeights"] = defaults["TierWeights"];
			updated = true;
		}
		else
		{
			auto& weights = dino_traits["TierWeights"];
			for (const auto& default_weight : defaults["TierWeights"].items())
			{
				const auto weight_iter = weights.find(default_weight.key());
				if (weight_iter == weights.end() || !weight_iter->is_number_integer())
				{
					weights[default_weight.key()] = default_weight.value();
					updated = true;
				}
			}
		}

		const auto traits_iter = dino_traits.find("Traits");
		if (traits_iter == dino_traits.end() || !traits_iter->is_array())
		{
			dino_traits["Traits"] = defaults["Traits"];
			updated = true;
		}

		return updated;
	}

	std::array<int, 4> GetDinoTraitTierWeights(const nlohmann::json& trait_config)
	{
		std::array<int, 4> weights{ 0, 74, 22, 4 };
		const auto weights_iter = trait_config.find("TierWeights");
		if (weights_iter == trait_config.end() || !weights_iter->is_object())
			return weights;

		for (int tier = 1; tier <= 3; ++tier)
		{
			const auto weight_iter = weights_iter->find(std::to_string(tier));
			if (weight_iter != weights_iter->end() && weight_iter->is_number_integer())
			{
				weights[tier] = weight_iter->get<int>();
				if (weights[tier] < 0)
					weights[tier] = 0;
			}
		}

		return weights;
	}

	bool ChooseRandomDinoTrait(DinoTraitEntry& selected_trait)
	{
		const auto trait_config = ArkShop::config.value("General", nlohmann::json::object())
			.value("DinoTraits", nlohmann::json::object());
		if (!trait_config.is_object())
			return false;

		const auto traits = trait_config.value("Traits", nlohmann::json::array());
		if (!traits.is_array() || traits.empty())
			return false;

		std::array<std::vector<DinoTraitEntry>, 4> traits_by_tier;
		for (const auto& raw_trait : traits)
		{
			if (!raw_trait.is_string())
				continue;

			DinoTraitEntry trait;
			if (ParseDinoTrait(raw_trait.get<std::string>(), trait))
				traits_by_tier[trait.tier].push_back(trait);
		}

		const auto weights = GetDinoTraitTierWeights(trait_config);
		int total_weight = 0;
		for (int tier = 1; tier <= 3; ++tier)
		{
			if (!traits_by_tier[tier].empty())
				total_weight += weights[tier];
		}

		if (total_weight <= 0)
			return false;

		static std::mt19937 rng{ std::random_device{}() };
		std::uniform_int_distribution<int> tier_dist(1, total_weight);
		int roll = tier_dist(rng);
		int selected_tier = 0;

		for (int tier = 1; tier <= 3; ++tier)
		{
			if (traits_by_tier[tier].empty())
				continue;

			roll -= weights[tier];
			if (roll <= 0)
			{
				selected_tier = tier;
				break;
			}
		}

		if (selected_tier == 0)
			return false;

		const auto& selected_traits = traits_by_tier[selected_tier];
		std::uniform_int_distribution<size_t> trait_dist(0, selected_traits.size() - 1);
		selected_trait = selected_traits[trait_dist(rng)];
		return true;
	}

	void ApplyRandomDinoTrait(APrimalDinoCharacter* dino)
	{
		if (!dino || dino->GeneTraitsField().Num() > 0 || dino->NextBabyGeneTraitsField().Num() > 0)
			return;

		DinoTraitEntry trait;
		if (!ChooseRandomDinoTrait(trait))
			return;

		const FName trait_name = MakeDinoTraitName(trait);
		dino->GeneTraitsField().Add(trait_name);
		dino->NextBabyGeneTraitsField().Add(trait_name);
	}
}

FString ArkShop::SetMapName()
{
	if (!ArkShop::MapName.IsEmpty())
		return ArkShop::MapName;

	LPWSTR* argv;
	int argc;
	int i;
	FString param(L"-serverkey=");
	FString LocalMapName;

	AsaApi::GetApiUtils().GetShooterGameMode()->GetMapName(&LocalMapName);

	argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (NULL != argv)
	{
		for (i = 0; i < argc; i++)
		{
			FString arg(argv[i]);
			if (arg.Contains(param))
			{
				if (arg.RemoveFromStart(param))
				{
					LocalMapName = arg;
					break;
				}
			}
		}

		LocalFree(argv);
	}

	Log::GetLog()->info("MapName: {}", LocalMapName.ToString());
	ArkShop::MapName = LocalMapName;

	return ArkShop::MapName;
}

void ArkShop::PostToDiscord(const std::wstring log)
{
	if (!ArkShop::discord_enabled || ArkShop::discord_webhook_url.IsEmpty())
		return;

	FString msg = L"{{\"content\":\"```stylus\\n{}```\",\"username\":\"{}\",\"avatar_url\":null}}";
	FString output = FString::Format(*msg, log, ArkShop::discord_sender_name);

	auto myCallback = [](bool, std::string, std::unordered_map<std::string, std::string>) {};

	bool result = API::Requests::Get().CreatePostRequest(
		ArkShop::discord_webhook_url.ToString(),
		myCallback,
		API::Tools::Utf8Encode(*output),
		"application/json"
	);
}

float ArkShop::getStatValue(float StatModifier, float InitialValueConstant, float RandomizerRangeMultiplier, float StateModifierScale, bool bDisplayAsPercent)
{
	float ItemStatValue;

	if (bDisplayAsPercent)
		InitialValueConstant += 100;

	if (InitialValueConstant > StatModifier)
		StatModifier = InitialValueConstant;

	ItemStatValue = (StatModifier - InitialValueConstant) / (InitialValueConstant * RandomizerRangeMultiplier * StateModifierScale);

	return ItemStatValue;
}

void ArkShop::ApplyItemStats(TArray<UPrimalItem*> items, int armor, int durability, int damage)
{
	if (armor > 0 || durability > 0 || damage > 0)
	{
		for (UPrimalItem* item : items)
		{
			bool updated = false;

			auto ItemStatInfos = item->ItemStatInfosField();

			if (armor > 0)
			{
				FItemStatInfo itemstat = ItemStatInfos()[EPrimalItemStat::Armor];

				if (itemstat.bUsed)
				{
					float newStat = 0.f;
					bool percent = itemstat.bDisplayAsPercent;

					newStat = getStatValue(armor, itemstat.InitialValueConstant, itemstat.RandomizerRangeMultiplier, itemstat.StateModifierScale, percent);

					if (newStat >= 65536.f)
						newStat = 65535;

					item->ItemStatValuesField()()[EPrimalItemStat::Armor] = newStat;
					updated = true;
				}
			}

			if (durability > 0)
			{
				FItemStatInfo itemstat = ItemStatInfos()[EPrimalItemStat::MaxDurability];

				if (itemstat.bUsed)
				{
					float newStat = 0.f;
					bool percent = itemstat.bDisplayAsPercent;

					newStat = getStatValue(durability, itemstat.InitialValueConstant, itemstat.RandomizerRangeMultiplier, itemstat.StateModifierScale, percent) + 1;

					if (newStat >= 65536.f)
						newStat = 65535;

					item->ItemStatValuesField()()[EPrimalItemStat::MaxDurability] = newStat;
					item->ItemDurabilityField() = item->GetItemStatModifier(EPrimalItemStat::MaxDurability);
					updated = true;
				}
			}

			if (damage > 0)
			{
				FItemStatInfo itemstat = ItemStatInfos()[EPrimalItemStat::WeaponDamagePercent];

				if (itemstat.bUsed)
				{
					float newStat = 0.f;
					bool percent = itemstat.bDisplayAsPercent;

					newStat = getStatValue(damage, itemstat.InitialValueConstant, itemstat.RandomizerRangeMultiplier, itemstat.StateModifierScale, percent);

					if (newStat >= 65536.f)
						newStat = 65535;

					item->ItemStatValuesField()()[EPrimalItemStat::WeaponDamagePercent] = newStat;
					updated = true;
				}
			}

			if (updated)
				item->UpdatedItem(false, false);
		}
	}
}

// Builds custom data for cryopod
FCustomItemData ArkShop::GetDinoCustomItemData(APrimalDinoCharacter* dino, UPrimalItem* saddle)
{
	FCustomItemData customItemData;

	FARKDinoData dinoData;
	dino->GetDinoData(&dinoData);

	//
	// Custom Data Name
	//
	customItemData.CustomDataName = FName("Dino", EFindName::FNAME_Add);

	TArray<FName> names;
	dino->GetColorSetNamesAsArray(&names);
	customItemData.CustomDataNames = names;
	// one time use settings
	customItemData.CustomDataNames.Add(FName("MissionTemporary", EFindName::FNAME_Add));
	customItemData.CustomDataNames.Add(FName("None", EFindName::FNAME_Find));

	//
	// Custom Data Floats
	//
	customItemData.CustomDataFloats = ArkShop::GetCharacterStatsAsFloats(dino);

	//
	// Custom Data Doubles
	//
	auto t1 = AsaApi::GetApiUtils().GetShooterGameMode()->GetWorld()->TimeSecondsField();
	customItemData.CustomDataDoubles.Doubles.Add(t1);
	customItemData.CustomDataDoubles.Doubles.Add(dino->BabyNextCuddleTimeField() - t1);
	customItemData.CustomDataDoubles.Doubles.Add(dino->NextAllowedMatingTimeField());

	const float d1 = static_cast<float>(dino->RandomMutationsMaleField());
	const double d2 = static_cast<double>(d1);
	customItemData.CustomDataDoubles.Doubles.Add(d2);

	const float d3 = static_cast<float>(dino->RandomMutationsFemaleField());
	const double d4 = static_cast<double>(d3);
	customItemData.CustomDataDoubles.Doubles.Add(d4);

	auto stat = dino->MyCharacterStatusComponentField();
	if (stat)
	{
		const double d5 = static_cast<double>(stat->DinoImprintingQualityField());
		customItemData.CustomDataDoubles.Doubles.Add(d5);
	}
	customItemData.CustomDataDoubles.Doubles.Add(std::time(nullptr));

	//
	// Custom Data Strings
	//
	customItemData.CustomDataStrings = ArkShop::GetDinoDataStrings(dino, dinoData.DinoNameInMap, dinoData.DinoName, saddle);

	//
	// Custom Data Classes
	//
	customItemData.CustomDataClasses.Add(dinoData.DinoClass);

	//
	//	Custom Data Soft Classes
	//
	//customItemData.CustomDataSoftClasses.Add(dinoData.DinoClass);

	//
	// Custom Data Bytes
	//
	FCustomItemByteArray dinoBytes, saddlebytes;
	dinoBytes.Bytes = dinoData.DinoData;
	customItemData.CustomDataBytes.ByteArrays.Add(dinoBytes);

	if (saddle)
	{
		saddle->GetItemBytes(&saddlebytes.Bytes);
		customItemData.CustomDataBytes.ByteArrays.Add(saddlebytes);
	}
	customItemData.CustomDataBytes.ByteArrays.Add(FCustomItemByteArray());

	FCustomItemByteArray arr = FCustomItemByteArray();
	arr.Bytes.Add(dino->TamedAggressionLevelField());
	customItemData.CustomDataBytes.ByteArrays.Add(arr);

	return customItemData;
}

void HandleStryder(APrimalDinoCharacter* dino, int stryderhead, int stryderchest)
{
	if (stryderhead >= 0 && stryderhead < 3)
	{
		FString attachmentBP("");
		switch (stryderhead)
		{
		case 0:
			attachmentBP = L"Blueprint'/Game/Genesis2/CoreBlueprints/Items/Saddle/PrimalItemArmor_TekStriderHarvester.PrimalItemArmor_TekStriderHarvester'";
			break;
		case 1:
			attachmentBP = "Blueprint'/Game/Genesis2/CoreBlueprints/Items/Saddle/PrimalItemArmor_TekStriderSilenceCannon.PrimalItemArmor_TekStriderSilenceCannon'";
			break;
		case 2:
			attachmentBP = "Blueprint'/Game/Genesis2/CoreBlueprints/Items/Saddle/PrimalItemArmor_TekStriderMachinegun.PrimalItemArmor_TekStriderMachinegun'";
			break;
		case 3:
			attachmentBP = "Blueprint'/Game/Genesis2/CoreBlueprints/Items/Saddle/PrimalItemArmor_TekStriderRadar.PrimalItemArmor_TekStriderRadar'";
			break;
		}

		UPrimalItem* head = dino->MyInventoryComponentField()->GetEquippedItemOfType(EPrimalEquipmentType::Hat);
		if (head)
		{
			head->bForceDropDestruction().Set(true);
			head->RemoveItemFromInventory(true, false);

			UClass* Class = UVictoryCore::BPLoadClass(attachmentBP);
			UPrimalItem* item = UPrimalItem::AddNewItem(Class, dino->MyInventoryComponentField(), true, true, 1, true, 1, false, 0, false, nullptr, 0, false, false, true, false, false, false, AsaApi::GetApiUtils().GetWorld());
			if (item)
			{
				FProperty* setHeadAttachment = dino->FindProperty(FName("set head attachment", EFindName::FNAME_Add));
				if (setHeadAttachment)
					setHeadAttachment->Set(dino, true);

				FProperty* headAttachmentIndex = dino->FindProperty(FName("head attachment index", EFindName::FNAME_Add));
				if (headAttachmentIndex)
					headAttachmentIndex->Set(dino, stryderhead);
			}
			else
			{
				dino->Destroy(false, false);
				return;
			}
		}
	}

	if (stryderchest >= 0 && stryderchest < 4)
	{
		FString attachmentBP("");
		switch (stryderchest)
		{
		case 0:
			attachmentBP = L"Blueprint'/Game/Genesis2/CoreBlueprints/Items/Saddle/PrimalItemArmor_TekStriderShield.PrimalItemArmor_TekStriderShield'";
			break;
		case 1:
			attachmentBP = "Blueprint'/Game/Genesis2/CoreBlueprints/Items/Saddle/PrimalItemArmor_TekStriderOnesidedShield.PrimalItemArmor_TekStriderOnesidedShield'";
			break;
		case 2:
			attachmentBP = "Blueprint'/Game/Genesis2/CoreBlueprints/Items/Saddle/PrimalItemArmor_TekStriderLargeCannon.PrimalItemArmor_TekStriderLargeCannon'";
			break;
		case 3:
			attachmentBP = "Blueprint'/Game/Genesis2/CoreBlueprints/Items/Saddle/PrimalItemArmor_TekStriderSaddlebags.PrimalItemArmor_TekStriderSaddlebags'";
			break;
		}

		UPrimalItem* chest = dino->MyInventoryComponentField()->GetEquippedItemOfType(EPrimalEquipmentType::Shirt);
		if (chest)
		{
			chest->bForceDropDestruction().Set(true);
			chest->RemoveItemFromInventory(true, false);

			UClass* Class = UVictoryCore::BPLoadClass(attachmentBP);
			UPrimalItem* item = UPrimalItem::AddNewItem(Class, dino->MyInventoryComponentField(), true, true, 1, true, 1, false, 0, false, nullptr, 0, false, false, true, false, false, false, AsaApi::GetApiUtils().GetWorld());
			if (item)
			{
				FProperty* setChestAttachment = dino->FindProperty(FName("set chest attachment", EFindName::FNAME_Add));
				if (setChestAttachment)
					setChestAttachment->Set(dino, true);

				FProperty* chestAttachmentIndex = dino->FindProperty(FName("chest attachment index", EFindName::FNAME_Add));
				if (chestAttachmentIndex)
					chestAttachmentIndex->Set(dino, stryderchest);
			}
			else
			{
				dino->Destroy(false, false);
				return;
			}
		}
	}
}

void HandleGacha(APrimalDinoCharacter* dino, nlohmann::json resourceOverrides)
{
	if (resourceOverrides.empty() || resourceOverrides == "")
		return;

	struct Gacha_ResourceStruct
	{
		UClass* Class;
		float BaseQuantity;
	};

	TArray<Gacha_ResourceStruct> X;
	for (auto Iter = resourceOverrides.begin(); Iter != resourceOverrides.end(); ++Iter)
	{
		FString tempResource(Iter.key());
		UClass* tempClass = UVictoryCore::BPLoadClass(tempResource);
		X.Emplace(Gacha_ResourceStruct{ tempClass, Iter.value() });
	}

	FProperty* resourceProduction = dino->FindProperty(FName("ResourceProduction", EFindName::FNAME_Find));
	if (resourceProduction)
	{
		if (dino->ClassField()->HasProperty(resourceProduction))
		{
			auto* currentList = ((TArray<Gacha_ResourceStruct>*)(dino + resourceProduction->Offset_InternalField()));
			if (currentList)
			{
				int counter = 0;
				for (auto Iter = currentList->CreateIterator(); Iter; ++Iter)
				{
					if (X.IsValidIndex(counter))
					{
						Iter->Class = X[counter].Class;
						Iter->BaseQuantity = X[counter].BaseQuantity;
					}

					counter += 1;
				}
			}
		}
	}
}

//Spawns dino or gives in cryopod
bool ArkShop::GiveDino(AShooterPlayerController* player_controller, int level, bool neutered, std::string gender, std::string blueprint, std::string saddleblueprint, bool PreventCryo, int stryderhead, int stryderchest, nlohmann::json resourceOverrides, bool giveRandomTrait)
{
	bool success = false;
	const FString fblueprint(blueprint.c_str());
	APrimalDinoCharacter* dino = AsaApi::GetApiUtils().SpawnDino(player_controller, fblueprint, nullptr, level, true, neutered);
	if (dino)
	{
		if (fblueprint.Contains("Blueprint'/Game/Genesis2/Dinos/TekStrider/TekStrider_Character_BP.TekStrider_Character_BP'"))
			HandleStryder(dino, stryderhead, stryderchest);
		else if (fblueprint.Contains("Blueprint'/Game/Extinction/Dinos/Gacha/Gacha_Character_BP.Gacha_Character_BP'"))
			HandleGacha(dino, resourceOverrides);

		if (giveRandomTrait)
			ApplyRandomDinoTrait(dino);

		if (dino->bUsesGender()())
		{
			if (strcmp(gender.c_str(), "male") == 0)
				dino->bIsFemale() = false;
			else if (strcmp(gender.c_str(), "female") == 0)
				dino->bIsFemale() = true;
		}

		UPrimalItem* saddle = nullptr;
		if (saddleblueprint.size() > 0)
		{
			FString fsaddleblueprint(saddleblueprint.c_str());
			UClass* saddleClass = UVictoryCore::BPLoadClass(fsaddleblueprint);
			saddle = UPrimalItem::AddNewItem(saddleClass, dino->MyInventoryComponentField(), true, false, 0, false, 0, false, 0, false, nullptr, 0, false, false, true, false, false, false, AsaApi::GetApiUtils().GetWorld());
		}

		const FString cryo = FString(ArkShop::config["General"].value("CryoItemPath", "Blueprint'/Game/Extinction/CoreBlueprints/Weapons/PrimalItem_WeaponEmptyCryopod.PrimalItem_WeaponEmptyCryopod'"));
		TSubclassOf<UPrimalItem> cryoClass = UVictoryCore::BPLoadClass(cryo);

		if (!PreventCryo && cryoClass.uClass != nullptr && ArkShop::config["General"].value("GiveDinosInCryopods", false))
		{
			UPrimalItem* item = UPrimalItem::AddNewItem(cryoClass, nullptr, false, false, 0, false, 0, false, 0, false, nullptr, 0, false, false, true, false, false, false, AsaApi::GetApiUtils().GetWorld());
			if (item)
			{
				if (ArkShop::config["General"].value("CryoLimitedTime", false))
					item->AddItemDurability((item->ItemDurabilityField() - 3600) * -1, false);

				FCustomItemData customItemData = GetDinoCustomItemData(dino, saddle);
				item->SetCustomItemData(&customItemData);
				item->UpdatedItem(true, false);

				if (player_controller->GetPlayerInventory())
				{
					UPrimalItem* item2 = player_controller->GetPlayerInventory()->AddItemObject(item);

					if (item2)
						success = true;
				}
			}

			dino->Destroy(true, false);
		}
		else
			success = true;
	}

	return success;
}

bool ArkShop::ShouldPreventStoreUse(AShooterPlayerController* player_controller)
{
	bool preventBuying = false;

	if (player_controller && player_controller->GetPlayerCharacter())
	{
		AShooterCharacter* character = player_controller->GetPlayerCharacter();

		//Noglin Buff Cache Controlling Player
		if (config["General"].value("PreventUseNoglin", true) && !NoglinBuffClass.Get(false))
		{
			try
			{
				FString buffClassString = "Blueprint'/Game/Genesis2/Dinos/BrainSlug/Buff_BrainSlugPostProccess.Buff_BrainSlugPostProccess'";
				TSubclassOf<UObject> archetype;
				UVictoryCore::StringReferenceToClass(&archetype, &buffClassString);
				NoglinBuffClass = GetWeakReference(archetype.uClass);
			}
			catch (const std::exception& error)
			{
				Log::GetLog()->error(error.what());
			}
		}

		////Noglin Buff Cache Controlled Player
		if (config["General"].value("PreventUseNoglin", true) && !NoglinBuffClass2.Get(false))
		{
			try
			{
				FString buffClassString = "Blueprint'/Game/Genesis2/Dinos/BrainSlug/Buff_BrainSlug_HumanControl.Buff_BrainSlug_HumanControl'";
				TSubclassOf<UObject> archetype;
				UVictoryCore::StringReferenceToClass(&archetype, &buffClassString);
				NoglinBuffClass2 = GetWeakReference(archetype.uClass);
			}
			catch (const std::exception& error)
			{
				Log::GetLog()->error(error.what());
			}
		}

		////Noglin Buff Cache Controlled Dino
		if (config["General"].value("PreventUseNoglin", true) && !NoglinBuffClass3.Get(false))
		{
			try
			{
				FString buffClassString = "Blueprint'/Game/Genesis2/Dinos/BrainSlug/Buff_BrainSlugControl.Buff_BrainSlugControl'";
				TSubclassOf<UObject> archetype;
				UVictoryCore::StringReferenceToClass(&archetype, &buffClassString);
				NoglinBuffClass3 = GetWeakReference(archetype.uClass);
			}
			catch (const std::exception& error)
			{
				Log::GetLog()->error(error.what());
			}
		}

		if (!preventBuying && config["General"].value("PreventUseNoglin", true) && (character->HasBuff(NoglinBuffClass.Get(false), true) || character->HasBuff(NoglinBuffClass2.Get(false), true) || character->HasBuff(NoglinBuffClass3.Get(false), true)))
			preventBuying = true;

		if (!preventBuying && config["General"].value("PreventUseUnconscious", true) && character->bIsSleeping()())
			preventBuying = true;

		if (!preventBuying && config["General"].value("PreventUseHandcuffed", true) && character->CurrentWeaponField() && character->CurrentWeaponField()->AssociatedPrimalItemField())
		{
			FString WeaponName;
			character->CurrentWeaponField()->AssociatedPrimalItemField()->GetItemName(&WeaponName, false, true, nullptr);
			if (WeaponName.Contains(L"Handcuffs"))
				preventBuying = true;
		}

		if (!preventBuying && config["General"].value("PreventUseCarried", true) && character->bIsCarried()())
			preventBuying = true;
	}

	return preventBuying;
}

bool Hook_AShooterGameMode_HandleNewPlayer(AShooterGameMode* _this, AShooterPlayerController* new_player,
	UPrimalPlayerData* player_data, AShooterCharacter* player_character,
	bool is_from_login)
{
	const FString& eos_id = AsaApi::IApiUtils::GetEOSIDFromController(new_player);

	if (!ArkShop::DBHelper::IsPlayerExists(eos_id))
	{
		const bool is_added = ArkShop::database->TryAddNewPlayer(eos_id);
		if (!is_added)
		{
			return AShooterGameMode_HandleNewPlayer_original(_this, new_player, player_data, player_character,
				is_from_login);
		}
	}

	const bool rewards_enabled = ArkShop::config["General"]["TimedPointsReward"]["Enabled"];
	if (rewards_enabled)
	{
		const int interval = ArkShop::config["General"]["TimedPointsReward"]["Interval"];

		ArkShop::TimedRewards::Get().AddTask(
			FString::Format("Points_{}", eos_id.ToString()), eos_id, [eos_id]()
			{
				auto groups_map = ArkShop::config["General"]["TimedPointsReward"]["Groups"];
				const bool stack_rewards = ArkShop::config["General"]["TimedPointsReward"].value("StackRewards", false);
				auto player_groups = Permissions::GetPlayerGroups(eos_id);

				int high_points_amount = 0;
				for (auto group_iter = groups_map.begin(); group_iter != groups_map.end(); ++group_iter)
				{
					const FString group_name(group_iter.key().c_str());
					if (player_groups.Contains(group_name))
					{
						int points_amount = group_iter.value().value("Amount", 0);

						if (stack_rewards)
							high_points_amount += points_amount;
						else
						{
							if (points_amount > high_points_amount)
								high_points_amount = points_amount;
						}
					}
				}

				if (high_points_amount <= 0 && !ArkShop::config.value("General", nlohmann::json::object()).value("TimedPointsReward", nlohmann::json::object()).value("AlwaysSendNotifications", false))
					return;

				ArkShop::Points::AddPoints(high_points_amount, eos_id);
			},
			interval);
	}
	return AShooterGameMode_HandleNewPlayer_original(_this, new_player, player_data, player_character, is_from_login);
}

void Hook_AShooterGameMode_Logout(AShooterGameMode* _this, AController* exiting)
{
	// Remove player from the online list

	const FString& eos_id = AsaApi::IApiUtils::GetEOSIDFromController(exiting);

	ArkShop::TimedRewards::Get().RemovePlayer(eos_id);

	AShooterGameMode_Logout_original(_this, exiting);
}

FString ArkShop::GetText(const std::string& str)
{
	return FString(AsaApi::Tools::Utf8Decode(config["Messages"].value(str, "No message")));
}

bool ArkShop::IsStoreEnabled(AShooterPlayerController* player_controller)
{
	if (!store_enabled)
	{
		AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *closed_store_reason);
		return false;
	}
	return true;
}

void ArkShop::ToogleStore(bool enabled, const FString& reason)
{
	store_enabled = enabled;
	closed_store_reason = reason;
}

void ReadConfig()
{
	const std::string config_path = AsaApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/ArkShop/config.json";
	std::ifstream file{ config_path };
	if (!file.is_open())
	{
		throw std::runtime_error("Can't open config.json");
	}

	file >> ArkShop::config;

	file.close();

	if (EnsureDinoTraitsConfig(ArkShop::config))
	{
		std::ofstream output{ config_path };
		if (!output.is_open())
		{
			throw std::runtime_error("Can't update config.json");
		}

		output << ArkShop::config.dump(2);
		output.close();
	}
}

void ReloadConfig(APlayerController* player_controller, FString* /*unused*/, bool /*unused*/)
{
	auto* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

	try
	{
		ReadConfig();

		if (AsaApi::Tools::IsPluginLoaded("ArkShopUI"))
			ArkShopUI::Reload();
	}
	catch (const std::exception& error)
	{
		AsaApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Failed to reload config");

		Log::GetLog()->error(error.what());
		return;
	}

	AsaApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Reloaded config");
}

void ReloadConfigRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld* /*unused*/)
{
	FString reply;

	try
	{
		ReadConfig();

		if (AsaApi::Tools::IsPluginLoaded("ArkShopUI"))
			ArkShopUI::Reload();
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());

		reply = error.what();
		rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
		return;
	}

	reply = "Reloaded config";
	rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
}

void ShowHelp(AShooterPlayerController* player_controller, FString* /*unused*/, int, int)
{
	const FString help = ArkShop::GetText("HelpMessage");
	if (help != AsaApi::Tools::Utf8Decode("No message").c_str())
	{
		const float display_time = ArkShop::config["General"].value("ShopDisplayTime", 15.0f);
		const float text_size = ArkShop::config["General"].value("ShopTextSize", 1.3f);
		AsaApi::GetApiUtils().SendNotification(player_controller, FColorList::Green, text_size, display_time, nullptr,
			*help);
	}
}

void Load()
{
	Log::Get().Init("ArkShop");
	AsaApi::GetApiUtils().SetMessagingManager<AsaApiUtilsMessagingManager>();
	//AsaApi::GetApiUtils().SetMessagingManager<MessagingManager>();

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
	}

	try
	{
		const auto& mysql_conf = ArkShop::config["Mysql"];

		const bool use_mysql = mysql_conf["UseMysql"];
		if (use_mysql)
		{
			ArkShop::database = std::make_unique<MySql>(
				mysql_conf.value("MysqlHost", ""),
				mysql_conf.value("MysqlUser", ""),
				mysql_conf.value("MysqlPass", ""),
				mysql_conf.value("MysqlDB", ""),
				mysql_conf.value("MysqlPlayersTable", "ArkShopPlayers"),
				mysql_conf.value("MysqlPort", 3306),
				mysql_conf.value("MysqlLogTable", "ArkShopLogTransactions"));
		}
		else
		{
			const std::string db_path = ArkShop::config["General"]["DbPathOverride"];
			ArkShop::database = std::make_unique<SqlLite>(db_path);
		}

		ArkShop::Points::Init();
		ArkShop::Store::Init();
		ArkShop::Kits::Init();
		ArkShop::StoreSell::Init();

		//Discord Functions
		const auto& discord_config = ArkShop::config["General"].value("Discord", nlohmann::json::object());

		ArkShop::discord_enabled = discord_config.value("Enabled", false);
		ArkShop::discord_sender_name = discord_config.value("SenderName", "");
		ArkShop::discord_webhook_url = discord_config.value("URL", "").c_str();

		const FString help = ArkShop::GetText("HelpCmd");
		if (help != AsaApi::Tools::Utf8Decode("No message").c_str())
		{
			AsaApi::GetCommands().AddChatCommand(help, &ShowHelp);
		}

		AsaApi::GetHooks().SetHook("AShooterGameMode.HandleNewPlayer_Implementation(AShooterPlayerController*,UPrimalPlayerData*,AShooterCharacter*,bool)", &Hook_AShooterGameMode_HandleNewPlayer, &AShooterGameMode_HandleNewPlayer_original);
		AsaApi::GetHooks().SetHook("AShooterGameMode.Logout(AController*)", &Hook_AShooterGameMode_Logout, &AShooterGameMode_Logout_original);

		AsaApi::GetCommands().AddConsoleCommand("ArkShop.Reload", &ReloadConfig);
		AsaApi::GetCommands().AddRconCommand("ArkShop.Reload", &ReloadConfigRcon);
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
	}
}

void Unload()
{
	const FString help = ArkShop::GetText("HelpCmd");
	if (help != AsaApi::Tools::Utf8Decode("No message").c_str())
	{
		AsaApi::GetCommands().RemoveChatCommand(help);
	}

	AsaApi::GetHooks().DisableHook("AShooterGameMode.HandleNewPlayer_Implementation(AShooterPlayerController*,UPrimalPlayerData*,AShooterCharacter*,bool)", &Hook_AShooterGameMode_HandleNewPlayer);
	AsaApi::GetHooks().DisableHook("AShooterGameMode.Logout(AController*)", &Hook_AShooterGameMode_Logout);

	AsaApi::GetCommands().RemoveConsoleCommand("ArkShop.Reload");
	AsaApi::GetCommands().RemoveRconCommand("ArkShop.Reload");

	ArkShop::Points::Unload();
	ArkShop::Store::Unload();
	ArkShop::Kits::Unload();

	ArkShop::StoreSell::Unload();
}

extern "C" __declspec(dllexport) void Plugin_Unload()
{
	// Stop threads here
	AsaApi::GetCommands().RemoveOnTimerCallback("RewardTimer");
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD ul_reason_for_call, LPVOID /*lpReserved*/)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Load();
		break;
	case DLL_PROCESS_DETACH:
		Unload();
		break;
	}
	return TRUE;
}

#pragma once

#include "API/ARK/Ark.h"

namespace ArkShop
{
	FORCEINLINE TArray<float> GetStatPoints(APrimalDinoCharacter* dino)
	{
		TArray<float> floats;
		UPrimalCharacterStatusComponent* comp = dino->GetCharacterStatusComponent();
		int NumEntries = EPrimalCharacterStatusValue::MAX - 1;
		for (int i = 0; i < NumEntries; i++)
			floats.Add(
				comp->NumberOfLevelUpPointsAppliedField()()[(EPrimalCharacterStatusValue::Type)i]
				+
				comp->NumberOfMutationsAppliedTamedField()()[(EPrimalCharacterStatusValue::Type)i]
			);

		for (int i = 0; i < NumEntries; i++)
			floats.Add(comp->NumberOfLevelUpPointsAppliedTamedField()()[(EPrimalCharacterStatusValue::Type)i]);

		return floats;
	}

	FORCEINLINE TArray<float> GetCharacterStatsAsFloats(APrimalDinoCharacter* dino)
	{
		TArray<float> floats;
		UPrimalCharacterStatusComponent* comp = dino->GetCharacterStatusComponent();
		int NumEntries = EPrimalCharacterStatusValue::MAX - 1;
		for (int i = 0; i < NumEntries; i++)
			floats.Add(comp->CurrentStatusValuesField()()[(EPrimalCharacterStatusValue::Type)i]);

		for (int i = 0; i < NumEntries; i++)
			floats.Add(comp->MaxStatusValuesField()()[(EPrimalCharacterStatusValue::Type)i]);

		for (int i = 0; i < NumEntries; i++)
			floats.Add(comp->GetStatusValueRecoveryRate((EPrimalCharacterStatusValue::Type)i));

		floats.Add((float)dino->bIsFemale()());

		int len = floats.Num();

		floats.Append(GetStatPoints(dino));
		floats.Add(len);
		return floats;
	}

	FORCEINLINE TArray<FString> GetSaddleData(UPrimalItem* saddle)
	{
		TArray<FString> data;
		TArray<FString> colors;

		data.Add("");
		data.Add("");
		data.Add("");
		colors.Add("0");
		colors.Add("0");
		colors.Add("0");

		if (!saddle)
		{
			data.Append(colors);
			return data;
		}

		const float modifier = saddle->GetItemStatModifier(EPrimalItemStat::Armor);
		data[0] = FString::FromInt(FMath::Floor(modifier));

		FLinearColor c;
		UPrimalGameData* pgd = AsaApi::GetApiUtils().GetGameData();
		auto index = saddle->ItemQualityIndexField();

		auto entry = pgd->ItemQualityDefinitionsField()[index];
		c = entry.QualityColorField();

		colors[0] = FString(std::to_string(c.R));
		colors[1] = FString(std::to_string(c.G));
		colors[2] = FString(std::to_string(c.B));
		FString str = entry.QualityNameField();

		data[1] = FString::Format("{} {}", API::Tools::Utf8Encode(*str), API::Tools::Utf8Encode(*saddle->DescriptiveNameBaseField()));
		data[2] = AsaApi::IApiUtils::GetBlueprint(saddle);

		data.Append(colors);
		return data;
	}

	FORCEINLINE TArray<FString> GetDinoDataStrings(APrimalDinoCharacter* dino, const FString& dinoNameInMAP, const FString& dinoName, UPrimalItem* saddle)
	{
		TArray<FString> strings;
		strings.Add(dinoNameInMAP); //0
		strings.Add(dinoName); //1

		FString tmp;
		dino->GetColorSetInidcesAsString(&tmp);
		strings.Add(tmp); //2

		strings.Add(dino->bNeutered()() ? "NEUTERED" : ""); //3

		tmp = "";
		if (dino->bUsesGender()())
			if (dino->bIsFemale()())
				tmp = "FEMALE";
			else
				tmp = "MALE";
		strings.Add(tmp); //4
		strings.Add(""); // 5 empty

		TArray<APrimalBuff*> buffs;
		dino->GetBuffs(&buffs);
		if (buffs.Num() > 0)
		{
			int buffsbitmask = AsaApi::GetApiUtils().GetGameData()->GetBitmaskForBuffs(&buffs);
			tmp = FString(std::to_string(buffsbitmask));
			strings.Add(tmp); // 6 - should get bitmasks for buffs but doesn't seem to get used
		}
		else
			strings.Add("0"); // 6 should get bitmasks for buffs but doesn't seem to get used

		auto matingtime = UVictoryCore::PersistentToUtcTime(dino, dino->NextAllowedMatingTimeField());
		tmp = FString(std::to_string(matingtime));
		strings.Add(tmp); // 7 next allowed mating time

		strings.Add(dino->ImprinterNameField()); // 8 imprinter name

		//strings.Add(dino->DinoNameTagField().ToString()); // 9 dino name tag
		//FString dinoNameTag(dino->DinoNameTagField().ToString());
		//strings.Add(dinoNameTag); // 9 dino name tag
		//strings.Add("Carno");  // 9 dino name tag
		strings.Add("");  // 9 dino name tag
		
		FString preventRegions = "";
		for (int i = 0; i <= 5; i++)
		{
			if (dino->GetUsesColorizationRegion(i))
				preventRegions.Append(L"0"); //allows region to be colored
			else
				preventRegions.Append(L"1"); //does not allow region to be colored
		}
		strings.Add(preventRegions); // 10 prevent color regions
		return strings;
	}
} // namespace ArkShop
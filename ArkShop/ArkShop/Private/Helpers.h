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
		strings.Add(dinoNameInMAP);
		strings.Add(dinoName);

		FString tmp;
		dino->GetColorSetInidcesAsString(&tmp);
		strings.Add(tmp);

		strings.Add(dino->bNeutered()() ? "NEUTERED" : "");

		tmp = "";
		if (dino->bUsesGender()())
			if (dino->bIsFemale()())
				tmp = "FEMALE";
			else
				tmp = "MALE";
		strings.Add(tmp);
		strings.Add(""); // empty
		strings.Add("0"); // should get bitmasks for buffs but doesn't seem to get used

		//strings.Append(GetSaddleData(saddle)); 
		//bypassing saddle data for now
		strings.Add("");
		strings.Add("");
		strings.Add("");
		strings.Add("0");
		strings.Add("0");
		strings.Add("0");

		strings.Add(""); // extra data like harvest levels - not needed
		tmp = "";
		dino->GetCurrentDinoName(&tmp, nullptr);
		strings.Add(tmp);
		return strings;
	}
} // namespace ArkShop
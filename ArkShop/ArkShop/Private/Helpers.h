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

	FORCEINLINE TArray<FString> GetDinoDataStrings(APrimalDinoCharacter* dino, const FString& dinoNameInMAP, const FString& dinoName, UPrimalItem* saddle)
	{
		TArray<FString> strings;
		strings.Reserve(11);
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
		strings.Add("");
		strings.Add("");
		if (!dino->DinoNameTagField().IsNone())
			strings.Add(dino->DinoNameTagField().ToString());
		else
			strings.Add("");
		strings.Add("");

		return strings;
	}
} // namespace ArkShop
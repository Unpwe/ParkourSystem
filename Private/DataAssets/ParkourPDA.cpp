// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAssets/ParkourPDA.h"

FParkourDataStruct UParkourPDA::GetParkourDataAsset(ECharacterState CharacterState)
{
	if (!ParkourDataAssets.Find(CharacterState))
	{
		int32 Index = FMath::RandRange(0, ParkourDataAssets[ECharacterState::Idle].ParkourDataList.Num()-1);
		return ParkourDataAssets[ECharacterState::Idle][Index];
	}
	else
	{
		int32 Index = FMath::RandRange(0, ParkourDataAssets[CharacterState].ParkourDataList.Num()-1);
		return ParkourDataAssets[CharacterState][Index];
	}	
}

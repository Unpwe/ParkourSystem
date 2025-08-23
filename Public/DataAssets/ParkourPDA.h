// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ParkourSystem.h"
#include "Engine/DataAsset.h"
#include "FunctionLibrary/PSFunctionLibrary.h"
#include "ParkourPDA.generated.h"

using namespace ParkourGameplayTags;
class UGameplayTagsManager;


UENUM(BlueprintType)
enum class ECharacterState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Walking UMETA(DisplayName = "Walking"),
	Sprint UMETA(DisplayName = "Sprint")
};


USTRUCT(Atomic, BlueprintType)
struct FParkourDataStruct
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour Setting")
	class UAnimMontage* ParkourMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour Setting")
	FGameplayTag ParkourInState;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour Setting")
	FGameplayTag ParkourOutState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour Setting")
	float WarpTopXOffset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour Setting")
	float WarpTopZOffset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour Setting")
	float WarpDepthXOffset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour Setting")
	float WarpDepthZOffset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour Setting")
	float WarpVaultXOffset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour Setting")
	float WarpVaultZOffset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour Setting")
	float WarpMantleXOffset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour Setting")
	float WarpMantleZOffset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour Setting")
	float WarpTopXOffset_Temp;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour Setting")
	float WarpTopZOffset_Temp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour Setting")
	float MontageStartPos;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour Setting")
	float FallingMontageStartPos;

public:
	FParkourDataStruct()
	{
		ParkourMontage = nullptr;

		WarpTopXOffset = 0.f;
		WarpTopZOffset = 0.f;
		WarpDepthXOffset = 0.f;
		WarpDepthZOffset = 0.f;
		WarpVaultXOffset = 0.f;
		WarpVaultZOffset = 0.f;
		WarpMantleXOffset = 0.f;
		WarpMantleZOffset = 0.f;
		WarpTopXOffset_Temp = 0.f;
		WarpTopZOffset_Temp = 0.f;
		MontageStartPos = 0.f;
		FallingMontageStartPos = 0.f;
	}

	FParkourDataStruct(const FParkourDataStruct& Other)
	{
		ParkourMontage = Other.ParkourMontage;
		ParkourInState = Other.ParkourInState;
		ParkourOutState = Other.ParkourOutState;

		WarpTopXOffset = Other.WarpTopXOffset;
		WarpTopZOffset = Other.WarpTopZOffset;
		WarpDepthXOffset = Other.WarpDepthXOffset;
		WarpDepthZOffset = Other.WarpDepthZOffset;
		WarpVaultXOffset = Other.WarpVaultXOffset;
		WarpVaultZOffset = Other.WarpVaultZOffset;
		WarpMantleXOffset = Other.WarpMantleXOffset;
		WarpMantleZOffset = Other.WarpMantleZOffset;
		WarpTopXOffset_Temp = Other.WarpTopXOffset_Temp;
		WarpTopZOffset_Temp = Other.WarpTopZOffset_Temp;

		MontageStartPos = Other.MontageStartPos;
		FallingMontageStartPos = Other.FallingMontageStartPos;
	}

	FParkourDataStruct& operator=(const FParkourDataStruct& Other)
	{
		if (this != &Other)
		{
			ParkourMontage = Other.ParkourMontage;
			ParkourInState = Other.ParkourInState;
			ParkourOutState = Other.ParkourOutState;

			WarpTopXOffset = Other.WarpTopXOffset;
			WarpTopZOffset = Other.WarpTopZOffset;
			WarpDepthXOffset = Other.WarpDepthXOffset;
			WarpDepthZOffset = Other.WarpDepthZOffset;
			WarpVaultXOffset = Other.WarpVaultXOffset;
			WarpVaultZOffset = Other.WarpVaultZOffset;
			WarpMantleXOffset = Other.WarpMantleXOffset;
			WarpMantleZOffset = Other.WarpMantleZOffset;
			WarpTopXOffset_Temp = Other.WarpTopXOffset_Temp;
			WarpTopZOffset_Temp = Other.WarpTopZOffset_Temp;

			MontageStartPos = Other.MontageStartPos;
			FallingMontageStartPos = Other.FallingMontageStartPos;
		}
		return *this;
	}

};


USTRUCT(Atomic, BlueprintType)
struct FParkourDataAsset
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour Data List")
	TArray<FParkourDataStruct> ParkourDataList;

	FParkourDataStruct operator[](int Index)
	{
		if (ParkourDataList.IsValidIndex(Index))
			return ParkourDataList[Index];
		else
		{
			UPSFunctionLibrary::CrashLog(TEXT("ParkourDataList Is Null"), TEXT("ParkourDataList Is Null"));
			return FParkourDataStruct();
		}
	}
};



UCLASS(BlueprintType, Blueprintable)
class PARKOURSYSTEM_API UParkourPDA : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour Data Assets")
	TMap<ECharacterState, FParkourDataAsset> ParkourDataAssets;

public:
	FParkourDataStruct GetParkourDataAsset(ECharacterState CharacterState);

};

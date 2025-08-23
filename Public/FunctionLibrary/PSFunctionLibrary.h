// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ParkourSystem.h"
#include "UObject/NoExportTypes.h"
#include "PSFunctionLibrary.generated.h"

/**
 * 
 */

const int32 NONE = 0;
const int32 LEFT = -1;
const int32 RIGHT = 1;
const int32 FORWARD = 10;
const int32 BACKWARD = -10;
const int32 FORWARD_RIGHT = 11;
const int32 FORWARD_LEFT = 9;
const int32 BACKWARD_RIGHT = -9;
const int32 BACKWARD_LEFT = -11;


UCLASS()
class PARKOURSYSTEM_API UPSFunctionLibrary : public UObject
{
	GENERATED_BODY()

public:
	/*--------------------------
			GetGameplayTag
	----------------------------*/
	static FGameplayTag GetCheckGameplayTag(FName TagName); // Check & Get Tag	
	static bool CompGameplayTagName(const FGameplayTag& GameplayTag, FName TagName); // Compare GameplayTag
	static bool CompGameplayTagName(const FName& GameplayTagName, FName TagName); // Compare GameplayTag
	static FGameplayTag GetGameplayTag(FName TagName); // Find GameplayTag
	static FName GetGameplayTagName(const FGameplayTag& GameplayTag);	// Get GameplatTag FName


	/*--------------------------------------------
			Select function according to Tag
	----------------------------------------------*/
	// ParkourStateTag에 맞는 상태에 따라 float값 출력
	static float SelectParkourStateFloat(
		FGameplayTag ParkourStateTag, 
		float NotBusy, 
		float Valut, 
		float Mantle, 
		float Climb);
	static float SelectClimbStyleFloat(
		FGameplayTag ClimbStyleTag,
		float Braced,
		float FreeHang);
	static FGameplayTag SelectClimbDirection(bool bSelect); // True -> Parkour.Direction.Right / False -> Parkour.Direction.Left
	
	// Selct Direction
	static int32 FindDirection(FGameplayTag DirectionTag);
	static float SelectDirectionFloat(
		FGameplayTag DirectionTag,
		float Forward, float Backward, float Left, float Right,
		float ForwardLeft, float ForwardRight, float BackwardLeft, float BackwardRight);
	

	/*-----------------------------------
		Normalized Delta Rotator
	-------------------------------------*/
	static FRotator NormalizeDeltaRotator_Yaw(FVector NormalVector);

	/*-----------------------
		Revers Rotator
	-------------------------*/
	static FRotator ReversDeltaRotator(FRotator Rotator);


	/*------------------------------
			Convert to Tag
	--------------------------------*/
	static EParkourGameplayTagNames ConvertFNameToParkourTagsEnum(FName Name); //FName값을 EParkourGameplayTagNames으로  Convert
	

	/*------------------
			Debug
	-------------------*/
	/*
		FName으로 GameplayTag를 비교및 검사하기때문에 혹시 코드 작성중에 실수로 이름을 다르게 쓸수도 있기 때문에
		GameplayTag의 이름으로 이루어진 EParkourGameplayTagNames를 통해 이름 체크.
		또는 Enum Type에 해당 DisplayName이 있는지도 확인가능 
	*/
	template<typename T>
	static bool DebugEnumNameCheck(FName Name);
	static void CrashLog(FString UE_LOGText, FString CrashText); // LOG에서 서식문자를 쓸 경우 가변인자	
	static void CheckTagName(FName TagName);
};

template<typename T>
inline bool UPSFunctionLibrary::DebugEnumNameCheck(FName Name)
{
	UEnum* ParkourTypeEnum = StaticEnum<T>();

	// Enum의 모든 값을 순회하며 FName과 비교
	for (int32 i = 0; i < ParkourTypeEnum->NumEnums(); i++)
	{
		// DisplayName 가져오기
		FString DisplayName = ParkourTypeEnum->GetDisplayNameTextByIndex(i).ToString();
		if (FName(*DisplayName) == Name)
			return true;
	}

	CrashLog(TEXT("Can't find that Enum Name"), TEXT("Can't find that Enum Name"), true);
	return false;
}

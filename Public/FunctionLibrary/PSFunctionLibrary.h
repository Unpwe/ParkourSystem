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
	// ParkourStateTag�� �´� ���¿� ���� float�� ���
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
	static EParkourGameplayTagNames ConvertFNameToParkourTagsEnum(FName Name); //FName���� EParkourGameplayTagNames����  Convert
	

	/*------------------
			Debug
	-------------------*/
	/*
		FName���� GameplayTag�� �񱳹� �˻��ϱ⶧���� Ȥ�� �ڵ� �ۼ��߿� �Ǽ��� �̸��� �ٸ��� ������ �ֱ� ������
		GameplayTag�� �̸����� �̷���� EParkourGameplayTagNames�� ���� �̸� üũ.
		�Ǵ� Enum Type�� �ش� DisplayName�� �ִ����� Ȯ�ΰ��� 
	*/
	template<typename T>
	static bool DebugEnumNameCheck(FName Name);
	static void CrashLog(FString UE_LOGText, FString CrashText); // LOG���� ���Ĺ��ڸ� �� ��� ��������	
	static void CheckTagName(FName TagName);
};

template<typename T>
inline bool UPSFunctionLibrary::DebugEnumNameCheck(FName Name)
{
	UEnum* ParkourTypeEnum = StaticEnum<T>();

	// Enum�� ��� ���� ��ȸ�ϸ� FName�� ��
	for (int32 i = 0; i < ParkourTypeEnum->NumEnums(); i++)
	{
		// DisplayName ��������
		FString DisplayName = ParkourTypeEnum->GetDisplayNameTextByIndex(i).ToString();
		if (FName(*DisplayName) == Name)
			return true;
	}

	CrashLog(TEXT("Can't find that Enum Name"), TEXT("Can't find that Enum Name"), true);
	return false;
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "FunctionLibrary/PSFunctionLibrary.h"
#include "Kismet/KismetMathLibrary.h"

/*--------------------------
		GetGameplayTag
----------------------------*/
FGameplayTag UPSFunctionLibrary::GetCheckGameplayTag(FName TagName)
{
	CheckTagName(TagName);
	return FGameplayTag::RequestGameplayTag(TagName);
}

bool UPSFunctionLibrary::CompGameplayTagName(const FGameplayTag& GameplayTag, FName TagName)
{
	return GameplayTag == GetCheckGameplayTag(TagName);
}

bool UPSFunctionLibrary::CompGameplayTagName(const FName& GameplayTagName, FName TagName)
{
	return GetCheckGameplayTag(GameplayTagName) == GetCheckGameplayTag(TagName);
}


FGameplayTag UPSFunctionLibrary::GetGameplayTag(FName TagName)
{
	return GetCheckGameplayTag(TagName);
}

FName UPSFunctionLibrary::GetGameplayTagName(const FGameplayTag& GameplayTag)
{
	return GameplayTag.GetTagName();
}


/*--------------------------------------------
		Select function according to Tag
----------------------------------------------*/
float UPSFunctionLibrary::SelectParkourStateFloat(FGameplayTag ParkourStateTag, float NotBusy, float Vault, float Mantle, float Climb)
{
	if (CompGameplayTagName(ParkourStateTag, FName(TEXT("Parkour.State.NotBusy"))))
		return NotBusy;
	else if (CompGameplayTagName(ParkourStateTag, FName(TEXT("Parkour.State.Vault"))))
		return Vault;
	else if (CompGameplayTagName(ParkourStateTag, FName(TEXT("Parkour.State.Mantle"))))
		return Mantle;
	else if (CompGameplayTagName(ParkourStateTag, FName(TEXT("Parkour.State.Climb"))))
		return Climb;

	return 0.f;
}

float UPSFunctionLibrary::SelectClimbStyleFloat(FGameplayTag ClimbStyleTag, float Braced, float FreeHang)
{
	if (CompGameplayTagName(ClimbStyleTag, FName(TEXT("Parkour.ClimbStyle.Braced"))))
		return Braced;
	else if (CompGameplayTagName(ClimbStyleTag, FName(TEXT("Parkour.ClimbStyle.FreeHang"))))
		return FreeHang;

	return 0.f;
}

FGameplayTag UPSFunctionLibrary::SelectClimbDirection(bool bSelect)
{
	if (bSelect)
		return GetCheckGameplayTag(TEXT("Parkour.Direction.Right"));
	else
		return GetCheckGameplayTag(TEXT("Parkour.Direction.Left"));

}

int32 UPSFunctionLibrary::FindDirection(FGameplayTag DirectionTag)
{
	if (CompGameplayTagName(DirectionTag, TEXT("Parkour.Direction.Forward")))
		return FORWARD;
	else if (CompGameplayTagName(DirectionTag, TEXT("Parkour.Direction.Backward")))
		return BACKWARD;
	else if (CompGameplayTagName(DirectionTag, TEXT("Parkour.Direction.Left")))
		return LEFT;
	else if (CompGameplayTagName(DirectionTag, TEXT("Parkour.Direction.Right")))
		return RIGHT;
	else if (CompGameplayTagName(DirectionTag, TEXT("Parkour.Direction.ForwardLeft")))
		return FORWARD_LEFT;
	else if (CompGameplayTagName(DirectionTag, TEXT("Parkour.Direction.ForwardRight")))
		return FORWARD_RIGHT;
	else if (CompGameplayTagName(DirectionTag, TEXT("Parkour.Direction.BackwardLeft")))
		return BACKWARD_LEFT;
	else if (CompGameplayTagName(DirectionTag, TEXT("Parkour.Direction.BackwardRight")))
		return BACKWARD_RIGHT;

	return NONE;
}

float UPSFunctionLibrary::SelectDirectionFloat(FGameplayTag DirectionTag, float Forward, float Backward, float Left, float Right, float ForwardLeft, float ForwardRight, float BackwardLeft, float BackwardRight)
{
	CheckTagName(DirectionTag.GetTagName());
	
	int32 Direction = FindDirection(DirectionTag);


	switch (Direction)
	{
	case FORWARD:
		return Forward;
		break;
	case BACKWARD:
		return Backward;
		break;
	case LEFT:
		return Left;
		break;
	case RIGHT:
		return Right;
		break;
	case FORWARD_LEFT:
		return ForwardLeft;
		break;
	case FORWARD_RIGHT:
		return ForwardRight;
		break;
	case BACKWARD_RIGHT:
		return ForwardRight;
		break;
	case BACKWARD_LEFT:
		return BackwardLeft;
		break;
	}

	return NONE;
}



/*-----------------------------------
		Normalized Delta Rotator
-------------------------------------*/
FRotator UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(FVector NormalVector)
{
	FRotator DeltaRotator = UKismetMathLibrary::NormalizedDeltaRotator(NormalVector.Rotation(), FRotator(0.f, 180.f, 0.f));
	return FRotator(0.f, DeltaRotator.Yaw, 0.f);
}

FRotator UPSFunctionLibrary::ReversDeltaRotator(FRotator Rotator)
{
	return UKismetMathLibrary::NormalizedDeltaRotator(Rotator, FRotator(0.f, 180.f, 0.f));
}

/*------------------------------
		Convert to Tag
--------------------------------*/
EParkourGameplayTagNames UPSFunctionLibrary::ConvertFNameToParkourTagsEnum(FName Name)
{
	UEnum* ParkourTypeEnum = StaticEnum<EParkourGameplayTagNames>();

	// Enum의 모든 값을 순회하며 FName과 비교
	for (int32 i = 0; i < ParkourTypeEnum->NumEnums(); i++)
	{
		// DisplayName 가져오기
		FString DisplayName = ParkourTypeEnum->GetDisplayNameTextByIndex(i).ToString();
		if (FName(*DisplayName) == Name)
			return static_cast<EParkourGameplayTagNames>(i);
	}

	return static_cast<EParkourGameplayTagNames>(0);
}


/*------------------
		Debug
-------------------*/
void UPSFunctionLibrary::CrashLog(FString UE_LOGText, FString CrashText)
{
	LOG(Error, TEXT("%s"), *UE_LOGText);

	if(bCrash)
		check(false && *CrashText);
}



void UPSFunctionLibrary::CheckTagName(FName TagName)
{
	UEnum* ParkourTypeEnum = StaticEnum<EParkourGameplayTagNames>();

	// Enum의 모든 값을 순회하며 FName과 비교
	for (int32 i = 0; i < ParkourTypeEnum->NumEnums(); i++)
	{
		// DisplayName 가져오기
		FString DisplayName = ParkourTypeEnum->GetDisplayNameTextByIndex(i).ToString();
		if (FName(*DisplayName) == TagName)
			return;
	}

	LOG(Error, TEXT("Can't find that Tag Name : %s"), *TagName.ToString());

}

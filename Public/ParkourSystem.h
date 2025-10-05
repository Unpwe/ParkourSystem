// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "Modules/ModuleManager.h"


#define LOG_CALLINFO (FString(__FUNCTION__) + TEXT("(") + FString::FromInt(__LINE__) + TEXT(")"))
#define LOG(Verbosity, Format, ...) UE_LOG(LogTemp, Verbosity, TEXT("%s %s"), *LOG_CALLINFO, *FString::Printf(Format, ##__VA_ARGS__))


bool bCrash = false;


namespace ParkourGameplayTags
{
	/* Parkour.Action */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_NoAction);
	// Climb
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_Climb);
	//-------------------
	// Climb Hop
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_ClimbHopDown);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_ClimbHopLeft);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_ClimbHopLeftUp);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_ClimbHopRightUp);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_ClimbHopRight);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_ClimbHopUp);
	//-------------------
	// Climbing
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_ClimbingUp);
	//-------------------
	// Corner Move
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_CornerMove);
	//-------------------
	// Drop Down
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_DropDown);
	//-------------------
	// Falling
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_FallingBraced);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_FallingFreeHang);
	//-------------------
	// FreeHang
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_FreeClimbHopDown);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_FreeClimbHopLeft);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_FreeClimbHopRight);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_FreeHangClimb);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_FreeHangClimbUp);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_FreeHangDropDown);
	//-------------------
	// Vault
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_HighVault);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_LowMantle);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_Mantle);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_ThinVault);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_Vault);
	//-------------------
	// Multiplayer Assistance
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_AssistedClimb);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_ProvideAssist);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_RequestAssist);
	//-------------------
	/* Climb Style */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_ClimbStyle_Braced);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_ClimbStyle_FreeHang);
	//-------------------
	/* Parkour Direction */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Direction_Backward);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Direction_BackwardLeft);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Direction_BackwardRight);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Direction_Forward);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Direction_ForwardLeft);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Direction_ForwardRight);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Direction_Left);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Direction_Right);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Direction_NoDirection);
	//------------------
	/* Parkour State */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_State_Climb);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_State_Mantle);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_State_NotBusy);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_State_ReachLedge);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_State_Vault);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_State_WaitingForAssist);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_State_ProvidingAssist);
	//-------------------
	/* Multiplayer Interaction */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Interaction_ClimbAssist);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Interaction_Available);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Interaction_InProgress);
	//-------------------
}

UENUM(BlueprintType)
enum class EParkourGameplayTagNames : uint8
{
	/* Parkour.Action */
	Parkour_Action_NoAction UMETA(DisplayName = "Parkour.Action.NoAction"),
	// Climb
	Parkour_Action_Climb UMETA(DisplayName = "Parkour.Action.Climb"),
	// Climb Hop
	Parkour_Action_ClimbHopLeft UMETA(DisplayName = "Parkour.Action.ClimbHopLeft"),
	Parkour_Action_ClimbHopLeftUp UMETA(DisplayName = "Parkour.Action.ClimbHopLeftUp"),
	Parkour_Action_ClimbHopRightUp UMETA(DisplayName = "Parkour.Action.ClimbHopRightUp"),
	Parkour_Action_ClimbHopRight UMETA(DisplayName = "Parkour.Action.ClimbHopRight"),
	Parkour_Action_ClimbHopUp UMETA(DisplayName = "Parkour.Action.ClimbHopUp"),
	Parkour_Action_ClimbHopDown UMETA(DisplayName = "Parkour.Action.ClimbHopDown"),
	
	// Climbing
	Parkour_Action_ClimbingUp UMETA(DisplayName = "Parkour.Action.ClimbingUp"),
	
	// Corner Move
	Parkour_Action_CornerMove UMETA(DisplayName = "Parkour.Action.CornerMove"),
	
	// Drop Down
	Parkour_Action_DropDown UMETA(DisplayName = "Parkour.Action.DropDown"),
	
	// Falling
	Parkour_Action_FallingBraced UMETA(DisplayName = "Parkour.Action.FallingBraced"),
	Parkour_Action_FallingFreeHang UMETA(DisplayName = "Parkour.Action.FallingFreeHang"),
	
	// FreeHang
	Parkour_Action_FreeClimbHopDown UMETA(DisplayName = "Parkour.Action.FreeClimbHopDown"),
	Parkour_Action_FreeClimbHopLeft UMETA(DisplayName = "Parkour.Action.FreeClimbHopLeft"),
	Parkour_Action_FreeClimbHopRight UMETA(DisplayName = "Parkour.Action.FreeClimbHopRight"),
	Parkour_Action_FreeHangClimb UMETA(DisplayName = "Parkour.Action.FreeHangClimb"),
	Parkour_Action_FreeHangClimbUp UMETA(DisplayName = "Parkour.Action.FreeHangClimbUp"),
	Parkour_Action_FreeHangDropDown UMETA(DisplayName = "Parkour.Action.FreeHangDropDown"),
	
	// Vault
	Parkour_Action_HighVault UMETA(DisplayName = "Parkour.Action.HighVault"),
	Parkour_Action_LowMantle UMETA(DisplayName = "Parkour.Action.LowMantle"),
	Parkour_Action_Mantle UMETA(DisplayName = "Parkour.Action.Mantle"),
	Parkour_Action_ThinVault UMETA(DisplayName = "Parkour.Action.ThinVault"),
	Parkour_Action_Vault UMETA(DisplayName = "Parkour.Action.Vault"),

	// Multiplayer Assistance
	Parkour_Action_AssistedClimb UMETA(DisplayName = "Parkour.Action.AssistedClimb"),
	Parkour_Action_ProvideAssist UMETA(DisplayName = "Parkour.Action.ProvideAssist"),
	Parkour_Action_RequestAssist UMETA(DisplayName = "Parkour.Action.RequestAssist"),
	
	/* Climb Style */
	Parkour_ClimbStyle_Braced UMETA(DisplayName = "Parkour.ClimbStyle.Braced"),
	Parkour_ClimbStyle_FreeHang UMETA(DisplayName = "Parkour.ClimbStyle.FreeHang"),
	
	/* Parkour Direction */
	Parkour_Direction_Backward UMETA(DisplayName = "Parkour.Direction.Backward"),
	Parkour_Direction_BackwardLeft UMETA(DisplayName = "Parkour.Direction.BackwardLeft"),
	Parkour_Direction_BackwardRight UMETA(DisplayName = "Parkour.Direction.BackwardRight"),
	Parkour_Direction_Forward UMETA(DisplayName = "Parkour.Direction.Forward"),
	Parkour_Direction_ForwardLeft UMETA(DisplayName = "Parkour.Direction.ForwardLeft"),
	Parkour_Direction_ForwardRight UMETA(DisplayName = "Parkour.Direction.ForwardRight"),
	Parkour_Direction_Left UMETA(DisplayName = "Parkour.Direction.Left"),
	Parkour_Direction_Right UMETA(DisplayName = "Parkour.Direction.Right"),
	Parkour_Direction_NoDirection UMETA(DisplayName = "Parkour.Direction.NoDirection"),
	
	/* Parkour State */
	Parkour_State_Climb UMETA(DisplayName = "Parkour.State.Climb"),
	Parkour_State_Mantle UMETA(DisplayName = "Parkour.State.Mantle"),
	Parkour_State_NotBusy UMETA(DisplayName = "Parkour.State.NotBusy"),
	Parkour_State_ReachLedge UMETA(DisplayName = "Parkour.State.ReachLedge"),
	Parkour_State_Vault UMETA(DisplayName = "Parkour.State.Vault"),
	Parkour_State_WaitingForAssist UMETA(DisplayName = "Parkour.State.WaitingForAssist"),
	Parkour_State_ProvidingAssist UMETA(DisplayName = "Parkour.State.ProvidingAssist"),

	/* Multiplayer Interaction */
	Parkour_Interaction_ClimbAssist UMETA(DisplayName = "Parkour.Interaction.ClimbAssist"),
	Parkour_Interaction_Available UMETA(DisplayName = "Parkour.Interaction.Available"),
	Parkour_Interaction_InProgress UMETA(DisplayName = "Parkour.Interaction.InProgress")
};




class FParkourSystemModule : public IModuleInterface
{
public:

	//IModuleInterface implementation
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};



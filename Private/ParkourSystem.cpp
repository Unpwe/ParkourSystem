// Copyright Epic Games, Inc. All Rights Reserved.

#include "ParkourSystem.h"

namespace ParkourGameplayTags
{
	/* Parkour.Action */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_NoAction, "Parkour.Action.NoAction", "Parkour Action NoAction");
	// Climb
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_Climb, "Parkour.Action.Climb", "Parkour Action Climb");
	// Climb Hop
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_ClimbHopDown, "Parkour.Action.ClimbHopDown", "Parkour Action ClimbHopDown");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_ClimbHopLeft, "Parkour.Action.ClimbHopLeft", "Parkour Action ClimbHopLeft");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_ClimbHopLeftUp, "Parkour.Action.ClimbHopLeftUp", "Parkour Action ClimbHopLeftUp");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_ClimbHopRightUp, "Parkour.Action.ClimbHopRightUp", "Parkour Action ClimbHopRightUp");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_ClimbHopRight, "Parkour.Action.ClimbHopRight", "Parkour Action ClimbHopRight");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_ClimbHopUp, "Parkour.Action.ClimbHopUp", "Parkour Action ClimbHopUp");
	// Climbing
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_ClimbingUp, "Parkour.Action.ClimbingUp", "Parkour Action ClimbingUp");
	// Corner Move
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_CornerMove, "Parkour.Action.CornerMove", "Parkour Action CornerMove");
	// Drop Down
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_DropDown, "Parkour.Action.DropDown", "Parkour Action DropDown");
	// Falling
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_FallingBraced, "Parkour.Action.FallingBraced", "Parkour Action FallingBraced");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_FallingFreeHang, "Parkour.Action.FallingFreeHang", "Parkour Action FallingFreeHang");
	// FreeHang
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_FreeClimbHopDown, "Parkour.Action.FreeClimbHopDown", "Parkour Action FreeClimbHopDown");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_FreeClimbHopLeft, "Parkour.Action.FreeClimbHopLeft", "Parkour Action FreeClimbHopLeft");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_FreeClimbHopRight, "Parkour.Action.FreeClimbHopRight", "Parkour Action FreeClimbHopRight");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_FreeHangClimb, "Parkour.Action.FreeHangClimb", "Parkour Action FreeHangClimb");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_FreeHangClimbUp, "Parkour.Action.FreeHangClimbUp", "Parkour Action FreeHangClimbUp");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_FreeHangDropDown, "Parkour.Action.FreeHangDropDown", "Parkour Action FreeHangDropDown");
	// Vault
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_HighVault, "Parkour.Action.HighVault", "Parkour Action HighVault");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_LowMantle, "Parkour.Action.LowMantle", "Parkour Action LowMantle");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_Mantle, "Parkour.Action.Mantle", "Parkour Action Mantle");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_ThinVault, "Parkour.Action.ThinVault", "Parkour Action ThinVault");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_Vault, "Parkour.Action.Vault", "Parkour Action Vault");
	
	/* Climb Style */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_ClimbStyle_Braced, "Parkour.ClimbStyle.Braced", "Parkour ClimbStyle Braced");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_ClimbStyle_FreeHang, "Parkour.ClimbStyle.FreeHang", "Parkour ClimbStyle FreeHang");
	
	/* Parkour Direction */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Direction_Backward, "Parkour.Direction.Backward", "Parkour Direction Backward");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Direction_BackwardLeft, "Parkour.Direction.BackwardLeft", "Parkour Direction BackwardLeft");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Direction_BackwardRight, "Parkour.Direction.BackwardRight", "Parkour Direction BackwardRight");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Direction_Forward, "Parkour.Direction.Forward", "Parkour Direction Forward");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Direction_ForwardLeft, "Parkour.Direction.ForwardLeft", "Parkour Direction ForwardLeft");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Direction_ForwardRight, "Parkour.Direction.ForwardRight", "Parkour Direction ForwardRight");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Direction_Left, "Parkour.Direction.Left", "Parkour Direction Left");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Direction_Right, "Parkour.Direction.Right", "Parkour Direction Right");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Direction_NoDirection, "Parkour.Direction.NoDirection", "Parkour Direction NoDirection");
	
	/* Parkour State */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_State_Climb, "Parkour.State.Climb", "Parkour State Climb");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_State_Mantle, "Parkour.State.Mantle", "Parkour State Mantle");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_State_NotBusy, "Parkour.State.NotBusy", "Parkour State NotBusy");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_State_ReachLedge, "Parkour.State.ReachLedge", "Parkour State ReachLedge");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_State_Vault, "Parkour.State.Vault", "Parkour State Vault");
}

//#define LOCTEXT_NAMESPACE "FParkourSystemModule"

//void FParkourSystemModule::StartupModule()
//{
//	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
//}
//
//void FParkourSystemModule::ShutdownModule()
//{
//	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
//	// we call this function before unloading the module.
//}

//#undef LOCTEXT_NAMESPACE
	
//IMPLEMENT_MODULE(FParkourSystemModule, ParkourSystem)

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, ParkourSystem, "ParkourSystem")


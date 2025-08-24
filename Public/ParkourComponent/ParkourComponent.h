// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#define DEBUG_PARKOURCOMPONENT
//#define DEBUG_TICK
//#define DEBUG_MOVEMENT
//#define DEBUG_IK


#include "ParkourSystem.h"
#include "Components/ActorComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/DataAsset.h"
#include "DataAssets/ParkourPDA.h"
#include "GameFramework/Actor.h"
#include "Engine/EngineTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "ParkourComponent.generated.h"


using namespace ParkourGameplayTags;

USTRUCT(BlueprintType)
struct FModifierPair
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Min Modifier")
	float SameDirection = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Min Modifier")
	float DifferenceDirection = 0.f;

	FModifierPair()
	{
	}

	FModifierPair(float NewSameDirection, float NewDifferenceDirection)
	{
		SameDirection = NewSameDirection;
		DifferenceDirection = NewDifferenceDirection;
	}

};



UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PARKOURSYSTEM_API UParkourComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	/* Public */
public:
	UParkourComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


public:
	/*-------------------
			Public
	---------------------*/

	/*-------------------
		InitializeValus
	---------------------*/
	/* 사용자가 반드시 설정해야하는 함수  */	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus")
	float SprintTypeParkour_Speed = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus")
	float WalkingTypeParkour_Speed = 300.f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitializeValus")
	bool bAutoJump = false; // BP_AutoJump를 쓸 것인가?

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitializeValus")
	bool bAlwaysParkour = false; 	// Parkour을 Play Parkour이 아닌 Tick으로 판단 할 것인지

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus")
	bool bAutoClimb = false;



	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus")
	FVector CheckInGroundSize = FVector(30.f, 30.f, 30.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus")
	FVector CheckInGroundSize_Climb = FVector(30.f, 30.f, 10.f);

	// Climb Height
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus")
	float CanClimbHeight = 200.f;

	


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|Modifier")
	FModifierPair BracedLeftHandModifier = { 195.646f,  254.056f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|Modifier")
	FModifierPair BracedRightHandModifier = { 253.518f , 194.274f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|Modifier")
	FModifierPair FreeHangLeftHandModifier = { 225.799f, 225.764f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|Modifier")
	FModifierPair FreeHangRightHandModifier = {	225.806f, 225.768f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|Modifier")
	float ModifierHandMAXHeight = 10.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InitializeValus|Modifier")
	FName LeftHandModifierName = TEXT("LeftHandModifier");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InitializeValus|Modifier")
	FName RightHandModifierName = TEXT("RightHandModifier");


	
	/* 가장 처음 앞에 있는 벽을 Check 하거나 , Climb 상태일 때 현재 상태에서 캐릭터의 Z축을 기준으로 해당 수치만큼 뺀다. 
그 위치의 Z축부터 Trace를 진행한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|Custom Check Parkour Trace", meta = (ClampMax = 0))
	float CheckParkourFromCharacterRootZ = -60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|Custom Check Parkour Trace", meta = (ClampMin = 5, ClampMax = 20))
	float CheckParkourFallingHeightCnt = 15; // 떨어질 때 체크 할 Trace 높이 갯수

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|Custom Check Parkour Trace", meta = (ClampMin = 5, ClampMax = 15))
	float CheckParkourDepthCnt = 8; // 기본 상태 체크 할 Trace Depth 갯수 * 20.f을 한다.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|Custom Check Parkour Trace", meta = (ClampMin = 100, ClampMax = 300))
	int32 CheckParkourDistance = 200;


	/*---------------------------------------------------------------------------------------
		Vault를 기준으로 Parkour Action Type이 정해지기 때문에 높이 지정
	-----------------------------------------------------------------------------------------*/

	// Vault Height
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|Custom Check Parkour Trace|Vault Height", meta = (ClampMin = 10, ClampMax = 100))
	float CheckVaultMinHeight = 70.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|Custom Check Parkour Trace|Vault Height", meta = (ClampMin = 80, ClampMax = 180))
	float CheckVaultMaxHeight = 120.f;

	// High Vault
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|Custom Check Parkour Trace|High Vault Depth", meta = (ClampMin = 80, ClampMax = 180))
	float CheckHighVaultMaxHeight = 160.f;

	// Vault Depth
	/*UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|Custom Check Parkour Trace|Vault Depth", meta = (ClampMin = 0, ClampMax = 30))
	float CheckVaultMinDepth = 0.f;*/

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|Custom Check Parkour Trace|Vault Depth", meta = (ClampMin = 80, ClampMax = 220))
	float CheckVaultMaxDepth = 180.f;

	// Thin Vault
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|Custom Check Parkour Trace|Thin Vault Depth", meta = (ClampMin = 0, ClampMax = 30))
	float CheckThinVaultMinDepth = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|Custom Check Parkour Trace|Thin Vault Depth", meta = (ClampMin = 10, ClampMax = 30))
	float CheckThinVaultMaxDepth = 30.f;

	// Low
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|Custom Check Parkour Trace|Low Mantle", meta = (ClampMin = 10, ClampMax = 30))
	float CheckLowMantleMin = 44.f;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "InitializeValus|Custom Check Parkour Trace|Low Mantle", meta = (ClampMin = 10, ClampMax = 30))
	float CheckLowMantleMax = CheckVaultMinHeight;

	


	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "InitializeValus|ClimbMovement")
	FName ClimbMoveSpeedCurveName = "ClimbMoveSpeed";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitializeValus|ClimbMovement")
	float ClimbSpeedMaxValue = 40.f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitializeValus|ClimbMovement", meta = (ClampMin = 0.01f, ClampMax = 0.1f))
	float ClimbSpeedBraced = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitializeValus|ClimbMovement", meta = (ClampMin = 0.01f, ClampMax = 0.1f))
	float ClimbSpeedFreeHang = 0.1f;

	/* IK Hnad */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|IK|Hand", meta = (ClampMin = -50f, ClampMax = 20f))
	float ClimbIKHandSpace = -20.f; // Climb상태에서 손 사이의 공간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|IK|Hand")
	float DirectionMovementSameHand = 0.f; // 이동방향과 손의 방향이 일치하는 경우 추가적으로 반대손보다 더 앞 쪽을 계산하기 위함.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|IK|Hand")
	FName LeftIKHandSocketName = "ik_hand_l";
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|IK|Hand")
	FName RightIKHandSocketName = "ik_hand_r";
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitializeValus|IK|Hand")
	FName LeftHandSocketName = "hand_l";
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitializeValus|IK|Hand")
	FName RightHandSocketName = "hand_r";
	
	/*
		IK를 진행할 때 벽의 ImpactNormal 값을 이용하여 손의 Rotation을 진행하기 때문에,
		애니메이션마다 손의 회전 값이 다를 수가 있음으로 커스텀 하게 수동 설정
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitializeValus|IK|Hand")
	FRotator Additive_LeftClimbIKHandRotation = FRotator(90.f, 0.f, 270.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitializeValus|IK|Hand")
	FRotator Additive_RightClimbIKHandRotation = FRotator(270.f, 0.f, 270.f);


	/* ----- IK Foot ------ */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "InitializeValus|IK|Foot")
	FName LeftIKFootSocketName = "ik_foot_l";
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "InitializeValus|IK|Foot")
	FName RightIKFootSocketName = "ik_foot_r";

	/* 
		Hand 위치(모서리)에서 어느만큼 아래에 위치한 곳에 Foot을 둘 것인지 ?
		두 발을 따로 요구하는 이유는 두 발의 위치값을 다르게하여 보다 자연스러운 애니메이션 연출을 위함.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|IK|Foot")
	float LeftFootPositionToHand = -135.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|IK|Foot")
	float RightFootPositionToHand = -125.f;

	// Braced, FreeHang을 판단하고자 할 때 손에서부터 어느정도 위치한 곳을 검사할 것인지.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|IK|Foot")
	float CheckClimbBracedStyle = -125.f;

	// IK Foot의 위치 조정 (Forward/Backward)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|IK|Foot")
	float FootDepthOffset = -18.f;


	
	/* ------ Use Corner Hop ------- */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|Hop")
	bool bUseCornerHop = true; 

	/*--------------------------------
		InitializeValus|Data Asset
	----------------------------------*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitializeValus|DataAsset")
	TMap<EParkourGameplayTagNames, TSubclassOf<class UParkourPDA>> ParkourDataAssets;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "InitializeValus|DataAsset")
	FParkourDataStruct ParkourVariables;


	/*----------------------------------------
		InitializeValus|Camera Lerp Curve
	-----------------------------------------*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitializeValus|CameraLerpCurve")
	UCurveFloat* ParkourCameraLerpCurve;


	/*------------------------------------------
		InitializeValus|Arrow Actor Location
	-------------------------------------------*/
	// 해당 ArrowActor를 이용하여 Climb할때 WorldLocation값을 얻어온다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|ArrowActor")
	FVector2D ArrowLocation = FVector2D(0.f, 195.f);


	/*---------------------------
			DrawDebugTrace
	-----------------------------*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|CheckWallShape")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_ParkourCheckWallShape = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|CheckWallShape")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_ParkourCheckWallTopDepthShape = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|CheckWallShape")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_ClimbLedgeResultInspection = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|AutoClimb")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_CheckInGround = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|CheckParkour")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_CheckMantleSurface = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|CheckParkour")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_CheckClimbSurface = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|CheckParkour")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_CheckVaultSurface = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|CheckParkour")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_CheckClimbStyle = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|MotionWarping")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_FindWarpMantleLocation = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|DropDown")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_FindDropDownHangLocation = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|FirstTraceHeight")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_GetFirstTraceHeight = EDrawDebugTrace::None;

	/*---------------
		DDT | IK
	-----------------*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|ClimbIK")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_LeftClimbHandIK = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|ClimbIK")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_LeftClimbFootIK = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|ClimbIK")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_RightClimbHandIK = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|ClimbIK")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_RightClimbFootIK = EDrawDebugTrace::None;


	/*-------------------------
		DDT | Climb Movement
	---------------------------*/

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|ClimbMovement")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_ClimbMovementCheck = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|ClimbMovement")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_ClimbMovementHeightCheck = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|ClimbMovement")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_CheckClimbBracedStyleMoving = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|ClimbMovement")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_CheckClimbMovementSurface = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|ClimbMovement|CheckCorner")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_CheckOutCorner = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|ClimbMovement|CheckCorner")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_OutCornerMovement = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|ClimbMovement|CheckCorner")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_InCornerMovement = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|ClimbMovement|IK")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_ParkourHandIK = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|ClimbMovement|IK")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_ParkourFootIK = EDrawDebugTrace::None;

	/*--------------------------------
		DDT | Climb Movement | Hop
	---------------------------------*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|ClimbMovement|Hop|CheckHop")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_CheckFindHopLocation = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|ClimbMovement|Hop|CheckHop")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_CheckHopCapsuleComponent = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|ClimbMovement|Hop|CheckHop")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_CheckHopWallTopLocation = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|ClimbMovement|Hop|CheckCornerHop")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_CheckInCornerHop = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|ClimbMovement|Hop|CheckCornerHop")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_CheckOutCornerHop = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|ClimbMovement|Hop|CheckCornerHop")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_CheckFindCornerHop = EDrawDebugTrace::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DrawDebugTrace|ClimbMovement|Hop|CheckCornerHop")
	TEnumAsByte<EDrawDebugTrace::Type> DDT_CheckFirstHoldingWall = EDrawDebugTrace::None;

	/*-------------------------
			Check Surface
	--------------------------*/
	// InGround 상태를 판단하기 위해 캐릭터의 발 밑이 땅에 닿아있는지 확인하기위해 Z축을 얼마나 아래로 내릴 것인지 판단하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CheckParkour|CheckInGround", meta = (ClampMin = -30.f, ClampMax = 20.f))
	float CheckAutoClimbToRoot = -10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CheckParkour|CheckInGround", meta = (ClampMin = 0.f, ClampMax = 60.f))
	float CheckAutoClimbToRoot_Braced = 50.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CheckParkour|CheckInGround", meta = (ClampMin = 0.f, ClampMax = 60.f))
	float CheckAutoClimbToRoot_FreeHang = 30.f;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CheckParkour|CheckMantleSurface")
	float CheckMantleHeight = 8.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CheckParkour|CheckMantleSurface")
	float CheckMantleCapsuleTraceRadius = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CheckParkour|CheckClimbSurface")
	float CheckClimbCapsuleTraceForwardDistance = 55.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CheckParkour|CheckVaultSurface")
	float CheckVaultHeight = 18.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CheckParkour|CheckVaultSurface")
	float CheckVaultCapsuleTraceRadius = 25.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CheckParkour|CheckVaultSurface")
	float CheckVaultCapsuleTraceHalfHeight = 53.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CheckParkour|CheckVaultSurface")
	float VaultLandingHeight = 200.f;



	/*------------------------------------------
		Pakrour Vault Velocity Length
	--------------------------------------------*/
	// Mantle과 Vault 둘다 가능한 상태 일 때 해당 속도 이상이면 Vault.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PakrourVaultVelocityLength")
	float Velocity_VaultMantle = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PakrourVaultVelocityLength")
	float Velocity_MantleOrHighVault = 200.f;


	/*------------------------------------
		Check Climb Style Radius
	--------------------------------------*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CheckClimbStyle")
	float CheckClimbStyle_ZHeight = -125.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CheckClimbStyle")
	float CheckClimbStyleRadius = 10.f;


	/*---------------------------------------------------
		Find Warp Mantle Location Distance
	-----------------------------------------------------*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FindWarp MantleHeight")
	float CheckMantleHeight_StartZ = 40.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FindWarp MantleHeight")
	float CheckMantleHeight_EndZ = 60.f;


	/*-------------------------------------------
		Previous State Camera Setting
	---------------------------------------------*/
	// 파쿠르 상태 변경시 카메라 위치 및 TargetArmLength 연출
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PreviousState Setting")
	FVector ReachLedge_CameraLocation = FVector(-50.f, 0.f, 70.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PreviousState Setting")
	float ReachLedge_TargetArmLength = 500.f;


	/*---------------------------
		Parkour State Settings
	-----------------------------*/
	// ParkourStateSettings 함수에서 무브먼트의 회전속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourRotationRate")
	FRotator ParkourRotationRate_Climb = FRotator(0.f, 0.f, 0.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourRotationRate")
	FRotator ParkourRotationRate_Matntle = FRotator(0.f, 500.f, 0.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourRotationRate")
	FRotator ParkourRotationRate_Vault = FRotator(0.f, 500.f, 0.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourRotationRate")
	FRotator ParkourRotationRate_ReachLedge = FRotator(0.f, 500.f, 0.f);

	/*----------------------
			Climb IK
	------------------------*/
	/*
		Left Climb IK, Right Climb IK에서 맨 처음 Left, Right를 체크할 때
		얼만큼의 폭으로 검사할 것인지.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Width")
	float CheckClimbForward = 20.f;

	/* 캐릭터의 높이, 캐릭터의 손 높이를 수동적으로 조절 */
	// 이 값이 올라가면 수치 만큼 캐릭터가 파쿠르할 때 위로 올라감
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings")
	float CharacterHeightDiff = 0.f;


	/* ClimbIK| Custom Settings| Hand */
	
	// 이 값이 올라가면 수치 만큼 캐릭터가 팔이 위로 더 올라감
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings|Hand")
	float CharacterHandUpDiff = 0.f;
	
	//이 값이 올라가면 수치 만큼 캐릭터가 앞으로 팔을 더 뻗는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings|Hand")
	float BracedHandFrontDiff = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings|Hand")
	float FreeHangHandFrontDiff = 0.f;

	// Climb 상태에 따른 손 높이 추가 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings|Hand")
	float BracedHandMovementHeight = -7.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings|Hand")
	float FreeHangHandMovementHeight = -9.f;
	
	// 손 두께 만큼 거리 조절
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings|Hand")
	float CharacterHandThickness = 3.f;


	/* ClimbIK| Custom Settings| Foot */
	// Foot IK를 할 때 Foot IK할 위치를 캐릭터의 키값에 비례하여 커스텀하게 수정
	// ex) CharacterHeightDiff - CharacterFootIKLocation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings|Foot")
	float CharacterFootIKLocation = 150.f;

	// ClimbFootIK에서 각각 RightVector에 값을 추가로 빼거나 더하여 IK하는 발 위치를 수동적으로 수정 가능.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings|Foot")
	float CharacterLeftFootLocation = 7.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings|Foot")
	float CharacterFootLocation_Right = 9.f;

	// Foot IK를 할 때 애니메이션 및 발 두께를 고려하여 ImpactPoint에서 어느정도 떨어진 곳에 발을 고정 시킬 것 인지.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings|Foot")
	float BracedFootAddThickness_Left = 17.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings|Foot")
	float BracedFootAddThickness_Right = 17.f;


	/*-------------------
			Drop
	---------------------*/
	// Climb 상태에서 ParkourDrop 함수가 실행되었을때 bCanManuelClimb 변수를 true로 셋업 할 Timer 시간을 조정하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop")
	float Drop_CanManuelClimbBecomesTrueTime = 0.3f;
	// 캐릭터의 앞 쪽 발 밑을 검사할때 사용함. 캐릭터의 위치값에서 해당 값을 빼서 Trace를 진행
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop")
	float CheckCharacterDropHeight = -120.f;
	// DropDown을 실행할때 캐릭터 회전값을 사용할지, 컨트롤러 회전값을 사용할 것인지.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop")
	bool bUseControllerRotation_DropDown = false;

	/*----------------------------
		Climb Movement
	-----------------------------*/
	// Climb상태에서 움직일 때 컨트롤러의 상대적 방향에따라 움직일 것인지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|ClimbMovement")
	bool bUseControllerRotation_Climb = false;

	// ClimbMovement에서 이동할때 거리체크
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|ClimbMovement")
	float ClimbMovementDistance = 17.f;

	// ClimbMovement에서 이동할때 캐릭터의 이동방향에 장애물이 있는지 체크할 거리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|ClimbMovement|Obstacle")
	float ClimbMovementObstacleCheckDistance = 13.f;
	// 장애물을 체크할 Capsule Trace의 크기
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|ClimbMovement|Obstacle")
	float CheckClimbMovementSurface_CapsuleTraceRadius = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|ClimbMovement|Obstacle")
	float CheckClimbMovementSurface_CapsuleTraceHalfHeight = 82.f;


	/*
		ClimbMovement에서 움직일 때 최종적으로 연산을 끝낸 위치로 캐릭터를 움직이려고 할 때의
		현재 위치값에서 Target 위치으로 이동하는 보간 속도
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|ClimbMovement|Interp Speed")
	float ClimbStyleBracedInterpSpeed = 2.7f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|ClimbMovement|Interp Speed")
	float ClimbStyleFreeHangInterpSpeed = 1.8f;

	/*
	* CornerMove, ClimbMovement에서 SetActorLocationAndRoation이나 MoveComponentTo 함수를 호출 할 때 필요한 변수
	* 루트모션 몽타주 애니메이션을 실행하면서 애니메이션 스테이트 머신으로 돌아갈때,
	* 두 애니메이션이 세트가 아닌경우 루트 모션이 달라서 위치값이 틀어질 수가 있는데,
	* 알맞은 자리에 알맞게 애니메이션 상태를 위치 시키기 위해 수동적으로 변경하기위한 변수
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|ClimbMovement|Edit Location")
	float ClimbStyleBracedXYPosition = -40.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|ClimbMovement|Edit Location")
	float ClimbStyleBracedZPosition = -100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|ClimbMovement|Edit Location")
	float ClimbStyleFreeHangXYPosition = -20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|ClimbMovement|Edit Location")
	float ClimbStyleFreeHangZPosition = -107.f;




	/*----------------------------------
			Corner Movement
	------------------------------------*/
	/* Corner를 돌때 해당 부분의 위치로 Move Component To를 실행하는데,
	Trace하여 알아낸 모서리의 위치에서 어느만큼의 Height를 빼야 해당 위치에 Target Relative Location을 하면
	캐릭터가 알맞게 이동 할지 커스텀하게 조절 할수 있는 변수
	인간형 기준 약 100 ~ 107 (애니메이션마다 다를수 있음) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CornerMovement")
	float CornerMovementHeight = 105.f;

	/*
		코너를 돌때 Check해야하는 캐릭터의 CapsuleSize
		일일히 수동으로 적는 이유는, 캐릭터마나 Climb 애니메이션형태나 크기가 다를 수 있기 때문에
		해당 캐릭터의 크기 및 애니메이션 형태에 따라 알맞는 사이즈를 검사하기 위함이다.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CornerMovement")
	float CornerCheckCapsuleRadius_Subtraction = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CornerMovement")
	float CornerCheckCapsuleHalfHeight_Subtraction = 6.f;

	/*
		코너를 이동하는 함수를 실행할 때 내부적으로
		MoveComponentTo 함수의 OverTime 인자값으로 사용하는 변수.
		해당 값 만큼 애니메이션의 보간시간을 주어 부드럽게 움직인다.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CornerMovement|OverTime")
	float CornerMoveComponentToOverTime_Braced = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CornerMovement|OverTime")
	float CornerMoveComponentToOverTime_FreeHang = 0.3f;


	/*--------------
			Hop
	----------------*/
	/* 
		Character가 Hop을 진행할 때 Hop이 가능한 부분을 찾았을 시
		Character CapsuleComponent 만큼 검사를 하게 되는데,
		이때 검사할 CapsuleComponent의 크기를 조절 하는 변수
	*/

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hop")
	float HopCharacterCapsuleHalfHeight_Subtraction = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hop")
	float HopCharacterCapsuleRadius_Subtraction = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hop")
	float CheckHopCharacterCapsuleDepth_Subtraction = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hop|Dimensions")
	float CheckHopWidth = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hop|Dimensions")
	float CheckHopDepth = 60.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hop|Dimensions")
	float CheckHopHeight = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hop|Dimensions")
	float CheckCornerHopWidth = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hop|Dimensions")
	float CheckCornerHopDepth = 60.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hop|Dimensions")
	float CheckCornerHopHeight = 8.f;

	/* Vertical Hop Check Distance */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hop|FindHopCheckDistnace|Vertical")
	float VerticalDistanceMultiplier = 25.f; // 아래 값들에 곱할 배율
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hop|FindHopCheckDistnace|Vertical")
	float Forward_Vertical = 1.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hop|FindHopCheckDistnace|Vertical")
	float Backward_Vertical = -7.5f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hop|FindHopCheckDistnace|Vertical")
	float LeftRight_Vertical = -2.5f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hop|FindHopCheckDistnace|Vertical")
	float ForwardLeftRight_Vertical = -1.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hop|FindHopCheckDistnace|Vertical")
	float BackwardLeftRight_Vertical = -4.f;


	/* Horizontal Hop Check Distance */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hop|FindHopCheckDistnace|Horizontal")
	float HorizontalDistanceMultiplier = 140.f; // 아래 값들에 곱할 배율
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hop|FindHopCheckDistnace|Horizontal")
	float LeftRight_Horizontal = 1.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hop|FindHopCheckDistnace|Horizontal")
	float ForwardAndBackWardLeftRight_Horizontal = 0.75;



/*---------------Public End------------------*/

private:
	/*---------------------
			Private
	----------------------*/
	/*---------------------
		InitializeValus 
	-----------------------*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InitializeValus", meta = (AllowPrivateAccess = "true"))
	class ACharacter* OwnerCharacter;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InitializeValus", meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* Mesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InitializeValus", meta = (AllowPrivateAccess = "true"))
	class AArrowActor* ArrowActor;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InitializeValus", meta = (AllowPrivateAccess = "true"))
	class UParkourAnimInstance* AnimInstance; // 해당 파쿠르의 IK 및 Climb 관련기능은 UParkourAnimInstance를 Cast 해야한다.	
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InitializeValus", meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* SpringArmComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InitializeValus", meta = (AllowPrivateAccess = "true"))
	UMotionWarpingComponent* MotionWarpingComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InitializeValus", meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* CameraComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InitializeValus", meta = (AllowPrivateAccess = "true"))
	class UCharacterMovementComponent* CharacterMovement;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InitializeValus", meta = (AllowPrivateAccess = "true"))
	class UCapsuleComponent* CapsuleComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InitializeValus", meta = (AllowPrivateAccess = "true"))
	TEnumAsByte<ETraceTypeQuery> ParkourTraceType = ETraceTypeQuery::TraceTypeQuery1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InitializeValus", meta = (AllowPrivateAccess = "true"))
	float CharacterCapsuleCompRadius;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InitializeValus", meta = (AllowPrivateAccess = "true"))
	float CharacterCapsuleCompHalfHeight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InitializeValus", meta = (AllowPrivateAccess = "true"))
	FRotator ParkourRotationRate_Default = FRotator(0.f, 500.f, 0.f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InitializeValus", meta = (AllowPrivateAccess = "true"))
	int32 CheckParkourHeight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InitializeValus", meta = (AllowPrivateAccess = "true"))
	int32 CheckParkourClimbHeight;



	/*------------------
			Bool
	--------------------*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Bool, meta = (AllowPrivateAccess = "true"))
	bool bCanParkour = true; // 파쿠르 가능상태


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Bool, meta = (AllowPrivateAccess = "true"))
	bool bCanManuelClimb;// 직접 Climb 실행

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Bool, meta = (AllowPrivateAccess = "true"))
	bool bCanAutoClimb; // Auto Climb 실행

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Bool, meta = (AllowPrivateAccess = "true"))
	bool bInGround;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = FirstClimbMove, meta = (AllowPrivateAccess = "true"))
	bool bFirstClimbMove = false; // First Climb Move

	/*---------------------------------
		Forward & Right Value
	----------------------------------*/ 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = MoveValue, meta = (AllowPrivateAccess = "true"))
	float ForwardValue;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = MoveValue, meta = (AllowPrivateAccess = "true"))
	float RightValue;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = MoveValue, meta = (AllowPrivateAccess = "true"))
	float HorizontalClimbForwardValue;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = MoveValue, meta = (AllowPrivateAccess = "true"))
	float HorizontalClimbRightValue;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = MoveValue, meta = (AllowPrivateAccess = "true"))
	float VerticalClimbForwardValue;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = MoveValue, meta = (AllowPrivateAccess = "true"))
	float VerticalClimbRightValue;


	/*------------------------------------
			Camera Lerp Curve
	-------------------------------------*/

	// CameraCurveTime은 Public 변수 ParkourCameraLerpCurve에서 SetInitializeReference 함수 실행시에 가져온다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CameraLerpCurve, meta = (AllowPrivateAccess = "true"))
	float CameraCurveTimeMin = 0.4;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CameraLerpCurve, meta = (AllowPrivateAccess = "true"))
	float CameraCurveTimeMax = 0.4;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CameraLerpCurve, meta = (AllowPrivateAccess = "true"))
	FTimerHandle CameraLerpFinishHandle;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CameraLerpCurve, meta = (AllowPrivateAccess = "true"))
	FTimerHandle CameraLerpHandle;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CameraLerpCurve, meta = (AllowPrivateAccess = "true"))
	float CameraCurveAlpha;

	/*-----------------------------------
			Previous State Setting
	-------------------------------------*/
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "PreviousState_Setting", meta = (AllowPrivateAccess = "true"))
	FVector LastClimbHand_RLocation;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "PreviousState_Setting", meta = (AllowPrivateAccess = "true"))
	FVector LastClimbHand_LLocation;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "PreviousState_Setting", meta = (AllowPrivateAccess = "true"))
	float FirstTargetArmLength;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "PreviousState_Setting", meta = (AllowPrivateAccess = "true"))
	FVector FirstCameraLocation;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "PreviousState_Setting", meta = (AllowPrivateAccess = "true"))
	float CurrentTargetArmLength;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "PreviousState_Setting", meta = (AllowPrivateAccess = "true"))
	FVector CurrentTargetCameraRelativeLocation;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "PreviousState_Setting", meta = (AllowPrivateAccess = "true"))
	float LerpTargetArmLength;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "PreviousState_Setting", meta = (AllowPrivateAccess = "true"))
	FVector LerpTargetCameraRelativeLocation;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "PreviousState_Setting", meta = (AllowPrivateAccess = "true"))
	FName LeftIKFootCurveName = "LeftFootIK";
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "PreviousState_Setting", meta = (AllowPrivateAccess = "true"))
	FName RightIKFootCurveName = "RightFootIK";

	/*------------------
			Test
	--------------------*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hand IK Location" , meta = (AllowPrivateAccess = "true"))
	FVector LastLeftHandIKTargetLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hand IK Location", meta = (AllowPrivateAccess = "true"))
	FVector LastRightHandIKTargetLocation;


	/*-------------------------
			Hit Results 
	---------------------------*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitResult", meta = (AllowPrivateAccess = "true"))
	TArray<FHitResult> WallHitTraces;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitResult", meta = (AllowPrivateAccess = "true"))
	TArray<FHitResult> HopHitTraces;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitResult", meta = (AllowPrivateAccess = "true"))
	FHitResult WallHitResult;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitResult", meta = (AllowPrivateAccess = "true"))
	FHitResult WallTopHitResult; // 최초 모서리 Top
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitResult", meta = (AllowPrivateAccess = "true"))
	FHitResult TopHits;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitResult", meta = (AllowPrivateAccess = "true"))
	FHitResult WallDepthHitResult;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitResult", meta = (AllowPrivateAccess = "true"))
	FHitResult WallVaultHitResult;

	// Climb 하는 벽의 정보
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ClimbHitResult", meta = (AllowPrivateAccess = "true"))
	FHitResult ClimbLedgeHitResult;

	// Hop의 정보
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hop", meta = (AllowPrivateAccess = "true"))
	FHitResult HopClimbLedgeHitResult;

	/*---------------------
			Rotator 
	----------------------*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall Rotator", meta = (AllowPrivateAccess = "true"))
	FRotator WallRotation;

	/*----------------------
			Distance 
	-----------------------*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour Distance", meta = (AllowPrivateAccess = "true"))
	float WallHeight;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour Distance", meta = (AllowPrivateAccess = "true"))
	float WallDepth;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour Distance", meta = (AllowPrivateAccess = "true"))
	float VaultHeight;

	/*------------------------------
			Montage Start Time 
	-------------------------------*/
	// ParkourVariable에 있는 MontageStartPosition과 FallingMontageStartPosition
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MontageStartTime", meta = (AllowPrivateAccess = "true"))
	float MontageStartTime = 0.f;


	/*-------------------------
			Corner Check 
	---------------------------*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Corner_Check", meta = (AllowPrivateAccess = "true"))
	bool bOutCornerReturn = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Corner_Check", meta = (AllowPrivateAccess = "true"))
	int32 OutCornerIndex = 0;


	/*---------------
			Hop 
	-----------------*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hop", meta = (AllowPrivateAccess = "true"))
	bool bCanInCornerHop; // 캐릭터의 수직방향 벽으로 Hop 가능 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hop", meta = (AllowPrivateAccess = "true"))
	bool bCanOutCornerHop; // 캐릭터 수평방향 벽으로 Hop 가능 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hop", meta = (AllowPrivateAccess = "true"))
	FRotator CornerHopRotation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hop", meta = (AllowPrivateAccess = "true"))
	float VerticalHopDistance;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hop", meta = (AllowPrivateAccess = "true"))
	float HorizontalHopDistance;


	/*--------------------------
			Gameplay Tags 
	---------------------------*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = GameplayTags, meta = (AllowPrivateAccess = "true"))
	FGameplayTag ParkourStateTag;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = GameplayTags, meta = (AllowPrivateAccess = "true"))
	FGameplayTag ParkourActionTag;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = GameplayTags, meta = (AllowPrivateAccess = "true"))
	FGameplayTag ClimbStyleTag;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = GameplayTags, meta = (AllowPrivateAccess = "true"))
	FGameplayTag ClimbDirectionTag;



/*------------- Private End --------------*/

public:
	/*--------------------
		Public Function
	----------------------*/
	UFUNCTION(BlueprintCallable, Category = "Initialize")
	void SetInitializeReference(
		class ACharacter* Character,
		class UCapsuleComponent* CharacterCapsuleComponent,
		class USpringArmComponent* CameraBoom,
		class UMotionWarpingComponent* MotionWarping,
		class UCameraComponent* Camera,
		class UCharacterMovementComponent* Movement,
		ETraceTypeQuery TraceTypeQuery); // 반드시 먼저 실행 해주어야하는 필수 초기화 함수

	UFUNCTION(BlueprintCallable, Category = "Parkour Callable")
	void PlayParkourCallable();

	UFUNCTION(BlueprintCallable, Category = "Parkour Callable")
	void ParkourCancleCallable();

	UFUNCTION(BlueprintCallable, Category = "Parkour Callable")
	void ParkourDropCallable();

	// Blueprint와 C++의 AddMovementInput을 연결해주는 인터페이스
	UFUNCTION(BlueprintCallable, Category = "Movement Callable")
	void MovementInputCallable(float ScaleValue, bool bFront);

	// Blueprint와 C++의 AddMovementInput을 연결해주는 인터페이스
	UFUNCTION(BlueprintCallable, Category = "Movement Callable")
	void ResetMovementCallable();
	
	UFUNCTION(BlueprintImplementableEvent)
	void BP_AutoJump();

	UFUNCTION(BlueprintImplementableEvent)
	void BP_SetParkourStateWidget();


	void CallMontageLeftIK(bool bIKStart);
	void CallMontageRightIK(bool bIKStart);

private:
	/*----------------------
		Private Function
	------------------------*/


	/*-----------------------
		Check Parkour
	-------------------------*/
	void ParkourAction(); // 핵심 메인 함수
	void ParkourType();
	void ParkourTypeUpdate();
	bool ParkourType_VaultOrMantle();
	bool ParkourType_HighVault();
	void ParkourCheckWallShape();
	void SetupParkourWallHitResult();
	void ParkourCheckWallTopDepthShape();
	void ParkourCheckDistance();
	void TickInGround();
	void ResetParkourHitResult();
	void ResetParkourHitResult_Tick();
	float GetFirstTraceHeight();


	/*-----------------------------------------
			Set Parkour Action / State 
	-------------------------------------------*/
	void SetParkourAction(FName NewParkourActionName); // 맞는 ParkourPDA를 가져옴
	void SetParkourAction(FGameplayTag NewParkourActionTag); // 맞는 ParkourPDA를 가져옴
	void PlayParkourMontage();
	void SetParkourState(FGameplayTag NewParkourState); // 새로운 State 상태로 변경
	void PreviousStateCameraSetting(FGameplayTag PreviousState, FGameplayTag NewParkourState);
	void LerpCameraTimerStart(float FinishTime);
	void FindMontageStartTime();
	void ParkourStateSettings(ECollisionEnabled::Type NewCollisionType, 
		EMovementMode NewMovementMode, 
		FRotator NewRotationRate, 
		bool bDoCollisionTest, 
		bool bStopMovementImmediately);

	/* UFUNCTION MACRO TIMER FUNCTION*/
	UFUNCTION()
	void LerpCameraPosition();
	UFUNCTION()
	void LerpCameraPositionFinish();

	/*-----------------------------------------------------------
			OnMontage AddDynamic 
	-------------------------------------------------------------*/
	UFUNCTION()
	void BlendingOut_SetParkourState(UAnimMontage* animMontage, bool bInterrupted);
	UFUNCTION()
	void ParkourMontageEnded(UAnimMontage* animMontage, bool bInterrupted);


	/*-------------------------------------------------------
			Motaion Warping Location Calculator
	---------------------------------------------------------*/
	/* --- Warp Target Name List --- 
		1. ParkourTop
		2. ParkourDepth
		3. ParkourVault
		4. ParkourMantle
		5. ParkourTop_Temp
	*/
	FVector FindWarpTopLocation(float WarpXOffet, float WarpZOffset);
	FVector FindWarpDepthLocation(float WarpXOffset, float WarpZOffset);
	FVector FindWarpVaultLocation(float WarpXOffset, float WarpZOffset);
	FVector FindWarpMantleLocation(float WarpXOffset, float WarpZOffset);
	FVector FindWarpTopLocation_Temp(float WarpXOffset, float WarpZOffset);


	/*--------------------------------
			Check Climb Style 
	---------------------------------*/
	void CheckClimb();
	void CheckClimbStyle();
	void SetClimbStyle(FName ClimbStyleName);
	void ClimbLedgeResult(); // 중요변수 ClimbLedgeHitResult를 갱신한다.

	/*---------------------------
			Check Surface 
	----------------------------*/
	/* 캐릭터가 움직일수 있는 공간이 충분한지 체크 */
	bool CheckMantleSurface();
	bool CheckClimbSurface();
	bool CheckVaultSurface();
	bool CheckClimbMovementSurface(FHitResult MovementHitResult);


	/*--------------------
		Montage IK
	----------------------*/
	void MontageLeftIK(bool bLastClimbLocation);
	void MontageLeftHandIK();
	void MontageLeftFootIK();
	void MontageRightIK(bool bLastClimbLocation);
	void MontageRightHandIK();
	void MontageRightFootIK();

	/*------------------------------
		Climb Movemnet IK
	--------------------------------*/
	// Hand
	void ClimbMovementIK();
	void ParkourHandIK();
	void ParkourLeftHandIK(FVector CharacterForwardVec, FVector CharacterRightVec, FVector ArrowWorldLoc);
	void ParkourRightHandIK(FVector CharacterForwardVec, FVector CharacterRightVec, FVector ArrowWorldLoc);
	
	// Foot
	void ParkourFootIK();
	void ParkourLeftFootIK(FVector CharacterForwardVec, FVector CharacterRightVec);
	void ParkourRightFootIK(FVector CharacterForwardVec, FVector CharacterRightVec);
	void ResetFootIK(bool bRightFoot);
	

	float GetLeftHandIKModifierGetZ();
	float GetRightHandIKModifierGetZ();

	/*------------------------
			Movement
	--------------------------*/
	// 이동 관련 함수
	void AddMovementInput(float ScaleValue, bool bFront);
	float GetHorizontalAxis(); 	// Climb 상태에서 방향키 입력에 따른 Axis 반환 (AddMovementInput과 연관)	
	float GetVerticalAxis(); // Climb 상태에서 방향키 입력에 따른 Axis 반환 (AddMovementInput과 연관)
	void ClimbMovement();
	void SetClimbMovementTransform(FVector ClimbFirstHitImpactPoint, FVector ClimbTopHitImpactPoint, FRotator NormalizeForwardRotator, FVector NormalizeForwardVector);
	void StopClimbMovement();
	void SetClimbDirection(FGameplayTag NewClimbDirectionTag);
	void SetClimbDirection(FName NewClimbDirectionName);
	void GetClimbForwardValue(float ScaleValue, float& HorizontalClimbForward, float& VerticalClimbForward);
	void GetClimbRightValue(float ScaleValue, float& HorizontalClimbRight, float& VerticalClimbRight);
	float GetClimbMoveSpeed(); // "Climb Move Speed"라는 이름을 가진 커브 값을 이용

	// Climb Style이 Braced인지 FreeHang인지 판단하는 함수
	void CheckClimbBracedStyleMoving(FVector ClimbTopHitImpactPoint, FRotator NormalizeDeltaRotation_Yaw); // ★★
	void ResetMovement();
	bool CheckAirHang();



	/*-----------------
			Drop
	-------------------*/
	void ParkourDrop();
	void FindDropDownHangLocation(); // 파루크상태가 아닐때 캐릭터가 바라보고있는 발아래에 Climb나 Hang이 가능한 장애물이 있는지 확인
	
	UFUNCTION()
	void SetCanManuelClimb();


	/*------------------
			Corner
	--------------------*/
	bool CheckOutCorner();
	/* Out Corner */
	void OutCornerMovement();
	/* In Corner */
	void InCornerMovement();
	void CheckInCornerSide(const FHitResult& CornerDepthHitResult, FVector ArrowActorWorldLocation, FRotator ArrowActorWorldRotation, FVector ArrowForwardVector, FVector ArrowRightVector, float HorizontalAxis);
	void CheckInCornerSideTop(const FHitResult& InCornerSideCheckHitResult);

	void CornerMove(FVector TargetRelativeLocation, FRotator TargetRelativeRotation);
	UFUNCTION()
	void CornerMoveCompleted();
	// 동시에 여러 latent action을 관리하기 위한 고유 ID 카운터
	int32 NextLatentActionUUID = 0;
	int32 GetNextLatentActionUUID();
	
	/*----------------
			Hop
	------------------*/
	void CheckClimbUpOrHop();
	void HopAction();
	void FirstClimbLedgeResult();
	void CheckInCornerHop();
	void CheckOutCornerHop();
	FGameplayTag SelectHopAction(); // Hop Action 판단
	int32 GetHopDirection();
	int32 GetHopDesireRotation(int32 HorizontalDirection, int32 VerticalDirection); 
	FGameplayTag ReturnBracedHopAction(int32 Direction);
	FGameplayTag ReturnFreeHangHopAction(int32 Direction);



	/*---------------------------
		Find Hop Location
	----------------------------*/
	void FindHopLocation();

	void CheckHopWallTopHitResult(); // FindHopLocation에서 검출되지 않았을 경우
	void FindCornerHopLocation();
	void CheckCornerHopWallTopHitResult(); // CheckHopWallTopHitResult 함수와 거의 유사

	void CheckHopCapsuleComponent(); // FindHop & FincCornerHop 둘다 사용

	// Retrun Distance
	float GetSelectVerticalHopDistance();
	float GetSelectHorizontalHopDisance();
	float GetHopResultDistance(const FHitResult& HopHitResult);
	
	
	/*------------------
			ETC.
	-------------------*/
	FRotator GetDesireRotation();
	FGameplayTag GetClimbDesireRotation();
	bool IsLedgeValid();

	ECharacterState GetCharacterState();
	void SetCharacterSprintSpeed(float Speed);
	void SetCharacterWalkingSpeed(float Speed);

	/*-------------------------------------
		Frequently Used Functions
	---------------------------------------*/
	FRotator MakeXRotator(FVector TargetVector);
	FVector GetForwardVector(FRotator Rotator);
	FVector GetRightVector(FRotator Rotator);
	FVector GetUpVector(FRotator Rotator);
	FVector GetCharacterForwardVector();
	FVector GetCharacterRightVector();

};

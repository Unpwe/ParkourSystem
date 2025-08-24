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
	/* ����ڰ� �ݵ�� �����ؾ��ϴ� �Լ�  */	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus")
	float SprintTypeParkour_Speed = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus")
	float WalkingTypeParkour_Speed = 300.f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitializeValus")
	bool bAutoJump = false; // BP_AutoJump�� �� ���ΰ�?

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitializeValus")
	bool bAlwaysParkour = false; 	// Parkour�� Play Parkour�� �ƴ� Tick���� �Ǵ� �� ������

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


	
	/* ���� ó�� �տ� �ִ� ���� Check �ϰų� , Climb ������ �� ���� ���¿��� ĳ������ Z���� �������� �ش� ��ġ��ŭ ����. 
�� ��ġ�� Z����� Trace�� �����Ѵ�. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|Custom Check Parkour Trace", meta = (ClampMax = 0))
	float CheckParkourFromCharacterRootZ = -60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|Custom Check Parkour Trace", meta = (ClampMin = 5, ClampMax = 20))
	float CheckParkourFallingHeightCnt = 15; // ������ �� üũ �� Trace ���� ����

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|Custom Check Parkour Trace", meta = (ClampMin = 5, ClampMax = 15))
	float CheckParkourDepthCnt = 8; // �⺻ ���� üũ �� Trace Depth ���� * 20.f�� �Ѵ�.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|Custom Check Parkour Trace", meta = (ClampMin = 100, ClampMax = 300))
	int32 CheckParkourDistance = 200;


	/*---------------------------------------------------------------------------------------
		Vault�� �������� Parkour Action Type�� �������� ������ ���� ����
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
	float ClimbIKHandSpace = -20.f; // Climb���¿��� �� ������ ����
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|IK|Hand")
	float DirectionMovementSameHand = 0.f; // �̵������ ���� ������ ��ġ�ϴ� ��� �߰������� �ݴ�պ��� �� �� ���� ����ϱ� ����.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|IK|Hand")
	FName LeftIKHandSocketName = "ik_hand_l";
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|IK|Hand")
	FName RightIKHandSocketName = "ik_hand_r";
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitializeValus|IK|Hand")
	FName LeftHandSocketName = "hand_l";
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitializeValus|IK|Hand")
	FName RightHandSocketName = "hand_r";
	
	/*
		IK�� ������ �� ���� ImpactNormal ���� �̿��Ͽ� ���� Rotation�� �����ϱ� ������,
		�ִϸ��̼Ǹ��� ���� ȸ�� ���� �ٸ� ���� �������� Ŀ���� �ϰ� ���� ����
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
		Hand ��ġ(�𼭸�)���� �����ŭ �Ʒ��� ��ġ�� ���� Foot�� �� ������ ?
		�� ���� ���� �䱸�ϴ� ������ �� ���� ��ġ���� �ٸ����Ͽ� ���� �ڿ������� �ִϸ��̼� ������ ����.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|IK|Foot")
	float LeftFootPositionToHand = -135.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|IK|Foot")
	float RightFootPositionToHand = -125.f;

	// Braced, FreeHang�� �Ǵ��ϰ��� �� �� �տ������� ������� ��ġ�� ���� �˻��� ������.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InitializeValus|IK|Foot")
	float CheckClimbBracedStyle = -125.f;

	// IK Foot�� ��ġ ���� (Forward/Backward)
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
	// �ش� ArrowActor�� �̿��Ͽ� Climb�Ҷ� WorldLocation���� ���´�.
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
	// InGround ���¸� �Ǵ��ϱ� ���� ĳ������ �� ���� ���� ����ִ��� Ȯ���ϱ����� Z���� �󸶳� �Ʒ��� ���� ������ �Ǵ��ϴ� ����
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
	// Mantle�� Vault �Ѵ� ������ ���� �� �� �ش� �ӵ� �̻��̸� Vault.
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
	// ���� ���� ����� ī�޶� ��ġ �� TargetArmLength ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PreviousState Setting")
	FVector ReachLedge_CameraLocation = FVector(-50.f, 0.f, 70.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PreviousState Setting")
	float ReachLedge_TargetArmLength = 500.f;


	/*---------------------------
		Parkour State Settings
	-----------------------------*/
	// ParkourStateSettings �Լ����� �����Ʈ�� ȸ���ӵ�
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
		Left Climb IK, Right Climb IK���� �� ó�� Left, Right�� üũ�� ��
		��ŭ�� ������ �˻��� ������.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Width")
	float CheckClimbForward = 20.f;

	/* ĳ������ ����, ĳ������ �� ���̸� ���������� ���� */
	// �� ���� �ö󰡸� ��ġ ��ŭ ĳ���Ͱ� ������ �� ���� �ö�
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings")
	float CharacterHeightDiff = 0.f;


	/* ClimbIK| Custom Settings| Hand */
	
	// �� ���� �ö󰡸� ��ġ ��ŭ ĳ���Ͱ� ���� ���� �� �ö�
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings|Hand")
	float CharacterHandUpDiff = 0.f;
	
	//�� ���� �ö󰡸� ��ġ ��ŭ ĳ���Ͱ� ������ ���� �� ���´�.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings|Hand")
	float BracedHandFrontDiff = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings|Hand")
	float FreeHangHandFrontDiff = 0.f;

	// Climb ���¿� ���� �� ���� �߰� ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings|Hand")
	float BracedHandMovementHeight = -7.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings|Hand")
	float FreeHangHandMovementHeight = -9.f;
	
	// �� �β� ��ŭ �Ÿ� ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings|Hand")
	float CharacterHandThickness = 3.f;


	/* ClimbIK| Custom Settings| Foot */
	// Foot IK�� �� �� Foot IK�� ��ġ�� ĳ������ Ű���� ����Ͽ� Ŀ�����ϰ� ����
	// ex) CharacterHeightDiff - CharacterFootIKLocation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings|Foot")
	float CharacterFootIKLocation = 150.f;

	// ClimbFootIK���� ���� RightVector�� ���� �߰��� ���ų� ���Ͽ� IK�ϴ� �� ��ġ�� ���������� ���� ����.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings|Foot")
	float CharacterLeftFootLocation = 7.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings|Foot")
	float CharacterFootLocation_Right = 9.f;

	// Foot IK�� �� �� �ִϸ��̼� �� �� �β��� ����Ͽ� ImpactPoint���� ������� ������ ���� ���� ���� ��ų �� ����.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings|Foot")
	float BracedFootAddThickness_Left = 17.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbIK|Custom Settings|Foot")
	float BracedFootAddThickness_Right = 17.f;


	/*-------------------
			Drop
	---------------------*/
	// Climb ���¿��� ParkourDrop �Լ��� ����Ǿ����� bCanManuelClimb ������ true�� �¾� �� Timer �ð��� �����ϴ� ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop")
	float Drop_CanManuelClimbBecomesTrueTime = 0.3f;
	// ĳ������ �� �� �� ���� �˻��Ҷ� �����. ĳ������ ��ġ������ �ش� ���� ���� Trace�� ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop")
	float CheckCharacterDropHeight = -120.f;
	// DropDown�� �����Ҷ� ĳ���� ȸ������ �������, ��Ʈ�ѷ� ȸ������ ����� ������.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop")
	bool bUseControllerRotation_DropDown = false;

	/*----------------------------
		Climb Movement
	-----------------------------*/
	// Climb���¿��� ������ �� ��Ʈ�ѷ��� ����� ���⿡���� ������ ������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|ClimbMovement")
	bool bUseControllerRotation_Climb = false;

	// ClimbMovement���� �̵��Ҷ� �Ÿ�üũ
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|ClimbMovement")
	float ClimbMovementDistance = 17.f;

	// ClimbMovement���� �̵��Ҷ� ĳ������ �̵����⿡ ��ֹ��� �ִ��� üũ�� �Ÿ�
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|ClimbMovement|Obstacle")
	float ClimbMovementObstacleCheckDistance = 13.f;
	// ��ֹ��� üũ�� Capsule Trace�� ũ��
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|ClimbMovement|Obstacle")
	float CheckClimbMovementSurface_CapsuleTraceRadius = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|ClimbMovement|Obstacle")
	float CheckClimbMovementSurface_CapsuleTraceHalfHeight = 82.f;


	/*
		ClimbMovement���� ������ �� ���������� ������ ���� ��ġ�� ĳ���͸� �����̷��� �� ����
		���� ��ġ������ Target ��ġ���� �̵��ϴ� ���� �ӵ�
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|ClimbMovement|Interp Speed")
	float ClimbStyleBracedInterpSpeed = 2.7f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|ClimbMovement|Interp Speed")
	float ClimbStyleFreeHangInterpSpeed = 1.8f;

	/*
	* CornerMove, ClimbMovement���� SetActorLocationAndRoation�̳� MoveComponentTo �Լ��� ȣ�� �� �� �ʿ��� ����
	* ��Ʈ��� ��Ÿ�� �ִϸ��̼��� �����ϸ鼭 �ִϸ��̼� ������Ʈ �ӽ����� ���ư���,
	* �� �ִϸ��̼��� ��Ʈ�� �ƴѰ�� ��Ʈ ����� �޶� ��ġ���� Ʋ���� ���� �ִµ�,
	* �˸��� �ڸ��� �˸°� �ִϸ��̼� ���¸� ��ġ ��Ű�� ���� ���������� �����ϱ����� ����
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
	/* Corner�� ���� �ش� �κ��� ��ġ�� Move Component To�� �����ϴµ�,
	Trace�Ͽ� �˾Ƴ� �𼭸��� ��ġ���� �����ŭ�� Height�� ���� �ش� ��ġ�� Target Relative Location�� �ϸ�
	ĳ���Ͱ� �˸°� �̵� ���� Ŀ�����ϰ� ���� �Ҽ� �ִ� ����
	�ΰ��� ���� �� 100 ~ 107 (�ִϸ��̼Ǹ��� �ٸ��� ����) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CornerMovement")
	float CornerMovementHeight = 105.f;

	/*
		�ڳʸ� ���� Check�ؾ��ϴ� ĳ������ CapsuleSize
		������ �������� ���� ������, ĳ���͸��� Climb �ִϸ��̼����³� ũ�Ⱑ �ٸ� �� �ֱ� ������
		�ش� ĳ������ ũ�� �� �ִϸ��̼� ���¿� ���� �˸´� ����� �˻��ϱ� �����̴�.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CornerMovement")
	float CornerCheckCapsuleRadius_Subtraction = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CornerMovement")
	float CornerCheckCapsuleHalfHeight_Subtraction = 6.f;

	/*
		�ڳʸ� �̵��ϴ� �Լ��� ������ �� ����������
		MoveComponentTo �Լ��� OverTime ���ڰ����� ����ϴ� ����.
		�ش� �� ��ŭ �ִϸ��̼��� �����ð��� �־� �ε巴�� �����δ�.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CornerMovement|OverTime")
	float CornerMoveComponentToOverTime_Braced = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CornerMovement|OverTime")
	float CornerMoveComponentToOverTime_FreeHang = 0.3f;


	/*--------------
			Hop
	----------------*/
	/* 
		Character�� Hop�� ������ �� Hop�� ������ �κ��� ã���� ��
		Character CapsuleComponent ��ŭ �˻縦 �ϰ� �Ǵµ�,
		�̶� �˻��� CapsuleComponent�� ũ�⸦ ���� �ϴ� ����
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
	float VerticalDistanceMultiplier = 25.f; // �Ʒ� ���鿡 ���� ����
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
	float HorizontalDistanceMultiplier = 140.f; // �Ʒ� ���鿡 ���� ����
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
	class UParkourAnimInstance* AnimInstance; // �ش� ������ IK �� Climb ���ñ���� UParkourAnimInstance�� Cast �ؾ��Ѵ�.	
	
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
	bool bCanParkour = true; // ���� ���ɻ���


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Bool, meta = (AllowPrivateAccess = "true"))
	bool bCanManuelClimb;// ���� Climb ����

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Bool, meta = (AllowPrivateAccess = "true"))
	bool bCanAutoClimb; // Auto Climb ����

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

	// CameraCurveTime�� Public ���� ParkourCameraLerpCurve���� SetInitializeReference �Լ� ����ÿ� �����´�.
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
	FHitResult WallTopHitResult; // ���� �𼭸� Top
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitResult", meta = (AllowPrivateAccess = "true"))
	FHitResult TopHits;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitResult", meta = (AllowPrivateAccess = "true"))
	FHitResult WallDepthHitResult;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitResult", meta = (AllowPrivateAccess = "true"))
	FHitResult WallVaultHitResult;

	// Climb �ϴ� ���� ����
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ClimbHitResult", meta = (AllowPrivateAccess = "true"))
	FHitResult ClimbLedgeHitResult;

	// Hop�� ����
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
	// ParkourVariable�� �ִ� MontageStartPosition�� FallingMontageStartPosition
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
	bool bCanInCornerHop; // ĳ������ �������� ������ Hop ���� ����
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hop", meta = (AllowPrivateAccess = "true"))
	bool bCanOutCornerHop; // ĳ���� ������� ������ Hop ���� ����
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
		ETraceTypeQuery TraceTypeQuery); // �ݵ�� ���� ���� ���־���ϴ� �ʼ� �ʱ�ȭ �Լ�

	UFUNCTION(BlueprintCallable, Category = "Parkour Callable")
	void PlayParkourCallable();

	UFUNCTION(BlueprintCallable, Category = "Parkour Callable")
	void ParkourCancleCallable();

	UFUNCTION(BlueprintCallable, Category = "Parkour Callable")
	void ParkourDropCallable();

	// Blueprint�� C++�� AddMovementInput�� �������ִ� �������̽�
	UFUNCTION(BlueprintCallable, Category = "Movement Callable")
	void MovementInputCallable(float ScaleValue, bool bFront);

	// Blueprint�� C++�� AddMovementInput�� �������ִ� �������̽�
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
	void ParkourAction(); // �ٽ� ���� �Լ�
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
	void SetParkourAction(FName NewParkourActionName); // �´� ParkourPDA�� ������
	void SetParkourAction(FGameplayTag NewParkourActionTag); // �´� ParkourPDA�� ������
	void PlayParkourMontage();
	void SetParkourState(FGameplayTag NewParkourState); // ���ο� State ���·� ����
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
	void ClimbLedgeResult(); // �߿亯�� ClimbLedgeHitResult�� �����Ѵ�.

	/*---------------------------
			Check Surface 
	----------------------------*/
	/* ĳ���Ͱ� �����ϼ� �ִ� ������ ������� üũ */
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
	// �̵� ���� �Լ�
	void AddMovementInput(float ScaleValue, bool bFront);
	float GetHorizontalAxis(); 	// Climb ���¿��� ����Ű �Է¿� ���� Axis ��ȯ (AddMovementInput�� ����)	
	float GetVerticalAxis(); // Climb ���¿��� ����Ű �Է¿� ���� Axis ��ȯ (AddMovementInput�� ����)
	void ClimbMovement();
	void SetClimbMovementTransform(FVector ClimbFirstHitImpactPoint, FVector ClimbTopHitImpactPoint, FRotator NormalizeForwardRotator, FVector NormalizeForwardVector);
	void StopClimbMovement();
	void SetClimbDirection(FGameplayTag NewClimbDirectionTag);
	void SetClimbDirection(FName NewClimbDirectionName);
	void GetClimbForwardValue(float ScaleValue, float& HorizontalClimbForward, float& VerticalClimbForward);
	void GetClimbRightValue(float ScaleValue, float& HorizontalClimbRight, float& VerticalClimbRight);
	float GetClimbMoveSpeed(); // "Climb Move Speed"��� �̸��� ���� Ŀ�� ���� �̿�

	// Climb Style�� Braced���� FreeHang���� �Ǵ��ϴ� �Լ�
	void CheckClimbBracedStyleMoving(FVector ClimbTopHitImpactPoint, FRotator NormalizeDeltaRotation_Yaw); // �ڡ�
	void ResetMovement();
	bool CheckAirHang();



	/*-----------------
			Drop
	-------------------*/
	void ParkourDrop();
	void FindDropDownHangLocation(); // �ķ�ũ���°� �ƴҶ� ĳ���Ͱ� �ٶ󺸰��ִ� �߾Ʒ��� Climb�� Hang�� ������ ��ֹ��� �ִ��� Ȯ��
	
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
	// ���ÿ� ���� latent action�� �����ϱ� ���� ���� ID ī����
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
	FGameplayTag SelectHopAction(); // Hop Action �Ǵ�
	int32 GetHopDirection();
	int32 GetHopDesireRotation(int32 HorizontalDirection, int32 VerticalDirection); 
	FGameplayTag ReturnBracedHopAction(int32 Direction);
	FGameplayTag ReturnFreeHangHopAction(int32 Direction);



	/*---------------------------
		Find Hop Location
	----------------------------*/
	void FindHopLocation();

	void CheckHopWallTopHitResult(); // FindHopLocation���� ������� �ʾ��� ���
	void FindCornerHopLocation();
	void CheckCornerHopWallTopHitResult(); // CheckHopWallTopHitResult �Լ��� ���� ����

	void CheckHopCapsuleComponent(); // FindHop & FincCornerHop �Ѵ� ���

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

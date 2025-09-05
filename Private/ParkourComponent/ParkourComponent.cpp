// Fill out your copyright notice in the Description page of Project Settings.


#include "ParkourComponent/ParkourComponent.h"
#include "ParkourComponent/ArrowActor.h"
#include "Curves/CurveFloat.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "FunctionLibrary/PSFunctionLibrary.h"
#include "MotionWarpingComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animations/ParkourAnimInstance.h"
#include "DataAssets/ParkourPDA.h"


// Sets default values for this component's properties
UParkourComponent::UParkourComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

void UParkourComponent::SetInitializeReference(ACharacter* Character, UCapsuleComponent* CharacterCapsuleComponent, USpringArmComponent* CameraBoom, UMotionWarpingComponent* MotionWarping, UCameraComponent* Camera, UCharacterMovementComponent* Movement, ETraceTypeQuery TraceTypeQuery)
{
	OwnerCharacter = Character;
	CapsuleComponent = CharacterCapsuleComponent;
	SpringArmComponent = CameraBoom;
	MotionWarpingComponent = MotionWarping;
	CameraComponent = Camera;
	CharacterMovement = Movement;
	ParkourTraceType = TraceTypeQuery;

	if (OwnerCharacter && SpringArmComponent && CharacterMovement)
	{
		Mesh = OwnerCharacter->GetMesh();
		AnimInstance = Cast<UParkourAnimInstance>(Mesh->GetAnimInstance());
		FirstTargetArmLength = SpringArmComponent->TargetArmLength;
		FirstCameraLocation = SpringArmComponent->GetRelativeLocation();
		ParkourRotationRate_Default = CharacterMovement->RotationRate;

		CheckParkourHeight = CanClimbHeight / 10; // Check Parkour Height
		CheckParkourClimbHeight = CanClimbHeight / 5; // Check Parkour Climb Height

		if (CapsuleComponent && AnimInstance)
		{
			CharacterCapsuleCompRadius = CapsuleComponent->GetScaledCapsuleRadius();
			CharacterCapsuleCompHalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();		
		}
		else
		{
			UPSFunctionLibrary::CrashLog(TEXT("Important information is missing. Please check. (CapsuleComponent, AnimInstance)"),
				TEXT("Important information is missing. Please check. (CapsuleComponent, AnimInstance)"));
		}

		FActorSpawnParameters SpawnParameter;
		SpawnParameter.Owner = OwnerCharacter;
		SpawnParameter.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::Undefined;
		ArrowActor = GetWorld()->SpawnActor<AArrowActor>(AArrowActor::StaticClass(), OwnerCharacter->GetActorTransform(), SpawnParameter);
		if (ArrowActor)
		{
			ArrowActor->AttachToComponent(Mesh, FAttachmentTransformRules::KeepRelativeTransform);
			ArrowActor->SetActorRelativeLocation(FVector(ArrowLocation.X, 0.f, ArrowLocation.Y - CharacterHeightDiff));
			ArrowActor->SetActorRelativeRotation(FRotator(0.f, 90.f, 0.f));
			ArrowActor->SetActorHiddenInGame(true);
		}
		else
		{
			UPSFunctionLibrary::CrashLog(TEXT("Can't find ArrowActor"), TEXT("Can't find ArrowActor"));
		}

		AnimInstance->OnMontageBlendingOut.AddDynamic(this, &UParkourComponent::BlendingOut_SetParkourState);
		AnimInstance->OnMontageEnded.AddDynamic(this, &UParkourComponent::ParkourMontageEnded);
	}

	if (ParkourCameraLerpCurve)
		ParkourCameraLerpCurve->GetTimeRange(CameraCurveTimeMin, CameraCurveTimeMax);

	ParkourStateTag = UPSFunctionLibrary::GetGameplayTag(FName(TEXT("Parkour.State.NotBusy")));
	ParkourActionTag = UPSFunctionLibrary::GetGameplayTag(FName(TEXT("Parkour.Action.NoAction")));

#ifdef DEBUG_PARKOURCOMPONENT
	if (!Character)
		LOG(Error, TEXT("Chactcer NULL"));
	if (!CharacterMovement)
		LOG(Error, TEXT("CharacterMovement NULL"));
#endif
}


// Called when the game starts
void UParkourComponent::BeginPlay()
{
	Super::BeginPlay();
	// ...
}


// Called every frame
void UParkourComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	TickInGround();
	ClimbMovementIK();
	// ...
}

void UParkourComponent::PlayParkourCallable()
{
	if (!OwnerCharacter || !CharacterMovement)
		return;

	if((bCanParkour && CapsuleComponent->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics)
		|| UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.Climb")))
		ParkourAction();
}


void UParkourComponent::ParkourCancleCallable()
{
	if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.Climb")))
	{
		ParkourActionTag = UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.Action.NoAction"));
		bCanManuelClimb = true;
		bCanAutoClimb = true;
	}

	ResetParkourHitResult();
}

void UParkourComponent::ParkourDropCallable()
{
	ParkourDrop();
}


// 캐릭터에서 연결할 BlueprintCallable 함수
void UParkourComponent::MovementInputCallable(float ScaleValue, bool bFront)
{
	// Ledge 상태에 도달하게 되면 Climb 상태에서 첫 움직임 false로 초기화
	if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.ReachLedge")))
		bFirstClimbMove = false; 

	AddMovementInput(ScaleValue, bFront); // Front

}

void UParkourComponent::ResetMovementCallable()
{
	ResetMovement();
}


void UParkourComponent::CallMontageLeftIK(bool bIKStart)
{
	MontageLeftIK(bIKStart);
}

void UParkourComponent::CallMontageRightIK(bool bIKStart)
{
	MontageRightIK(bIKStart);
}


void UParkourComponent::CallableAdditiveHandIKLocation(bool bLeft, FVector AddtiveLocation)
{
	AdditiveHandIKLocation(bLeft, AddtiveLocation);
}

void UParkourComponent::CallableAdditiveHandIKRotation(bool bLeft, FRotator AddtiveRotation)
{
	AdditiveHandIKRotation(bLeft, AddtiveRotation);
}

void UParkourComponent::CallableAdditiveFootIKLocation(bool bLeft, FVector AddtiveLocation)
{
	AdditiveFootIKLocation(bLeft, AddtiveLocation);
}




/*----------------------
	Private Function
------------------------*/
/*---------------------------------------*/

/*------------------
	Check Parkour
--------------------*/
// 파쿠르를 실행하는 메인 함수 
void UParkourComponent::ParkourAction()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("ParkourAction"));
#endif

	// No Action 상태일때 실행가능
	if (UPSFunctionLibrary::CompGameplayTagName(ParkourActionTag, FName(TEXT("Parkour.Action.NoAction"))))
	{
		if (bAutoClimb ? bCanAutoClimb : bCanManuelClimb)
		{
			ParkourCheckWallShape();
			ParkourCheckDistance();
			ParkourType();
		}
	}
}


// 파쿠르 상태 GameplayTag 업데이트
void UParkourComponent::ParkourType()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("ParkourType"));
#endif

	if (!WallTopHitResult.bBlockingHit)
	{
		if (bAutoClimb && UPSFunctionLibrary::CompGameplayTagName(ParkourActionTag, FName(TEXT("Parkour.Action.NoAction"))))
		{
			OwnerCharacter->Jump();
		}
			
	}
	else
		ParkourTypeUpdate();
}

void UParkourComponent::ParkourTypeUpdate()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("ParkourTypeUpdate"));
#endif

	// 아무런 상태가 아닐시에
	if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, "Parkour.State.NotBusy"))
	{
		if (bInGround)
		{
			if (ParkourType_VaultOrMantle())
			{
				#ifdef DEBUG_PARKOURCOMPONENT
					LOG(Warning, TEXT("ParkourType_VaultOrMantle -> True"));
				#endif
			}
			else if (ParkourType_HighVault())
			{
				#ifdef DEBUG_PARKOURCOMPONENT
					LOG(Warning, TEXT("ParkourType_HighVault -> True"));
				#endif
			}
			else if (UKismetMathLibrary::InRange_FloatFloat(WallHeight, 0.f, 30.f, true, true))
			{
				#ifdef DEBUG_PARKOURCOMPONENT
					LOG(Warning, TEXT("Jump 구간"));
				#endif
				// 벽 높이가 매우 낮음. 파쿠르가 불가능 하여 점프만 가능한 시점
				if (bAutoJump)
					BP_AutoJump();
				else
					OwnerCharacter->Jump(); // 여기까지오면 파쿠르가 아예 할 수 없다는 뜻이므로 점프 처리
			}
			else if (CheckClimbSurface())
			{
				// Climb 가능 존재
				//if (WallHeight >= CanClimbHeight)
				#ifdef DEBUG_PARKOURCOMPONENT
					LOG(Warning, TEXT("Climb 가능 구간"));
				#endif
			
					CheckClimb();
			}
			else
				SetParkourAction(TEXT("Parkour.Action.NoAction"));
		}
		else if (CheckClimbSurface())
		{
			// 땅이 아니므로 Climb 가능 존재	
			CheckClimb();
		}
	}
	// 현재 Climb 상태일 때,
	else if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, "Parkour.State.Climb"))
	{
		// 현재 Climb 상태 일때 Hop 가능 존재
		// 추후 추가
		CheckClimbUpOrHop();
	}
}

bool UParkourComponent::ParkourType_VaultOrMantle()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("ParkourType_VaultOrMantle"));
#endif

	// WallHeight = 캐릭터 root부터 벽의 Top까지의 높이
	// WallDepth = 벽의 첫 Top 지점부터 끝까지의 거리
	// VaultHeight = 벽의 Top.Z부터 Vault 착지부분의 땅의 Z축까지의 높이

	bool bVaultHeight = CheckVaultMinHeight <= WallHeight && WallHeight <= CheckVaultMaxHeight;
	bool bVaultDepth = CheckThinVaultMinDepth <= WallDepth && WallDepth <= CheckVaultMaxDepth;
	bool bVaultLandingHeight = CheckVaultMinHeight <= VaultHeight && VaultHeight <= CheckVaultMaxHeight;

	/*
	bVaultHeight == A / bVaultDepth == B / bVaultLandingHeight == C
	ABC -> Vault
	ABC' -> Mantle
	AB' -> Mantle
	A' -> LowMantle
	Else -> ret -> false
	*/


	/* Can Vault */
	if (bVaultHeight && bVaultDepth && bVaultLandingHeight)
	{
		bool bThinVault = CheckThinVaultMinDepth <= WallDepth && WallDepth <= CheckThinVaultMaxDepth;

		// 벽이 높지않고 Vault 할 수있으며 벽의 폭이 좁은 경우
		if (bThinVault)
		{
			if (CheckVaultSurface())
				SetParkourAction(TEXT("Parkour.Action.ThinVault"));
			else
				SetParkourAction(TEXT("Parkour.Action.NoAction"));
		}
		else if (CharacterMovement->Velocity.Length() > Velocity_VaultMantle)
		{
			// 속도 체크
			if (CheckVaultSurface())
				SetParkourAction(TEXT("Parkour.Action.Vault"));
			else
				SetParkourAction(TEXT("Parkour.Action.NoAction"));
		}
		else
		{
			if (CheckMantleSurface())
				SetParkourAction(TEXT("Parkour.Action.Mantle"));
			else
				SetParkourAction(TEXT("Parkour.Action.NoAction"));
		}

		return true;
	}


	/* Low Mantle */
	if (!bVaultHeight)
	{
		// WallHeight가 낮아 Mantle만 가능
		bool bLowMantle = CheckLowMantleMin <= WallHeight && WallHeight <= CheckLowMantleMax;

		if (bLowMantle)
		{
			LOG(Warning, TEXT("Low Mantle"));
			if (CheckMantleSurface())
				SetParkourAction(TEXT("Parkour.Action.LowMantle"));
			else
				SetParkourAction(TEXT("Parkour.Action.NoAction"));

			return true; // Low Mantle일 때만 True 출력
		}
		else
		{
			#define DEBUG_PARKOURCOMPONENT
				LOG(Warning, TEXT("Don't Low Mantle"));
		}
			
	}

	/* Can Mantle */
	if (bVaultHeight && (!bVaultDepth || !bVaultLandingHeight))
	{
		if (CheckMantleSurface())
			SetParkourAction(TEXT("Parkour.Action.Mantle"));
		else
			SetParkourAction(TEXT("Parkour.Action.NoAction"));

		return true;
	}


	return false;
}

bool UParkourComponent::ParkourType_HighVault()
{
	// WallHeight = 캐릭터 root부터 벽의 Top까지의 높이
	// WallDepth = 벽의 첫 Top 지점부터 끝까지의 거리
	// VaultHeight = 벽의 Top.Z부터 Vault 착지부분의 땅의 Z축까지의 높이

	bool bHighVaultHeight = CheckVaultMaxHeight <= WallHeight && WallHeight <= CheckHighVaultMaxHeight;
	bool bHighVaultDepth = CheckThinVaultMaxDepth <= WallDepth && WallDepth <= CheckVaultMaxDepth;
	bool bHighVaultLandingHeight = CheckThinVaultMinDepth <= VaultHeight && VaultHeight <= CheckHighVaultMaxHeight;

	/*
	bHighVaultHeight == A / bHighVaultDepth == B / bHighVaultLandingHeight == C
	ABC -> HighVault
	C' -> Mantle
	B' -> Mantle
	Else -> ret -> false
	*/


	if (bHighVaultHeight && bHighVaultDepth && bHighVaultLandingHeight)
	{
		if (CharacterMovement->Velocity.Length() > Velocity_MantleOrHighVault)
		{
			if (CheckVaultSurface())
				SetParkourAction(TEXT("Parkour.Action.HighVault"));
			else
				SetParkourAction(TEXT("Parkour.Action.NoAction"));
		}
		else
		{
			if (CheckMantleSurface())
				SetParkourAction(TEXT("Parkour.Action.Mantle"));
			else
				SetParkourAction(TEXT("Parkour.Action.NoAction"));
		}

		return true;
	}

	if (bHighVaultHeight && (!bHighVaultDepth || !bHighVaultLandingHeight))
	{
		if (CheckMantleSurface())
			SetParkourAction(TEXT("Parkour.Action.Mantle"));
		else
			SetParkourAction(TEXT("Parkour.Action.NoAction"));

		return true;
	}
	
	return false;
}



// 각종 지형지물 트레이스하여 FHitResult로 정보 습득
void UParkourComponent::ParkourCheckWallShape()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("ParkourCheckWallShape"));
#endif

	// 캐릭터 정면에 벽이 있는지 판단하기 위한 Trace
	int32 FirstCheck_LastIndex = UKismetMathLibrary::SelectInt(CheckParkourFallingHeightCnt, 15, CharacterMovement->IsFalling());
	FVector CharacterFwdVector = OwnerCharacter->GetActorForwardVector();
	FVector CharacterLocation = OwnerCharacter->GetActorLocation();

	FHitResult FirstCheckHitResult;
	for (int32 FirstCheckIndex = 0; FirstCheckIndex < FirstCheck_LastIndex; FirstCheckIndex++)
	{
		FVector FirstCheckBasePos = FVector(0.f, 0.f, (FirstCheckIndex * 20.f) + GetFirstTraceHeight()) + CharacterLocation;
		FVector FirstCheckStartPos = FirstCheckBasePos + (CharacterFwdVector * -20.f);
		FVector FirstCheckEndPos = FirstCheckBasePos + (CheckParkourDistance * CharacterFwdVector);
		UKismetSystemLibrary::SphereTraceSingle(GetWorld(), FirstCheckStartPos, FirstCheckEndPos, 10.f, ParkourTraceType, false, TArray<AActor*>(), DDT_ParkourCheckWallShape, FirstCheckHitResult, true);

		// Find Wall
		if (FirstCheckHitResult.bBlockingHit && !FirstCheckHitResult.bStartPenetrating)
		{
			// 두번째 Trace 구간
			// 처음 벽의 위치를 파악한 FirstCheckHitResult의 값을 통해 두번째 Trace를 진행
			WallHitTraces.Empty();

			// Climb 상태인 경우 FirstCheckHitResult.ImpactPoint.Z를 아닌경우 OwnerCharacter의 Location.Z값을 사용
			bool bClimbState = UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, FName(TEXT("Parkour.State.Climb")));
			FVector FirstHitTraceLocation = FVector(FirstCheckHitResult.ImpactPoint.X, FirstCheckHitResult.ImpactPoint.Y, (bClimbState ? FirstCheckHitResult.ImpactPoint.Z : CharacterLocation.Z));
			FVector NormalizeDeltaRightVector = GetRightVector(UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(FirstCheckHitResult.ImpactNormal));
			FVector NormalizeDeltaForwadVector = GetForwardVector(UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(FirstCheckHitResult.ImpactNormal));

			FHitResult SecondCheckHitResult;
			float PositionIndex = UPSFunctionLibrary::SelectParkourStateFloat(ParkourStateTag, -40.f, 0.f, 0.f, -20.f);
			int32 SecondCheck_LastIndex = UKismetMathLibrary::FTrunc(UPSFunctionLibrary::SelectParkourStateFloat(ParkourStateTag, 4.f, 0.f, 0.f, 2.f));
			for (int32 i = 0; i <= SecondCheck_LastIndex; i++)
			{
				// 왼쪽 오른쪽 균등하게 검사하기 위해 i는 홀수여야한다.
				float StatePositionIndex = i * 20 + PositionIndex;
				FVector BasePos = FVector(0.f, 0.f, bClimbState ? 0.f : CheckParkourFromCharacterRootZ) + FirstHitTraceLocation + (NormalizeDeltaRightVector * StatePositionIndex);
				FVector StartPos = BasePos + (NormalizeDeltaForwadVector * -40.f);
				FVector EndPos = BasePos + (NormalizeDeltaForwadVector * 30.f);

				UKismetSystemLibrary::LineTraceSingle(GetWorld(), StartPos, EndPos, ParkourTraceType, false, TArray<AActor*>(), DDT_ParkourCheckWallShape, SecondCheckHitResult, true);
				HopHitTraces.Empty();

				// 세번째 Trace 구간
				int32 ThirdCheck_LastIndex = UPSFunctionLibrary::SelectParkourStateFloat(ParkourStateTag, CheckParkourClimbHeight, 0.f, 0.f, 7.f);
				for (int32 j = 0; j < ThirdCheck_LastIndex; j++)
				{
					// 위의 Second Check Hit Result의 자리에서 세로로 Line Trace를 하는 과정
					FVector HopTraceStartPos = SecondCheckHitResult.TraceStart + FVector(0.f, 0.f, j * 8.f);
					FVector HopTraceEndPos = SecondCheckHitResult.TraceEnd + FVector(0.f, 0.f, j * 8.f);

					FHitResult HopCheckHitResult;
					UKismetSystemLibrary::LineTraceSingle(GetWorld(), HopTraceStartPos, HopTraceEndPos, ParkourTraceType, false, TArray<AActor*>(), DDT_ParkourCheckWallShape, HopCheckHitResult, true);
					HopHitTraces.Emplace(HopCheckHitResult); // 멤버 변수
				}

				for (int32 Index = 1; Index < HopHitTraces.Num(); Index++)
				{
					FHitResult PrevHopHitResult = HopHitTraces[Index - 1];
					FHitResult CurrentHopHitResult = HopHitTraces[Index];
					float PrevHtDistance = 0.f;
					float CurrentHtDistance = 0.f;

					// bBlocking 상태에 따라 Hit한 Distance인지, Trace 그 자체 길이인지 여부가 갈린다.
					if (CurrentHopHitResult.bBlockingHit)
						CurrentHtDistance = CurrentHopHitResult.Distance;
					else
						CurrentHtDistance = UKismetMathLibrary::Vector_Distance(CurrentHopHitResult.TraceStart, CurrentHopHitResult.TraceEnd);

					if (PrevHopHitResult.bBlockingHit)
						PrevHtDistance = PrevHopHitResult.Distance;
					else
						PrevHtDistance = UKismetMathLibrary::Vector_Distance(PrevHopHitResult.TraceStart, PrevHopHitResult.TraceEnd);

					/*
						Distance가 양수가 나왔다는 것은 Prev가 마지막으로 최소값인 부분이라는 중명이다.
						캐릭터에게 유효한 벽의 최종적인 정보를 중요 변수인 WallHitTrace에 저장한다.

					*/
					if (CurrentHtDistance - PrevHtDistance > 10.f)
					{
						WallHitTraces.Emplace(PrevHopHitResult);
						break;
					}
				}
			}
			break;
		}
	}

	


#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("HopHitTraces Num : %d"), HopHitTraces.Num());
#endif

	SetupParkourWallHitResult();

}

void UParkourComponent::SetupParkourWallHitResult()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("SetupParkourWallHitResult"));
	LOG(Warning, TEXT("WallHitTraces Num : %d"), WallHitTraces.Num());
#endif

	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	for (int32 i = 0; i < WallHitTraces.Num(); i++)
	{
		if (i == 0)
		{
			WallHitResult = WallHitTraces[0];
			continue;
		}

		float CurrnetHitResultDistance = UKismetMathLibrary::Vector_Distance(WallHitTraces[i].ImpactPoint, CharacterLocation);
		float PrevHitResultDistance = UKismetMathLibrary::Vector_Distance(CharacterLocation, WallHitResult.ImpactPoint);

		// 최소 거리일때 갱신
		if (CurrnetHitResultDistance <= PrevHitResultDistance)
			WallHitResult = WallHitTraces[i];
	}

	// 구한 WallHitResult를 통해 Wall의 Top과 Depth를 구한다
	ParkourCheckWallTopDepthShape();
}


// ParkourCheckWallShape 함수에서 구한 FHitResult 값을 이용해 Wall Top을 구하고,
// Vault가 가능할 경우 Depth를 구하는 함수
void UParkourComponent::ParkourCheckWallTopDepthShape()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("ParkourCheckWallTopDepthShape"));
#endif

	if (WallHitResult.bBlockingHit && !WallHitResult.bStartPenetrating)
	{
		// Climb 상태가 아닌경우에만 Wall Rotation이 필요하다.
		if (!UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, FName(TEXT("Parkour.State.Climb"))))
			WallRotation = UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(WallHitResult.ImpactNormal);

		FVector WallForwardVector = GetForwardVector(WallRotation);
		for (int32 i = 0; i < CheckParkourDepthCnt; i++)
		{	
			FVector BasePos = WallHitResult.ImpactPoint + (WallForwardVector * (i * 30));
			FVector StartPos = BasePos + FVector(0.f, 0.f, 7.f);
			FVector EndPos = BasePos - FVector(0.f, 0.f, 7.f);

			FHitResult TopHitResult;
			bool bHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartPos, EndPos, 2.5f, ParkourTraceType, false, TArray<AActor*>(), DDT_ParkourCheckWallTopDepthShape, TopHitResult, true);

			// 첫번째인 경우 Hit하지않았다면 continue
			if (i == 0)
			{
				if (bHit)
				{
					// WallTopHitResult는 처음 Top을 의미한다.
					WallTopHitResult = TopHitResult;
					TopHits = TopHitResult;
				}
				else
					continue;
			}
			else if (bHit)
				TopHits = TopHitResult; // 검사 된다면 계속 갱신


			// 더이상 Hit 하지않았다면 Vault 가능성이 있다는 뜻
			// 때문에 벽 너머에 착지할 수 있는 곳이 있는지 판단하는 로직
			if (!bHit)
			{
				if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.NotBusy")))
				{
					// 최소한의 벽 두께 측정
					// 위에서 계속 초기화 해준 TopHits를 이용하여 Depth 측정
					FVector StartDepthPos = TopHits.ImpactPoint + (WallForwardVector * 30.f);
					FVector EndDepthPos = TopHits.ImpactPoint;
					FHitResult DepthHitResult;
					if (UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartDepthPos, EndDepthPos, 10.f, ParkourTraceType, false, TArray<AActor*>(), DDT_ParkourCheckWallTopDepthShape, DepthHitResult, true))
					{
						// Vault Result를 구하는 과정.
						WallDepthHitResult = DepthHitResult;
					
						FVector StartVaultPos = WallDepthHitResult.ImpactPoint + WallForwardVector * 70.f;
						FVector EndVaultPos = StartVaultPos - FVector(0.f, 0.f, VaultLandingHeight);
						FHitResult VaultResult;
						if (UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartVaultPos, EndVaultPos, 10.f, ParkourTraceType, false, TArray<AActor*>(), DDT_ParkourCheckWallTopDepthShape, VaultResult, true, FLinearColor::Red, FLinearColor::Blue))
							WallVaultHitResult = VaultResult;

					}
				}
				break;
			}
		}
	}
}

void UParkourComponent::ParkourCheckDistance()
{
	if(!WallHitResult.bBlockingHit)
	{
		WallHeight = 0.f;
		WallDepth = 0.f;
		VaultHeight = 0.f;
		return;
	}

	// 캐릭터의 루트와 Top까지의 Z축 높이 측정
	if (WallTopHitResult.bBlockingHit)
	{
		if (OwnerCharacter)
		{
			float MeshRootLocation = Mesh->GetSocketLocation(FName(TEXT("root"))).Z;
			WallHeight = WallTopHitResult.ImpactPoint.Z - MeshRootLocation;
		}	
	}

	// Top과 Dpeth의 Distance.
	if (WallTopHitResult.bBlockingHit && WallDepthHitResult.bBlockingHit)
		WallDepth = UKismetMathLibrary::Vector_Distance(WallTopHitResult.ImpactPoint, WallDepthHitResult.ImpactPoint);
	else
		WallDepth = 0.f;


	// Depth에서 Vault까지의 Z축을 빼서 높이 측정
	if (WallDepthHitResult.bBlockingHit && WallVaultHitResult.bBlockingHit)
		VaultHeight = WallDepthHitResult.ImpactPoint.Z - WallVaultHitResult.ImpactPoint.Z;
	else
		VaultHeight = 0.f;
}

// Tick에서 실행하는 함수
// 캐릭터가 땅에 있을 때 FHitResult들을 Reset해주는 역할
// bAuto Climb = true 상태일시 캐릭터 root 아래를 Tick마다 체크하여 InGround가 아니면 PakrourAction을 실행한다. 
// (ParkourAction 안에는 bAutoClimb가 true 일시 자동 점프하는 하는 로직이 있다.)
void UParkourComponent::TickInGround()
{
	if (!OwnerCharacter)
		return;

	FVector RootSocketLocation = Mesh->GetSocketLocation(TEXT("root"));
	float CheckAddZ = 0.f;
	FVector CheckInGroundLocation;
	FHitResult InGroundHitResult;

	if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.Climb")))
	{	
		CheckAddZ = UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag, CheckAutoClimbToRoot_Braced, CheckAutoClimbToRoot_FreeHang);
		
		FVector WallForwardVector = GetForwardVector(WallRotation);
		// ClimbStyleBracedXYPosition, ClimbStyleFreeHangXYPosition 변수를 통해 조절한 경우 Trace할 길이도 조절 해야한다.
		float CustomClimbXYPostion = UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag, ClimbStyleBracedXYPosition, ClimbStyleFreeHangXYPosition);	
		FVector ClimbXYPostionVector = WallForwardVector * (CustomClimbXYPostion/2);
		CheckInGroundLocation = FVector(RootSocketLocation.X + ClimbXYPostionVector.X, RootSocketLocation.Y + ClimbXYPostionVector.Y, RootSocketLocation.Z + CheckAddZ);


		// Climb 전용 Check Ground
		bInGround = UKismetSystemLibrary::BoxTraceSingle(GetWorld(), CheckInGroundLocation, CheckInGroundLocation,
			CheckInGroundSize_Climb, FRotator(0.f, 0.f, 0.f),
			ParkourTraceType, false, TArray<AActor*>(), DDT_CheckInGround, InGroundHitResult, true);
	}		
	else
	{
		CheckAddZ = CheckAutoClimbToRoot;
		CheckInGroundLocation = FVector(RootSocketLocation.X, RootSocketLocation.Y, RootSocketLocation.Z + CheckAddZ);

		bInGround = UKismetSystemLibrary::BoxTraceSingle(GetWorld(), CheckInGroundLocation, CheckInGroundLocation,
			CheckInGroundSize, FRotator(0.f, 0.f, 0.f),
			ParkourTraceType, false, TArray<AActor*>(), DDT_CheckInGround, InGroundHitResult, true);
	}
		

	if (bInGround)
	{
		if (UPSFunctionLibrary::CompGameplayTagName(ParkourActionTag, TEXT("Parkour.Action.NoAction")))
		{
			bCanManuelClimb = true;
			bCanAutoClimb = true;
			ResetParkourHitResult_Tick(); // 땅에 있으므로 초기화
		}
	}
	else if(bAlwaysParkour)
	{
		if(UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.NotBusy")))
			ParkourAction();
	}
}

// FHitResult 전부 초기화
void UParkourComponent::ResetParkourHitResult()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("ResetParkourHitResult"));
#endif

	WallHitTraces.Empty();
	HopHitTraces.Empty();
	WallHitResult.Init();
	WallTopHitResult.Init();
	TopHits.Init();
	WallDepthHitResult.Init();
	WallVaultHitResult.Init();
	ClimbLedgeHitResult.Init();
	HopClimbLedgeHitResult.Init();
}


void UParkourComponent::ResetParkourHitResult_Tick()
{
	// #ifdef DEBUG_PARKOURCOMPONENT 상태일때 Tick에서 실행하는 ResetParkourHitResult Log가 뜨지 않기 위함.

	WallHitTraces.Empty();
	HopHitTraces.Empty();
	WallHitResult.Init();
	WallTopHitResult.Init();
	TopHits.Init();
	WallDepthHitResult.Init();
	WallVaultHitResult.Init();
	ClimbLedgeHitResult.Init();
	HopClimbLedgeHitResult.Init();
}

// ParkourCheckWallShape에서 사용
// Climb 상태와 밀접한 관계를 가지고 있는 함수. Parkour Climb 상태가 아니라면 그냥 설정해둔 값을 return 한다.
float UParkourComponent::GetFirstTraceHeight()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("GetFirstTraceHeight"));
#endif

	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	float Hand_r_Z = Mesh->GetSocketLocation(TEXT("hand_r")).Z;
	float Hand_l_Z = Mesh->GetSocketLocation(TEXT("hand_l")).Z;

	if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.Climb")))
	{
		for (int32 i = 0; i <= 4; i++)
		{			
			float SelectHand_Z = UKismetMathLibrary::SelectFloat(Hand_l_Z, Hand_r_Z, Hand_r_Z < Hand_l_Z);
			FVector FirstBasePos = FVector(CharacterLocation.X, CharacterLocation.Y, SelectHand_Z - CharacterHeightDiff - CharacterHandUpDiff);
			FVector FirstStartPos = FirstBasePos + (CharacterForwardVector * -20.f);
			FVector FirstEndPos = FirstBasePos + (CharacterForwardVector * (i * 20));

			FHitResult FirstTraceHitResult;
			UKismetSystemLibrary::SphereTraceSingle(GetWorld(), FirstStartPos, FirstEndPos, 5.f, ParkourTraceType, false,
				TArray<AActor*>(), DDT_GetFirstTraceHeight, FirstTraceHitResult, true);

			if (FirstTraceHitResult.bBlockingHit)
			{
				float LocalClimbHeight = 0.f;
				for (int32 j = 0; j <= 9; j++)
				{
					FVector ForwardNormalVector = UKismetMathLibrary::GetForwardVector(UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(FirstTraceHitResult.ImpactNormal));
					FVector SecondBasePos = FirstTraceHitResult.ImpactPoint + (ForwardNormalVector * 2);
					FVector SecondStartPos = SecondBasePos + FVector(0.f, 0.f, (j * 10) + 5); //  Z = Base + 5 , + 15 , + 25 ... + 95
					FVector SecondEndPos = SecondStartPos - FVector(0.f, 0.f, (j * -5) + 25); // Z = Start - 25, - 20, - 15 ... + 20

					FHitResult SecondTraceHitResult;
					UKismetSystemLibrary::SphereTraceSingle(GetWorld(), SecondStartPos, SecondEndPos, 2.5f, ParkourTraceType, false,
						TArray<AActor*>(), DDT_GetFirstTraceHeight, SecondTraceHitResult, true);

					if (SecondTraceHitResult.bBlockingHit && !SecondTraceHitResult.bStartPenetrating)
					{
						LocalClimbHeight = SecondTraceHitResult.ImpactPoint.Z;
						break;
					}
				}

				return LocalClimbHeight - CharacterLocation.Z - 4.f;
			}
		}
	}

	return CheckParkourFromCharacterRootZ;
}



/*-----------------------------------------
		Set Parkour Action / State
-------------------------------------------*/
// ParkourActionTag를 새롭게 Set 하고, 그에 해당하는 DT를 가져와서 PlayMontage 해주는 함수.
void UParkourComponent::SetParkourAction(FName NewParkourActionName)
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("SetParkourAction %s"), *NewParkourActionName.ToString());
#endif

	if (!UPSFunctionLibrary::CompGameplayTagName(ParkourActionTag, NewParkourActionName))
	{
		ParkourActionTag = UPSFunctionLibrary::GetGameplayTag(NewParkourActionName);

		AnimInstance->SetParkourAction(ParkourActionTag);

		if (UPSFunctionLibrary::CompGameplayTagName(ParkourActionTag, TEXT("Parkour.Action.NoAction")))
		{
			ResetParkourHitResult();
			return;
		}

#ifdef DEBUG_PARKOURCOMPONENT
		LOG(Warning, TEXT("------Action------"));
		LOG(Warning, TEXT("Parkour Action Tag : %s"), *ParkourActionTag.ToString());
		LOG(Warning, TEXT("--------------------"));
#endif
		if (UPSFunctionLibrary::CompGameplayTagName(ParkourActionTag, TEXT("Parkour.Action.CornerMove")))
			return;

		// ParkourDataAssets Type-> TMap<FName, class UParkourPDA*>
		EParkourGameplayTagNames ParkourGameplayEnum = UPSFunctionLibrary::ConvertFNameToParkourTagsEnum(NewParkourActionName);
		if (!ParkourDataAssets.Find(ParkourGameplayEnum))
		{
			UPSFunctionLibrary::CrashLog(TEXT("ParkourGameplayTagEnum Can't Find Action 1"), TEXT("ParkourGameplayTagEnum Can't Find"));
			return;
		}

		class UParkourPDA* ParkourPDA = ParkourDataAssets[ParkourGameplayEnum]->GetDefaultObject<UParkourPDA>();
		ParkourVariables = ParkourPDA->GetParkourDataAsset(GetCharacterState());
		
		PlayParkourMontage();
	}
}

void UParkourComponent::SetParkourAction(FGameplayTag NewParkourActionTag)
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("SetParkourAction"));
#endif

	if (ParkourActionTag != NewParkourActionTag)
	{
		AnimInstance->SetParkourAction(ParkourActionTag);

		ParkourActionTag = NewParkourActionTag;
		FName ParkourActionTagName = UPSFunctionLibrary::GetGameplayTagName(ParkourActionTag);

		if (UPSFunctionLibrary::CompGameplayTagName(ParkourActionTag, TEXT("Parkour.Action.NoAction")))
		{
			ResetParkourHitResult();
			return;
		}			

#ifdef DEBUG_PARKOURCOMPONENT
		LOG(Warning, TEXT("------Action------"));
		LOG(Warning, TEXT("Parkour Action Tag : %s"), *ParkourActionTagName.ToString());
		LOG(Warning, TEXT("--------------------"));
#endif
		if (UPSFunctionLibrary::CompGameplayTagName(ParkourActionTag, TEXT("Parkour.Action.CornerMove")))
			return;

		EParkourGameplayTagNames ParkourGameplayEnum = UPSFunctionLibrary::ConvertFNameToParkourTagsEnum(ParkourActionTagName);
		if (!ParkourDataAssets.Find(ParkourGameplayEnum))
		{
			UPSFunctionLibrary::CrashLog(TEXT("ParkourGameplayTagEnum Can't Find Action 2"), TEXT("ParkourGameplayTagEnum Can't Find"));
			return;
		}
		

		class UParkourPDA* ParkourPDA = ParkourDataAssets[ParkourGameplayEnum]->GetDefaultObject<UParkourPDA>();
		ParkourVariables = ParkourPDA->GetParkourDataAsset(GetCharacterState());

		PlayParkourMontage();
	}
}

void UParkourComponent::PlayParkourMontage()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("PlayParkourMontage"));
#endif

	if (!OwnerCharacter || !AnimInstance)
	{
		LOG(Warning, TEXT("OwnerCharacter, AnimInstance Can't Find"));
		return;
	}


	// 파쿠르 몽타주를 실행하면서 스테이트 변경
	// ParkourInState -> 해당 몽타주의 상태
	// ParkourInOut -> 몽타주가 끝나면 돼야할 상태 
	// ex)In : Mantle -> Out : NotBusy / in : ReachLedge -> Out :Climb
	SetParkourState(ParkourVariables.ParkourInState);

	bCanParkour = false; // 파쿠르 실행중

	// Play MotionWarping
	MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(TEXT("ParkourTop"),
		FindWarpTopLocation(ParkourVariables.WarpTopXOffset, ParkourVariables.WarpTopZOffset), WallRotation);

	MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(TEXT("ParkourDepth"),
		FindWarpDepthLocation(ParkourVariables.WarpDepthXOffset, ParkourVariables.WarpDepthZOffset), WallRotation);
	
	MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(TEXT("ParkourVault"),
		FindWarpVaultLocation(ParkourVariables.WarpVaultXOffset, ParkourVariables.WarpVaultZOffset), WallRotation);

	MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(TEXT("ParkourMantle"),
		FindWarpMantleLocation(ParkourVariables.WarpMantleXOffset, ParkourVariables.WarpMantleZOffset), WallRotation);

	MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(TEXT("ParkourTopTemp"),
		FindWarpTopLocation_Temp(ParkourVariables.WarpTopXOffset_Temp, ParkourVariables.WarpTopZOffset_Temp), WallRotation);


	AnimInstance->Montage_Play(ParkourVariables.ParkourMontage, 1.f, EMontagePlayReturnType::MontageLength, MontageStartTime);
	
}

// Parkour State에 따른 Movement Component 상태 변화
// 또는 Privious State에 따른 각 ParkourState 상태에 따라 Camera, Spring Arm 상태 변화
void UParkourComponent::SetParkourState(FGameplayTag NewParkourState)
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("SetParkourState"));
#endif

	if (ParkourStateTag == NewParkourState)
		return;

	PreviousStateCameraSetting(ParkourStateTag, NewParkourState);
	
	ParkourStateTag = NewParkourState;
	AnimInstance->SetParkourState(NewParkourState);

	/* ★ 현재 파쿠르 상태를 표시 하려면 여기서 widget 셋업 */
	BP_SetParkourStateWidget();

	FindMontageStartTime();

	if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.Climb")))
	{
		ParkourStateSettings(ECollisionEnabled::NoCollision, 
			EMovementMode::MOVE_Flying, 
			ParkourRotationRate_Climb,
			true, 
			true);
	}
	else if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.Mantle")))
	{
		ParkourStateSettings(ECollisionEnabled::NoCollision,
			EMovementMode::MOVE_Flying,
			ParkourRotationRate_Matntle,
			true,
			false);
	}
	else if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.Vault")))
	{
		ParkourStateSettings(ECollisionEnabled::NoCollision,
			EMovementMode::MOVE_Flying,
			ParkourRotationRate_Vault,
			true,
			false);
	}
	else if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.NotBusy")))
	{
		ParkourStateSettings(ECollisionEnabled::QueryAndPhysics,
			EMovementMode::MOVE_Walking,
			ParkourRotationRate_Default,
			true,
			false);
	}
	else if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.ReachLedge")))
	{
		ParkourStateSettings(ECollisionEnabled::NoCollision,
			EMovementMode::MOVE_Flying,
			ParkourRotationRate_ReachLedge,
			true,
			false);
	}
}

/* Hand Location 또는 Spring Arm Component의 위치 및 길이 조절*/
void UParkourComponent::PreviousStateCameraSetting(FGameplayTag PreviousState, FGameplayTag NewParkourState)
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("PreviousStateCameraSetting"));
	LOG(Warning, TEXT("--------------------------------------"));
	FName PreviouseStateName = UPSFunctionLibrary::GetGameplayTagName(PreviousState);
	FName NewParkourStateName = UPSFunctionLibrary::GetGameplayTagName(NewParkourState);
	LOG(Error, TEXT("PreviousState : %s -> NewParkourState : %s"), *PreviouseStateName.ToString(), *NewParkourStateName.ToString());
	LOG(Warning, TEXT("--------------------------------------"));
#endif


	if (UPSFunctionLibrary::CompGameplayTagName(PreviousState, TEXT("Parkour.State.Climb")))
	{
		if (UPSFunctionLibrary::CompGameplayTagName(NewParkourState, TEXT("Parkour.State.ReachLedge")))
		{
			if (!OwnerCharacter || !OwnerCharacter->GetMesh())
				return;

			LastClimbHand_RLocation = OwnerCharacter->GetMesh()->GetSocketLocation(RightHandSocketName);
			LastClimbHand_LLocation = OwnerCharacter->GetMesh()->GetSocketLocation(LeftHandSocketName);
		}
		else if (UPSFunctionLibrary::CompGameplayTagName(NewParkourState, TEXT("Parkour.State.Mantle")))
		{
			// 맨 처음 위치 및 길이로 조절
			LerpTargetCameraRelativeLocation = FirstCameraLocation;
			LerpTargetArmLength = FirstTargetArmLength;
			LerpCameraTimerStart(CameraCurveTimeMax);
		}
		else if (UPSFunctionLibrary::CompGameplayTagName(NewParkourState, TEXT("Parkour.State.NotBusy")))
		{
			// 맨 처음 위치 및 길이로 조절
			LerpTargetCameraRelativeLocation = FirstCameraLocation;
			LerpTargetArmLength = FirstTargetArmLength;
			LerpCameraTimerStart(CameraCurveTimeMax);
		}

	}
	// Climb 상태가 된 경우, 카메라 연출
	else if(UPSFunctionLibrary::CompGameplayTagName(PreviousState, TEXT("Parkour.State.NotBusy")))
	{
		if (UPSFunctionLibrary::CompGameplayTagName(NewParkourState, TEXT("Parkour.State.ReachLedge")))
		{
			LerpTargetCameraRelativeLocation = ReachLedge_CameraLocation;
			LerpTargetArmLength = ReachLedge_TargetArmLength;
			LerpCameraTimerStart(CameraCurveTimeMax);
		}
	}
}

void UParkourComponent::LerpCameraTimerStart(float FinishTime)
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("LerpCameraTimerStart"));
#endif

	GetWorld()->GetTimerManager().SetTimer(CameraLerpFinishHandle, this, &UParkourComponent::LerpCameraPositionFinish, FinishTime, false);
	GetWorld()->GetTimerManager().SetTimer(CameraLerpHandle, this, &UParkourComponent::LerpCameraPosition, GetWorld()->GetDeltaSeconds(), true);
	
}

// LerpTargetCameraRelativeLocation, LerpTargetArmLength 변수에 저장된 값으로 Interp 실행
void UParkourComponent::LerpCameraPosition()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("LerpCameraPosition"));
#endif

	if (ParkourCameraLerpCurve)
		CameraCurveAlpha = ParkourCameraLerpCurve->GetFloatValue(GetWorld()->GetTimerManager().GetTimerElapsed(CameraLerpFinishHandle));
	else
		CameraCurveAlpha = 0.5f;


	// Camera Location 변경
	FVector CameraLocation = SpringArmComponent->GetRelativeLocation();
	FVector LocationDiff = LerpTargetCameraRelativeLocation - CameraLocation;
	SpringArmComponent->SetRelativeLocation(CameraLocation + (LocationDiff * CameraCurveAlpha));

	// Target Arm Length 변경
	float TargetArmLengthDiff = LerpTargetArmLength - SpringArmComponent->TargetArmLength;
	SpringArmComponent->TargetArmLength = SpringArmComponent->TargetArmLength + (TargetArmLengthDiff * CameraCurveAlpha);

}

void UParkourComponent::LerpCameraPositionFinish()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("LerpCameraPositionFinish"));
#endif

	GetWorld()->GetTimerManager().ClearTimer(CameraLerpFinishHandle);
	GetWorld()->GetTimerManager().ClearTimer(CameraLerpHandle);	
	CameraCurveAlpha = 0.f;
}

void UParkourComponent::FindMontageStartTime()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("FindMontageStartTime"));
#endif

	if (ParkourActionTag == UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.Action.Climb")) ||
		ParkourActionTag == UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.Action.FreeHangClimb")))

	{
		// InGround 상태가 아니라면 다른 StartPos를 사용하는 옵션
		if (!bInGround)
			MontageStartTime = ParkourVariables.FallingMontageStartPos;
		else
			MontageStartTime = ParkourVariables.MontageStartPos;
	}
	else
		MontageStartTime = ParkourVariables.MontageStartPos;
}

// Parkour State Tag가 변경되면서 변경되어야할 콜리전 및 무브먼트, 스프링 암의 콜리전 테스트 등의 설정을 맡는 함수.
void UParkourComponent::ParkourStateSettings(ECollisionEnabled::Type NewCollisionType, EMovementMode NewMovementMode, FRotator NewRotationRate, bool bDoCollisionTest, bool bStopMovementImmediately)
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("ParkourStateSettings"));
#endif

	CapsuleComponent->SetCollisionEnabled(NewCollisionType);
	CharacterMovement->SetMovementMode(NewMovementMode);
	CharacterMovement->RotationRate = NewRotationRate;
	SpringArmComponent->bDoCollisionTest = bDoCollisionTest;
	if (bStopMovementImmediately)
		CharacterMovement->StopMovementImmediately();
}




/*-------------------------------------------------------
		OnMontageBlendingOut AddDynamic
---------------------------------------------------------*/;
void UParkourComponent::BlendingOut_SetParkourState(UAnimMontage* animMontage, bool bInterrupted)
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("BlendingOut_SetParkourState"));
#endif

	// ParkourOutState
	// 블렌딩 아웃을 하면서 몽타주가 끝나면서 변경해야할 Parkour State 변경
	SetParkourState(ParkourVariables.ParkourOutState);
	SetParkourAction(TEXT("Parkour.Action.NoAction"));	
	bCanParkour = true;
}

void UParkourComponent::ParkourMontageEnded(UAnimMontage* animMontage, bool bInterrupted)
{
	
}



/*------------------------------------------------------
		Motaion Warping Location Calculator
--------------------------------------------------------*/
// 맨처음 벽의 Top Warp 위치를 계산하는 함수
FVector UParkourComponent::FindWarpTopLocation(float WarpXOffset, float WarpZOffset)
{
	return WallTopHitResult.ImpactPoint + (GetForwardVector(WallRotation) * WarpXOffset)
		+ FVector(0.f, 0.f, WarpZOffset);
}

// 장애물의 끝부분 위치를 기준으로 한 위치 계산
FVector UParkourComponent::FindWarpDepthLocation(float WarpXOffset, float WarpZOffset)
{
	return WallDepthHitResult.ImpactPoint + (GetForwardVector(WallRotation) * WarpXOffset)
		+ FVector(0.f, 0.f, WarpZOffset);
}

// 장애물 착지 지점 위치를 기준으로 한 계산
FVector UParkourComponent::FindWarpVaultLocation(float WarpXOffset, float WarpZOffset)
{

	return WallVaultHitResult.ImpactPoint + (GetForwardVector(WallRotation) * WarpXOffset)
		+ FVector(0.f, 0.f, WarpZOffset);
}


// Mantle이 가능한지 우선 장애물의 WarpXOffset 위치만큼 앞을 검사하여 캐릭터가 발을 디딜수 있는지 판단
FVector UParkourComponent::FindWarpMantleLocation(float WarpXOffset, float WarpZOffset)
{
	FVector BasePos = WallTopHitResult.ImpactPoint + GetForwardVector(WallRotation) * WarpXOffset;
	FVector StartPos = BasePos + FVector(0.f, 0.f, CheckMantleHeight_StartZ);
	FVector EndPos = BasePos - FVector(0.f, 0.f, CheckMantleHeight_EndZ);

	FHitResult MantleHitResult;
	bool bMantleHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartPos, EndPos, 5.f, ParkourTraceType, false, TArray<AActor*>(), DDT_FindWarpMantleLocation, MantleHitResult, true);

	if (bMantleHit)
		return MantleHitResult.ImpactPoint + FVector(0.f, 0.f, WarpZOffset);
	else
		return WallTopHitResult.ImpactPoint + FVector(0.f, 0.f, WarpZOffset);
}

// Top Location이 필요한데 다른 값으로 필요한 경우를 위한 임시 함수
FVector UParkourComponent::FindWarpTopLocation_Temp(float WarpXOffset, float WarpZOffset)
{
	return WallTopHitResult.ImpactPoint + GetForwardVector(WallRotation) * WarpXOffset
		+ FVector(0.f, 0.f, WarpZOffset);
}



/*-------------------------------
		Check Climb Style
---------------------------------*/
void UParkourComponent::CheckClimb()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("CheckClimb"));
#endif

	CheckClimbStyle();
	ClimbLedgeResult();
	if (CheckAirHang())
	{
		if (UPSFunctionLibrary::CompGameplayTagName(ClimbStyleTag, TEXT("Parkour.ClimbStyle.Braced")))
			SetParkourAction(TEXT("Parkour.Action.FallingBraced"));
		else
			SetParkourAction(TEXT("Parkour.Action.FallingFreeHang"));
	}
	else
	{
		if (UPSFunctionLibrary::CompGameplayTagName(ClimbStyleTag, TEXT("Parkour.ClimbStyle.Braced")))
			SetParkourAction(TEXT("Parkour.Action.Climb"));
		else
			SetParkourAction(TEXT("Parkour.Action.FreeHangClimb"));
	}
}

void UParkourComponent::CheckClimbStyle()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("CheckClimbStyle"));
#endif

	// Braced모드인지 FreeHang모드인지도 체크
	FVector BasePos = WallTopHitResult.ImpactPoint + FVector(0.f, 0.f, CheckClimbStyle_ZHeight);
	FVector ForwardVector = UKismetMathLibrary::GetForwardVector(WallRotation);
	FVector StartPos = BasePos + ForwardVector * -10.f;
	FVector EndPos = BasePos + ForwardVector * 30.f;

	FHitResult CheckClimbResult;
	if (UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartPos, EndPos,
		CheckClimbStyleRadius,
		ParkourTraceType, false, TArray<AActor*>(), DDT_CheckClimbStyle, CheckClimbResult, true))
	{
		SetClimbStyle(TEXT("Parkour.ClimbStyle.Braced"));
	}
	else
		SetClimbStyle(TEXT("Parkour.ClimbStyle.FreeHang"));
}

void UParkourComponent::SetClimbStyle(FName ClimbStyleName)
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("SetClimbStyle"));
#endif

	// 현재 ClimbStyle과 같은경우 ret
	if (UPSFunctionLibrary::CompGameplayTagName(ClimbStyleTag, ClimbStyleName))
		return;

	ClimbStyleTag = UPSFunctionLibrary::GetGameplayTag(ClimbStyleName);
	AnimInstance->SetClimbStyle(ClimbStyleTag);
}

// ClimbMovement를 위해 벽 모서리의 정면과 수직을 검사
void UParkourComponent::ClimbLedgeResult()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("ClimbLedgeResult"));
#endif

	// 캐릭터와의 거리가 가장 짧은 벽의 정보를 가진 Wall Hit Result를 이용함.
	// 캐릭터 바로 앞 장애물의 ForwardVector를 이용하기 위함.
	FVector ForwardNormalVector = UKismetMathLibrary::GetForwardVector(UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(WallHitResult.ImpactNormal));
	FVector WallHitStartPos = WallHitResult.ImpactPoint + (ForwardNormalVector * -30.f);
	FVector WallHitEndPos = WallHitResult.ImpactPoint + (ForwardNormalVector * 30.f);

	FHitResult ClimbLedgeResult_First;
	UKismetSystemLibrary::SphereTraceSingle(GetWorld(), WallHitStartPos, WallHitEndPos, 5.f, ParkourTraceType, false, TArray<AActor*>(), DDT_ClimbLedgeResultInspection, ClimbLedgeResult_First, true);

	WallRotation = UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(ClimbLedgeResult_First.ImpactNormal);
	FVector ForwardVector = UKismetMathLibrary::GetForwardVector(WallRotation);
	FVector StartPos = ClimbLedgeResult_First.ImpactPoint + (ForwardVector * 2.f) + FVector(0.f, 0.f, 5.f);
	FVector EndPos = ClimbLedgeResult_First.ImpactPoint + (ForwardVector * 2.f) - FVector(0.f, 0.f, 50.f);

	FHitResult ClimbLedgeResult_Second;
	UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartPos, EndPos, 5.f, ParkourTraceType, false, TArray<AActor*>(), DDT_ClimbLedgeResultInspection, ClimbLedgeResult_Second, true);
	
	ClimbLedgeHitResult = ClimbLedgeResult_First;
	ClimbLedgeHitResult.Location = ClimbLedgeResult_Second.Location; // 교체
	ClimbLedgeHitResult.ImpactPoint.Z = ClimbLedgeResult_Second.ImpactPoint.Z; // 교체
	
}


/*---------------------------
		Check Surface
----------------------------*/
// Mantle 할 수있는 표면인지 Check
// 꼭 지상에 발을 딛고있는 상태뿐만 아닌, Climb 상태에서 올라갈때도 Mantle이 적용된다.
bool UParkourComponent::CheckMantleSurface()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Error, TEXT("CheckMantleSurface"));
#endif

	FVector TraceVector = WallTopHitResult.ImpactPoint + FVector(0.f, 0.f, CharacterCapsuleCompHalfHeight + CheckMantleHeight);
	FHitResult MantleHitResult;
	bool bHit = UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(),
		TraceVector, TraceVector,
		CheckMantleCapsuleTraceRadius,
		CharacterCapsuleCompHalfHeight - CheckMantleHeight,
		ParkourTraceType, false, TArray<AActor*>(), DDT_CheckMantleSurface, MantleHitResult, true);
	
	return !bHit;
}

// Climb 할 수있는 표면인지 Check
bool UParkourComponent::CheckClimbSurface()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Error, TEXT("CheckClimbSurface"));
#endif

	FVector TraceVector = WallTopHitResult.ImpactPoint + FVector(0.f, 0.f, -CharacterCapsuleCompHalfHeight)
		+ (UKismetMathLibrary::GetForwardVector(WallRotation) * -CheckClimbCapsuleTraceForwardDistance);
	FHitResult ClimbHitResult;
	bool bHit = UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(),
		TraceVector, TraceVector,
		CharacterCapsuleCompRadius,
		CharacterCapsuleCompHalfHeight,
		ParkourTraceType, false, TArray<AActor*>(), DDT_CheckClimbSurface, ClimbHitResult, true);

	return !bHit;
}

// Vault 할 때 지나갈 공간이 있는지 Check
bool UParkourComponent::CheckVaultSurface()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Error, TEXT("CheckVaultSurface"));
#endif

	FVector TraceVector = WallTopHitResult.ImpactPoint + FVector(0.f, 0.f, (CharacterCapsuleCompHalfHeight / 2) + CheckVaultHeight);
	FHitResult VaultHitResult;
	bool bHit = UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(),
		TraceVector, TraceVector,
		CheckVaultCapsuleTraceRadius,
		CheckVaultCapsuleTraceHalfHeight, // Vault를 시전할 때 캐릭터가 혹시 위쪽 장애물에 닿는지 검사하기 위함
		ParkourTraceType, false, TArray<AActor*>(), DDT_CheckVaultSurface, VaultHitResult, true);

	return !bHit;
}

bool UParkourComponent::CheckClimbMovementSurface(FHitResult MovementHitResult)
{
	FRotator ArrowActorWorldRotation = ArrowActor->GetArrowWorldRotation();
	FVector ArrowForwardVector = GetForwardVector(ArrowActorWorldRotation);
	FVector ArrowRightVector = GetRightVector(ArrowActorWorldRotation);

	// 입력방향 체크
	float HorizontalAxisLength = GetHorizontalAxis() * ClimbMovementObstacleCheckDistance;

	FVector BasePos = MovementHitResult.ImpactPoint + (ArrowRightVector * HorizontalAxisLength) + FVector(0.f, 0.f, -90.f);
	FVector ObstacleCheckStartPos = BasePos + (ArrowForwardVector * -40.f);
	FVector ObstacleCheckEndPos = BasePos + (ArrowForwardVector * -25.f);

	FHitResult CheckClimbMovementSurfaceHitResult;
	bool bObstacleHit = UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(), ObstacleCheckStartPos, ObstacleCheckEndPos,
		CheckClimbMovementSurface_CapsuleTraceRadius, CheckClimbMovementSurface_CapsuleTraceHalfHeight,
		ParkourTraceType, false, TArray<AActor*>(), DDT_CheckClimbMovementSurface,
		CheckClimbMovementSurfaceHitResult, true);

	return !bObstacleHit;
}


/*----------------
		IK
------------------*/
void UParkourComponent::MontageLeftIK(bool bLastClimbLocation)
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("MontageLeftIK"));
#endif

	if (!bLastClimbLocation)
	{
		AnimInstance->SetLeftHandLedgeLocation(LastClimbHand_LLocation);
	}
	else if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.ReachLedge")))
	{
		MontageLeftHandIK();
		MontageLeftFootIK();
	}
}

void UParkourComponent::MontageLeftHandIK()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("MontageLeftHandIK"));
#endif

	// ClimbLedgeHitResult : 찾아낸 오브젝트를 수직으로 다시 재검사하여 잡을 수 있는 위치의 정보.
	FHitResult LedgeResult = ClimbLedgeHitResult;
	FVector WallForwardVector = GetForwardVector(WallRotation);
	FVector WallRightVector = GetRightVector(WallRotation);

	if(LedgeResult.bBlockingHit)
	{
		/* 최종적으로 AnimInstance->SetLeftHandLedgeLocation / Rotation을 하는 위치 */
		FVector TargetLeftHandLedgeLocation;
		FRotator TagetLeftHandLedgeRotation;
		int32 IKEndIndex = FMath::Abs(ClimbIKHandSpace) / 4;

		for (int32 i = 0; i <= 4; i++)
		{
			/* WallRotation의 LedgeResult.ImpactPoint + Forward Vector 방향을 기준으로 
			Left로 LeftIndex * -2만큼, -CheckClimbWidth 만큼 Trace*/ 
			int32 LeftIndex = i * IKEndIndex;

			FVector WallRotation_Left = WallRightVector * (-ClimbIKHandSpace + LeftIndex);  // Left는 음수, Right는 양수
			FVector StartPos = LedgeResult.ImpactPoint + (WallForwardVector * -CheckClimbForward) + WallRotation_Left;
			FVector EndPos = LedgeResult.ImpactPoint + (WallForwardVector * CheckClimbForward) + WallRotation_Left;
			
			FHitResult LeftClimbHitResult;
			UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartPos, EndPos, 5.f, ParkourTraceType, false, TArray<AActor*>(), DDT_LeftClimbHandIK, LeftClimbHitResult, true);

			// True인 경우 : IK가 가능함.
			if (LeftClimbHitResult.bBlockingHit)
			{
				FRotator LeftClimbHitResultNormalRotator = UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(LeftClimbHitResult.ImpactNormal);
				FVector LeftClimbHitResultForwardVector = GetForwardVector(LeftClimbHitResultNormalRotator);
			
				FHitResult LeftClimbHitResult_ZResult;
				// ImpactNormal로 Rotation방향의 ForwardVector * 2 + ImpactPoint의 위치부터 j번만큼 j * 5의 Z값위치를 검사한다.
				for (int32 j = 0; j <= 5; j++)
				{
					// 잡을 수 있는 위치에서부터 (ImpactNormal_ForwardVector * 2.f) + FVector(0.f, 0.f, j * 5) 간격만큼 수직 아래쪽으로 Trace				
					FVector ImpactNormal_StartPos = LeftClimbHitResult.ImpactPoint + (LeftClimbHitResultForwardVector * 2.f) + FVector(0.f, 0.f, j * 5);
					FVector ImpactNormal_EndPos = ImpactNormal_StartPos - FVector(0.f, 0.f, (j * 5) + 50.f);
					UKismetSystemLibrary::SphereTraceSingle(GetWorld(), ImpactNormal_StartPos, ImpactNormal_EndPos, 2.5f, ParkourTraceType, false, TArray<AActor*>(), DDT_LeftClimbHandIK, LeftClimbHitResult_ZResult, true);
				

					// True일시 : bStartPenetrating일시 IK를 해야하는 위치 및 회전값 전달 
					// 중요한 점은 LeftClimbHitResult_ZResult이 아닌 위의 LeftClimbHitResult.ImpactPoint 값을 이용한다는 점이다.
					// bStartPenetrating로 판단하는 이유는 bBlockingHit와 달리 bStartPenetratingrk true라는 것은 Blocking이 아닌 겹쳐져서 침투가 되었다는 것이기때문에
					// 현재 검사하는 위치보다 벽이 더 높거나 하는 등의 이유로 더 윗부분을 검사 해야한다는 것을 파악할 수 있기 때문이다.
					if (LeftClimbHitResult_ZResult.bStartPenetrating)
					{
						// 마지막 까지 진행하여 
						if (j == 5)
						{
							TargetLeftHandLedgeLocation =
								FVector(LeftClimbHitResult.ImpactPoint.X, LeftClimbHitResult.ImpactPoint.Y, LeftClimbHitResult.ImpactPoint.Z - 9.f);

							TagetLeftHandLedgeRotation = FRotator(LeftClimbHitResultNormalRotator + Additive_LeftClimbIKHandRotation); // 애니메이션마다 회전값 다름

							LastLeftHandIKTargetLocation = TargetLeftHandLedgeLocation;

							AnimInstance->SetLeftHandLedgeLocation(TargetLeftHandLedgeLocation);
							AnimInstance->SetLeftHandLedgeRotation(TagetLeftHandLedgeRotation);
						}
					}
					else if (LeftClimbHitResult_ZResult.bBlockingHit)
					{
						// CharacterHandThickness을 빼는 이유는 벽에 딱 맞춰서 IK위치가 계산되었기 때문에 손 두께를 생각해서 조금 더 뒤로 가도록 함
						float HandFrontDiff = UKismetMathLibrary::SelectFloat(BracedHandFrontDiff, 0.f, UPSFunctionLibrary::CompGameplayTagName(ClimbStyleTag, TEXT("Parkour.ClimbStyle.Braced")))
							+ -CharacterHandThickness;

						FVector AddHandFrontLocation = LeftClimbHitResult.ImpactPoint + (LeftClimbHitResultForwardVector * HandFrontDiff);

						// Z축에서 CharacterHeightDiff, CharacterHandUpDiff 값을 더해서 최종적인 IK Location 구하기
						TargetLeftHandLedgeLocation =
							FVector(AddHandFrontLocation.X, AddHandFrontLocation.Y,
								(LeftClimbHitResult.ImpactPoint.Z + CharacterHeightDiff + CharacterHandUpDiff - 9.f));

						TagetLeftHandLedgeRotation = FRotator(LeftClimbHitResultNormalRotator + Additive_LeftClimbIKHandRotation);

						LastLeftHandIKTargetLocation = TargetLeftHandLedgeLocation;
						LastLeftHandIKTargetRotation = TagetLeftHandLedgeRotation;

						AnimInstance->SetLeftHandLedgeLocation(TargetLeftHandLedgeLocation);
						AnimInstance->SetLeftHandLedgeRotation(TagetLeftHandLedgeRotation);

						break;						
					}					
				}

				break; // 필수
			}
		}	
	}	
}

void UParkourComponent::MontageLeftFootIK()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("MontageLeftFootIK"));
#endif

	// Foot IK는 Climb 상태가 Braced일 때만 유효하다.
	if (UPSFunctionLibrary::CompGameplayTagName(ClimbStyleTag, TEXT("Parkour.ClimbStyle.Braced")))
	{
		FVector WallForwardVector = GetForwardVector(WallRotation);
		FVector WallRightVector = GetRightVector(WallRotation);

		// 수동Z축에 FootIK하는 위치 빼주기
		FVector FootIKLocation = FVector(0.f, 0.f, CharacterHeightDiff - CharacterFootIKLocation);

		FHitResult LedgeResult = ClimbLedgeHitResult;
		for (int32 cnt = 0; cnt <= 2; cnt++)
		{					
			// Z축으로 더해가며, Left Foot이 위치할 방향에서 Z축으로 순차적으로 Trace 진행 (필요한 결과를 얻으면 반복 Trace할 필요x)
			FVector BasePos =
				FVector(0.f, 0.f, cnt * 5) + LedgeResult.ImpactPoint
				+ FootIKLocation
				+ (WallRightVector * -CharacterLeftFootLocation);
						
			FVector StartPos = BasePos + (WallForwardVector * -30.f);
			FVector EndPos = BasePos + (WallForwardVector * 30.f);

			FHitResult ClimbFootHitResult;
			bool bHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartPos, EndPos, 10.f, ParkourTraceType, false, TArray<AActor*>(), DDT_LeftClimbFootIK, ClimbFootHitResult, true);
			if (bHit)
			{
				//찾아낸 Foot IK Location에서 사용자가 지정 할 수있는 BracedFootAddForward_Left 변수의 값까지 더하여 애님 인스턴스에 최종 위치 전달
				FVector ClimbFootHitResult_ForwardVector = GetForwardVector(UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(ClimbFootHitResult.ImpactNormal));
				FVector LeftFootLocation = ClimbFootHitResult.ImpactPoint + (ClimbFootHitResult_ForwardVector * -BracedFootAddThickness_Left);
				
				AnimInstance->SetLeftFootLocation(LeftFootLocation);
				break;
			}
			else if (cnt == 2) // false : 찾지 못함
			{
				// 더 넓은 범위로 검사
				for (int cnt2 = 0; cnt2 <= 4; cnt2++)
				{
					// for문 인덱스 값을 이용하여 Z축을 더해가며 검사
					FVector SecondBasePos = LedgeResult.ImpactPoint + FootIKLocation + FVector(0.f, 0.f,  (cnt2 * 5));
					FVector SecondStartPos = SecondBasePos + (WallForwardVector * -30.f);
					FVector SecondEndPos = SecondBasePos + (WallForwardVector * 30.f);

					FHitResult SecondClimbFootHitResult;
					bool bSecondHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), SecondStartPos, SecondEndPos, 15.f, ParkourTraceType, false, TArray<AActor*>(), DDT_LeftClimbFootIK, SecondClimbFootHitResult, true);

					if (bSecondHit)
					{
						FVector SecondClimbFootHitResult_ForwardVector = GetForwardVector(UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(SecondClimbFootHitResult.ImpactNormal));
						FVector LeftFootLocation = SecondClimbFootHitResult.ImpactPoint + (SecondClimbFootHitResult_ForwardVector * -BracedFootAddThickness_Left);

						LastLeftFootIKTargetLocation = LeftFootLocation;
						AnimInstance->SetLeftFootLocation(LeftFootLocation);
						break;
					}
				}
			}
		}
	}
}

void UParkourComponent::MontageRightIK(bool bLastClimbLocation)
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("MontageRightIK"));
#endif

	if (!bLastClimbLocation)
	{
		if (AnimInstance)
			AnimInstance->SetRightHandLedgeLocation(LastClimbHand_RLocation);
	}
	else if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.ReachLedge")))
	{
		MontageRightHandIK();
		MontageRightFootIK();
	}
}

void UParkourComponent::MontageRightHandIK()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("MontageRightHandIK"));
#endif

	FHitResult LedgeResult = ClimbLedgeHitResult;
	FVector WallForwardVector = GetForwardVector(WallRotation);
	FVector WallRightVector = GetRightVector(WallRotation);
	
	if (LedgeResult.bBlockingHit)
	{
		/* 최종적으로 AnimInstance->SetLeftHandLedgeLocation / Rotation을 하는 위치 */
		FVector TargetRightHandLedgeLocation;
		FRotator TagetRightHandLedgeRotation;
		int32 IKEndIndex = FMath::Abs(ClimbIKHandSpace) / 4;

		for (int32 i = 0; i <= 4; i++)
		{
			int32 RightIndex = i * IKEndIndex;
		
			FVector WallRotation_Right = WallRightVector * (ClimbIKHandSpace - RightIndex);
			FVector StartPos = LedgeResult.ImpactPoint + (WallForwardVector * -CheckClimbForward) + WallRotation_Right;
			FVector EndPos = LedgeResult.ImpactPoint + (WallForwardVector * CheckClimbForward) + WallRotation_Right;
			
			FHitResult RightClimbHitResult;
			UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartPos, EndPos, 5.f, ParkourTraceType, false, TArray<AActor*>(), DDT_RightClimbHandIK, RightClimbHitResult, true);

			if (RightClimbHitResult.bBlockingHit)
			{
				FRotator RightClimbHitResultNormalRotator = UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(RightClimbHitResult.ImpactNormal);
				FVector RightClimbHitResultForwardVector = GetForwardVector(RightClimbHitResultNormalRotator);
				
				FHitResult RightClimbHitResult_ZResult;
				// ImpactNormal로 Rotation방향의 ForwardVector * 2 + ImpactPoint의 위치부터 j번만큼 j * 5의 Z값위치를 검사한다.
				for (int32 j = 0; j <= 5; j++)
				{
					FVector ImpactNormal_StartPos = RightClimbHitResult.ImpactPoint + (RightClimbHitResultForwardVector * 2.f) + FVector(0.f, 0.f, j * 5);
					FVector ImpactNormal_EndPos = ImpactNormal_StartPos - FVector(0.f, 0.f, (j * 5) +50.f);
					
					UKismetSystemLibrary::SphereTraceSingle(GetWorld(), ImpactNormal_StartPos, ImpactNormal_EndPos, 2.5f, ParkourTraceType, false, TArray<AActor*>(), DDT_RightClimbHandIK, RightClimbHitResult_ZResult, true);
					if (RightClimbHitResult_ZResult.bStartPenetrating)
					{
						if (j == 5)
						{
							TargetRightHandLedgeLocation =
								FVector(RightClimbHitResult.ImpactPoint.X, RightClimbHitResult.ImpactPoint.Y, RightClimbHitResult.ImpactPoint.Z - 9.f);

							TagetRightHandLedgeRotation = FRotator(RightClimbHitResultNormalRotator + Additive_RightClimbIKHandRotation); // 애니메이션마다 회전값이 다르다.

							LastRightHandIKTargetLocation = TargetRightHandLedgeLocation;

							AnimInstance->SetRightHandLedgeLocation(TargetRightHandLedgeLocation);
							AnimInstance->SetRightHandLedgeRotation(TagetRightHandLedgeRotation);
						}
					}
					else if (RightClimbHitResult_ZResult.bBlockingHit)
					{
						// CharacterHandThickness을 빼는 이유는 벽에 딱 맞춰서 IK위치가 계산되었기 때문에 손 두께를 생각해서 조금 더 뒤로 가도록 함
						float HandFrontDiff = UKismetMathLibrary::SelectFloat(BracedHandFrontDiff, 0.f, UPSFunctionLibrary::CompGameplayTagName(ClimbStyleTag, TEXT("Parkour.ClimbStyle.Braced")))
							+ -CharacterHandThickness;
						FVector AddHandFrontLocation = RightClimbHitResult.ImpactPoint + (RightClimbHitResultForwardVector * HandFrontDiff);

						// Z축에서 CharacterHeightDiff, CharacterHandUpDiff 값을 더 해서 IK 진행
						TargetRightHandLedgeLocation =
							FVector(AddHandFrontLocation.X, AddHandFrontLocation.Y, (RightClimbHitResult.ImpactPoint.Z + CharacterHeightDiff + CharacterHandUpDiff - 9.f));

						TagetRightHandLedgeRotation = FRotator(RightClimbHitResultNormalRotator + Additive_RightClimbIKHandRotation);

						LastRightHandIKTargetLocation = TargetRightHandLedgeLocation;
						LastRightHandIKTargetRotation = TagetRightHandLedgeRotation;

						AnimInstance->SetRightHandLedgeLocation(TargetRightHandLedgeLocation);
						AnimInstance->SetRightHandLedgeRotation(TagetRightHandLedgeRotation);
						break;
					}
				}

				break; // 필수
			}
		}
	}
}

void UParkourComponent::MontageRightFootIK()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("MontageRightFootIK"));
#endif

	if (UPSFunctionLibrary::CompGameplayTagName(ClimbStyleTag, TEXT("Parkour.ClimbStyle.Braced")))
	{
		FVector WallForwardVector = GetForwardVector(WallRotation);
		FVector WallRightVector = GetRightVector(WallRotation);

		// 수동Z축에 FootIK하는 위치 빼주기
		FVector FootIKLocation = FVector(0.f, 0.f, CharacterHeightDiff - CharacterFootIKLocation);

		FHitResult LedgeResult = ClimbLedgeHitResult;
		for (int32 cnt = 0; cnt <= 2; cnt++)
		{
			FVector BasePos =
				FVector(0.f, 0.f, cnt * 5) + LedgeResult.ImpactPoint
				+ FootIKLocation
				+ (WallRightVector * CharacterFootLocation_Right);

			FVector StartPos = BasePos + (WallForwardVector * -30.f);
			FVector EndPos = BasePos + (WallForwardVector * 30.f);

			FHitResult ClimbFootHitResult;
			bool bHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartPos, EndPos, 6.f, ParkourTraceType, false, TArray<AActor*>(), DDT_RightClimbFootIK, ClimbFootHitResult, true);
			if (bHit)
			{
				FVector ClimbFootHitResult_ForwardVector = GetForwardVector(UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(ClimbFootHitResult.ImpactNormal));
				FVector RightFootLocation = ClimbFootHitResult.ImpactPoint + (ClimbFootHitResult_ForwardVector * -BracedFootAddThickness_Right);
				
				AnimInstance->SetRightFootLocation(RightFootLocation);
				break;
			}
			else // false
			{
				if (cnt == 2)
				{
					for (int cnt2 = 0; cnt2 <= 4; cnt2++)
					{
						// for문 인덱스 값을 이용하여 Z축을 더해가며 검사
						FVector SecondBasePos = LedgeResult.ImpactPoint + FootIKLocation + FVector(0.f, 0.f, (cnt2 * 5));
						FVector SecondStartPos = SecondBasePos + (WallForwardVector * -30.f);
						FVector SecondEndPos = SecondBasePos + (WallForwardVector * 30.f);

						FHitResult SecondClimbFootHitResult;
						bool bSecondHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), SecondStartPos, SecondEndPos, 15.f, ParkourTraceType, false, TArray<AActor*>(), DDT_RightClimbFootIK, SecondClimbFootHitResult, true);

						if (bSecondHit)
						{
							FVector SecondClimbFootHitResult_ForwardVector = GetForwardVector(UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(SecondClimbFootHitResult.ImpactNormal));
							FVector RightFootLocation = SecondClimbFootHitResult.ImpactPoint + (SecondClimbFootHitResult_ForwardVector * -BracedFootAddThickness_Right);
							
							LastRightFootIKTargetLocation = RightFootLocation;
							AnimInstance->SetRightFootLocation(RightFootLocation);
							break;
						}
					}
				}
			}
		}
	}
}


/*-----------------------------------------------------------
		Montage Last Hand IK Additive Location
-------------------------------------------------------------*/
void UParkourComponent::AdditiveHandIKLocation(bool bLeft, FVector AddtiveLocation)
{
	FVector ForwardVector = GetForwardVector(WallRotation) * AddtiveLocation.X;
	FVector RightVector = GetRightVector(WallRotation) * AddtiveLocation.Y;
	FVector UpVector = GetUpVector(WallRotation) * AddtiveLocation.Z;
	FVector AddLocation = ForwardVector + RightVector + UpVector;


	if (bLeft)
	{
		FVector StartPos = LastLeftHandIKTargetLocation + AddLocation + FVector(0.f, 0.f, 30.f);
		FVector EndPos = LastLeftHandIKTargetLocation + AddLocation + FVector(0.f, 0.f, -10.f);

		FHitResult AdditiveLeftHandIKLocationResult;
		bool bHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartPos, EndPos, 2.5f, ParkourTraceType, false, TArray<AActor*>(), DDT_AdditiveHandClimbHandIK, AdditiveLeftHandIKLocationResult, true);
		
		if(AdditiveLeftHandIKLocationResult.bBlockingHit)
			AnimInstance->SetLeftHandLedgeLocation(AdditiveLeftHandIKLocationResult.ImpactPoint);
		else if (AdditiveLeftHandIKLocationResult.bStartPenetrating)
		{
			// 겹칠 경우 벽이 존재
		}
	}
	else
	{
		FVector StartPos = LastRightHandIKTargetLocation + AddLocation + FVector(0.f, 0.f, 30.f);
		FVector EndPos = LastRightHandIKTargetLocation + AddLocation + FVector(0.f, 0.f, -10.f);

		FHitResult AdditiveRightHandIKLocationResult;
		bool bHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartPos, EndPos, 2.5f, ParkourTraceType, false, TArray<AActor*>(), DDT_AdditiveHandClimbHandIK, AdditiveRightHandIKLocationResult, true);
		
		if(AdditiveRightHandIKLocationResult.bBlockingHit)
			AnimInstance->SetRightHandLedgeLocation(AdditiveRightHandIKLocationResult.ImpactPoint);
		else if (AdditiveRightHandIKLocationResult.bStartPenetrating)
		{
			// 겹칠 경우 벽이 존재
		}
	}

}

void UParkourComponent::AdditiveHandIKRotation(bool bLeft, FRotator AddtiveRotation)
{
	if (bLeft)
		AnimInstance->SetLeftHandLedgeRotation(LastLeftHandIKTargetRotation + AddtiveRotation);
	else
		AnimInstance->SetRightHandLedgeRotation(LastRightHandIKTargetRotation + AddtiveRotation);
}

void UParkourComponent::AdditiveFootIKLocation(bool bLeft, FVector AddtiveLocation)
{
	FVector ForwardVector = GetForwardVector(WallRotation) * AddtiveLocation.X;
	FVector RightVector = GetRightVector(WallRotation) * AddtiveLocation.Y;
	FVector UpVector = GetUpVector(WallRotation) * AddtiveLocation.Z;
	FVector AddLocation = ForwardVector + RightVector + UpVector;

	if (bLeft)
	{
		FVector StartPos = LastLeftFootIKTargetLocation + AddLocation + (ForwardVector * -30.f);
		FVector EndPos = LastLeftFootIKTargetLocation + AddLocation + (ForwardVector * 30.f);

		FHitResult AdditiveLeftFootIKLocationResult;
		bool bHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartPos, EndPos, 15.f, ParkourTraceType, false, TArray<AActor*>(), DDT_AdditiveFootClimbFootIK, AdditiveLeftFootIKLocationResult, true);

		if (AdditiveLeftFootIKLocationResult.bBlockingHit)
			AnimInstance->SetLeftFootLocation(AdditiveLeftFootIKLocationResult.ImpactPoint);
		else if (AdditiveLeftFootIKLocationResult.bStartPenetrating)
		{
			// 겹칠 경우 벽이 존재
		}
	}
	else
	{
		FVector StartPos = LastRightFootIKTargetLocation + AddLocation + (ForwardVector * -30.f);
		FVector EndPos = LastRightFootIKTargetLocation + AddLocation + (ForwardVector * 30.f);

		FHitResult AdditiveRightFootIKLocationResult;
		bool bHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartPos, EndPos, 15.f, ParkourTraceType, false, TArray<AActor*>(), DDT_AdditiveFootClimbFootIK, AdditiveRightFootIKLocationResult, true);

		if (AdditiveRightFootIKLocationResult.bBlockingHit)
			AnimInstance->SetRightFootLocation(AdditiveRightFootIKLocationResult.ImpactPoint);
		else if (AdditiveRightFootIKLocationResult.bStartPenetrating)
		{
			// 겹칠 경우 벽이 존재
		}
	}
}





/*---------------------------
	Climb Movemnet IK
----------------------------*/
void UParkourComponent::ClimbMovementIK()
{
	if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.Climb")))
	{
		ParkourHandIK();
		ParkourFootIK();
	}
}

void UParkourComponent::ParkourHandIK()
{
	if (bFirstClimbMove)
	{
		FVector CharacterForwardVec = GetCharacterForwardVector();
		FVector CharacterRightVec = GetCharacterRightVector();
		FVector ArrowWorldLoc = ArrowActor->GetArrowWorldLocation(); 

		ParkourLeftHandIK(CharacterForwardVec, CharacterRightVec, ArrowWorldLoc);
		ParkourRightHandIK(CharacterForwardVec, CharacterRightVec, ArrowWorldLoc);
	}
}


void UParkourComponent::ParkourLeftHandIK(FVector CharacterForwardVec, FVector CharacterRightVec, FVector ArrowWorldLoc)
{
#ifdef DEBUG_IK
	LOG(Warning, TEXT("ParkourLeftHandIK"));
#endif

	float SameDirection = 0.f;

	if (UPSFunctionLibrary::CompGameplayTagName(ClimbDirectionTag, TEXT("Parkour.Direction.Left")))
		SameDirection = DirectionMovementSameHand;

	FVector LeftIKHandSocketLoc = OwnerCharacter->GetMesh()->GetSocketLocation(LeftIKHandSocketName);

	// ClimbStyleBracedXYPosition, ClimbStyleFreeHangXYPosition 변수를 통해 조절한 경우 Trace할 길이도 조절 해야한다.
	float CustomClimbXYPostion = UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag, ClimbStyleBracedXYPosition, ClimbStyleFreeHangXYPosition);

	for (int32 i = 0; i <= 4; i++)
	{
		int32 IndexCnt = i * 2;
		
		FVector LeftIKHandBasePos =
			FVector(LeftIKHandSocketLoc.X, LeftIKHandSocketLoc.Y, LeftIKHandSocketLoc.Z - CharacterHeightDiff)
			+ (CharacterRightVec * (-ClimbMovementIKHandSpace - IndexCnt - SameDirection));

		int32 IndexHeightCnt = i * 10;
		LeftIKHandBasePos -= FVector(0.f, 0.f, IndexHeightCnt); // Z값 까지 변경


		FVector LeftIKHandStartPos = LeftIKHandBasePos + (CharacterForwardVec * (-10 - FMath::Abs(CustomClimbXYPostion)));
		FVector LeftIKHandEndPos = LeftIKHandBasePos + (CharacterForwardVec * (30 + FMath::Abs(CustomClimbXYPostion)));

		// 벽 체크
		FHitResult Ht_LeftHandWall;
		UKismetSystemLibrary::SphereTraceSingle(GetWorld(),
			LeftIKHandStartPos, LeftIKHandEndPos, 10.f,
			ParkourTraceType, false, TArray<AActor*>(), DDT_ParkourHandIK, Ht_LeftHandWall, true);

		if (Ht_LeftHandWall.bBlockingHit && !Ht_LeftHandWall.bStartPenetrating)
		{
			for (int32 i2 = 0; i2 <= 5; i2++)
			{
				FVector TraceBaseLoc = 
					FVector(Ht_LeftHandWall.ImpactPoint.X, Ht_LeftHandWall.ImpactPoint.Y, ArrowWorldLoc.Z);

				int32 IndexCnt2 = i2 * 5;
				FVector TraceStartLoc = TraceBaseLoc + FVector(0.f, 0.f, IndexCnt2);
				FVector TraceEndLoc = TraceStartLoc - FVector(0.f, 0.f, 50.f + IndexCnt2);
			
				/* Wall Top 계산 */
				FHitResult LeftHandWallTopHitResult;
				UKismetSystemLibrary::SphereTraceSingle(GetWorld(),
					TraceStartLoc, TraceEndLoc, 2.5f,
					ParkourTraceType, false, TArray<AActor*>(), DDT_ParkourHandIK, LeftHandWallTopHitResult, true);

				if (!LeftHandWallTopHitResult.bStartPenetrating && LeftHandWallTopHitResult.bBlockingHit)
				{
					float ClimbStyleHandHeight = UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag, BracedHandMovementHeight, FreeHangHandMovementHeight);

					/* Set Left Hand Ledge Location */
					// 구한 Top 위치에 사용자가 수동적으로 작성한 변수들을 더해 최종적인 위치값 얻어오기
					float LeftHandLedgeLocactionZ =
						LeftHandWallTopHitResult.ImpactPoint.Z + ClimbStyleHandHeight
						+ CharacterHeightDiff + CharacterHandUpDiff
						+ GetLeftHandIKModifierGetZ();

					

					// X축 방향만 냅두고 으로 회전값 생성
					// ImpactNormal Vector의 X축의 회전 방향을 받아온다.
					FRotator LeftHandWallMakeRotX = MakeXRotator(Ht_LeftHandWall.ImpactNormal);

					FVector LeftHandWallForwardVec = GetForwardVector(LeftHandWallMakeRotX);
					FVector CharacterHandFrontDiffV = Ht_LeftHandWall.ImpactPoint +
						(LeftHandWallForwardVec * (-1.f * UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag, BracedHandFrontDiff, FreeHangHandFrontDiff)));


					FVector LeftHandLedgeTargetLoc = FVector(CharacterHandFrontDiffV.X, CharacterHandFrontDiffV.Y, LeftHandLedgeLocactionZ);

					AnimInstance->SetLeftHandLedgeLocation(LeftHandLedgeTargetLoc);


					/* Set Left Hand Ledge Rotation */
					// ReversDeltaRotator를 해야하는 이유는 ImpactNomal값의 회전값을 반대로 돌려야 손의 방향이 맞기 때문
					FRotator ReversDeltaRotator = UPSFunctionLibrary::ReversDeltaRotator(LeftHandWallMakeRotX);
					FRotator LeftHandLedgeRotationQuat = ReversDeltaRotator + Additive_LeftClimbIKHandRotation;
					AnimInstance->SetLeftHandLedgeRotation(LeftHandLedgeRotationQuat);
					break;				
				}
			}
		
			break;
		}
	}
}


void UParkourComponent::ParkourRightHandIK(FVector CharacterForwardVec, FVector CharacterRightVec, FVector ArrowWorldLoc)
{
#ifdef DEBUG_IK
	LOG(Warning, TEXT("ParkourRightHandIK"));
#endif

	float SameDirection = 0.f;

	if (UPSFunctionLibrary::CompGameplayTagName(ClimbDirectionTag, TEXT("Parkour.Direction.Right")))
		SameDirection = DirectionMovementSameHand;


	FVector RightIKHandSocketLoc = OwnerCharacter->GetMesh()->GetSocketLocation(RightIKHandSocketName);

	// ClimbStyleBracedXYPosition, ClimbStyleFreeHangXYPosition 변수를 통해 조절한 경우 Trace할 길이도 조절 해야한다.
	float CustomClimbXYPostion = UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag, ClimbStyleBracedXYPosition, ClimbStyleFreeHangXYPosition);

	for (int32 i = 0; i <= 4; i++)
	{
		int32 IndexCnt = i * 2;

		FVector RightIKHandBasePos =
			FVector(RightIKHandSocketLoc.X, RightIKHandSocketLoc.Y, RightIKHandSocketLoc.Z - CharacterHeightDiff)
			+ (CharacterRightVec * (ClimbMovementIKHandSpace + IndexCnt - SameDirection));

		
		int32 IndexHeightCnt = i * 10;
		RightIKHandBasePos -= FVector(0.f, 0.f, IndexHeightCnt); // Z값 까지 변경
		
		FVector RightIKHandStartPos = RightIKHandBasePos + (CharacterForwardVec * (-10 - FMath::Abs(CustomClimbXYPostion)));
		FVector RightIKHandEndPos = RightIKHandBasePos + (CharacterForwardVec * (30 + FMath::Abs(CustomClimbXYPostion)));

		// 벽 체크
		FHitResult Ht_RightHandWall;
		UKismetSystemLibrary::SphereTraceSingle(GetWorld(),
			RightIKHandStartPos, RightIKHandEndPos, 10.f,
			ParkourTraceType, false, TArray<AActor*>(), DDT_ParkourHandIK, Ht_RightHandWall, true);

		if (Ht_RightHandWall.bBlockingHit && !Ht_RightHandWall.bStartPenetrating)
		{
			for (int32 i2 = 0; i2 <= 5; i2++)
			{
				FVector TraceBasePos =
					FVector(Ht_RightHandWall.ImpactPoint.X, Ht_RightHandWall.ImpactPoint.Y, ArrowWorldLoc.Z);

				int32 IndexCnt2 = i2 * 5;
				FVector TraceStartPos = TraceBasePos + FVector(0.f, 0.f, IndexCnt2);
				FVector TraceEndPos = TraceStartPos - FVector(0.f, 0.f, 50.f + IndexCnt2);

				// Wall Top
				FHitResult Ht_RightHandWallTop;
				UKismetSystemLibrary::SphereTraceSingle(GetWorld(),
					TraceStartPos, TraceEndPos, 2.5f,
					ParkourTraceType, false, TArray<AActor*>(), DDT_ParkourHandIK, Ht_RightHandWallTop, true);

				if (!Ht_RightHandWallTop.bStartPenetrating && Ht_RightHandWallTop.bBlockingHit)
				{
					float ClimbStyleHandHeight = UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag, BracedHandMovementHeight, FreeHangHandMovementHeight);

					/* Set Left Hand Ledge Location */
					float RightHandLedgeLocationZ =
						Ht_RightHandWallTop.ImpactPoint.Z + ClimbStyleHandHeight
						+ CharacterHeightDiff + CharacterHandUpDiff
						+ GetRightHandIKModifierGetZ();


					// X축 방향만 냅두고 으로 회전값 생성
					// ImpactNormal Vector의 X축의 회전 방향을 받아온다.
					FRotator RightHandWallMakeRotX = MakeXRotator(Ht_RightHandWall.ImpactNormal);


					FVector RightHandWallForwardVec = GetForwardVector(RightHandWallMakeRotX);
					FVector CharacterHandFrontDiffVector = Ht_RightHandWall.ImpactPoint +
						(RightHandWallForwardVec * (-1.f * UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag, BracedHandFrontDiff, FreeHangHandFrontDiff)));


					FVector RightHandLedgeTargetLoc = FVector(CharacterHandFrontDiffVector.X, CharacterHandFrontDiffVector.Y, RightHandLedgeLocationZ);
					AnimInstance->SetRightHandLedgeLocation(RightHandLedgeTargetLoc);


					/* Set Left Hand Ledge Rotation */
					FRotator ReversDeltaRotator = UPSFunctionLibrary::ReversDeltaRotator(RightHandWallMakeRotX);
					FRotator RightHandLedgeRotation = ReversDeltaRotator + Additive_RightClimbIKHandRotation;
					AnimInstance->SetRightHandLedgeRotation(RightHandLedgeRotation);
					break;				
				}
			}

			break;
		}
	}
}

void UParkourComponent::ParkourFootIK()
{
	if (bFirstClimbMove)
	{
		FVector CharacterForwardVec = GetCharacterForwardVector();
		FVector CharacterRightVec = GetCharacterRightVector();

		ParkourLeftFootIK(CharacterForwardVec, CharacterRightVec);
		ParkourRightFootIK(CharacterForwardVec, CharacterRightVec);
	}
}

void UParkourComponent::ParkourLeftFootIK(FVector CharacterForwardVecor, FVector CharacterRightVector)
{
#ifdef DEBUG_IK
	LOG(Warning, TEXT("ParkourLeftFootIK"));
#endif

	if (AnimInstance->GetCurveValue(RightIKFootCurveName) == 1.f)
	{
		FVector LeftIKFootLoc = OwnerCharacter->GetMesh()->GetSocketLocation(LeftIKFootSocketName);
		FVector LeftHandLoc = OwnerCharacter->GetMesh()->GetSocketLocation(LeftHandSocketName);

		for (int32 i = 0; i <= 4; i++)
		{
			for (int32 i2 = 0; i2 <= 4; i2++)
			{
				int32 indexCnt2 = i2 * 5;
				/*
					 Left IK Foot (X,Y) 위치 + (Left Hand - FootPositionToHand) * indexCnt2 (Z) 위치
					i_indexCnt2_l를 Z축에 더해주는 이유는 첫 검사하는 부분이 비어있는 경우
					Z축 위쪽으로 검사해가며 발 디딜곳을 찾아 가기 위함이다.
				*/
				FVector LeftFootLoc = FVector(LeftIKFootLoc.X, LeftIKFootLoc.Y,
					((LeftHandLoc.Z + LeftFootPositionToHand) + indexCnt2));

				float AdditiveLeftLength = i * 4 + 13.f;
				FVector AdditiveTraceRightVector = CharacterRightVector * AdditiveLeftLength;

				FVector LeftFootTraceBasePos = LeftFootLoc + AdditiveTraceRightVector;
				FVector LeftFootTraceStartPos = LeftFootTraceBasePos + (CharacterForwardVecor * -30.f);
				FVector LeftFootTraceEndPos = LeftFootTraceBasePos + (CharacterForwardVecor * 70.f);

				FHitResult Ht_LeftFootResult;
				UKismetSystemLibrary::SphereTraceSingle(GetWorld(), LeftFootTraceStartPos, LeftFootTraceEndPos,
					15.f, ParkourTraceType, false, TArray<AActor*>(),
					DDT_ParkourFootIK, Ht_LeftFootResult, true);

				if (Ht_LeftFootResult.bBlockingHit && !Ht_LeftFootResult.bStartPenetrating)
				{
					// 설정한 Offset 만큼 빼준 위치
					FVector FootDepth = GetForwardVector(MakeXRotator(Ht_LeftFootResult.ImpactNormal)) * FootDepthOffset;
					FVector TargetIKFootLocation = Ht_LeftFootResult.ImpactPoint - FootDepth;
					AnimInstance->SetLeftFootLocation(TargetIKFootLocation);
					return;
				}
			}
		}
	}
	else
		ResetFootIK(false);
	
}

void UParkourComponent::ParkourRightFootIK(FVector CharacterForwardVecor, FVector CharacterRightVector)
{
#ifdef DEBUG_IK
	LOG(Warning, TEXT("ParkourRightFootIK"));
#endif
	if (AnimInstance->GetCurveValue(RightIKFootCurveName) == 1.f)
	{
		FVector RightIKFootLoc = OwnerCharacter->GetMesh()->GetSocketLocation(RightIKFootSocketName);
		FVector RightHandLoc = OwnerCharacter->GetMesh()->GetSocketLocation(RightHandSocketName);

		for (int32 i = 0; i <= 4; i++)
		{
			for (int32 i2 = 0; i2 <= 4; i2++)
			{
				int32 indexCnt2 = i2 * 5;
				/*
					 Right IK Foot (X,Y) 위치 + (Right Hand - RightFootPositionToHand) * indexCnt2 (Z) 위치
					i_indexCnt2_l를 Z축에 더해주는 이유는 첫 검사하는 부분이 비어있는 경우
					Z축 위쪽으로 검사해가며 발 디딜곳을 찾아 가기 위함이다.
				*/
				FVector RightFootLoc = FVector(RightIKFootLoc.X, RightIKFootLoc.Y,
					((RightHandLoc.Z + RightFootPositionToHand) + indexCnt2));

				float AdditiveRightLength = i * 4 - 13.f;
				FVector AdditiveTraceRightVector = CharacterRightVector * AdditiveRightLength;

				FVector RightFootTraceBasePos = RightFootLoc + AdditiveTraceRightVector;
				FVector RightFootTraceStartPos = RightFootTraceBasePos + (CharacterForwardVecor * -30.f);
				FVector RightFootTraceEndPos = RightFootTraceBasePos + (CharacterForwardVecor * 70.f);

				FHitResult Ht_RightFootResult;
				UKismetSystemLibrary::SphereTraceSingle(GetWorld(), RightFootTraceStartPos, RightFootTraceEndPos,
					15.f, ParkourTraceType, false, TArray<AActor*>(),
					DDT_ParkourFootIK, Ht_RightFootResult, true);

				if (Ht_RightFootResult.bBlockingHit && !Ht_RightFootResult.bStartPenetrating)
				{
					// 설정한 Offset 만큼 빼준 위치
					FVector FootDepth = GetForwardVector(MakeXRotator(Ht_RightFootResult.ImpactNormal)) * FootDepthOffset;
					FVector TargetIKFootLocation = Ht_RightFootResult.ImpactPoint - FootDepth;
					AnimInstance->SetRightFootLocation(TargetIKFootLocation);

					return;
				}
			}
		}
	}
	else
		ResetFootIK(true);

}

void UParkourComponent::ResetFootIK(bool bRightFoot)
{
	if (bRightFoot)
	{
		FVector RightIKFootLocation = OwnerCharacter->GetMesh()->GetSocketLocation(RightIKFootSocketName);
		AnimInstance->SetRightFootLocation(RightIKFootLocation);
	}
	else
	{
		FVector LeftIKFootLocation = OwnerCharacter->GetMesh()->GetSocketLocation(LeftIKFootSocketName);
		AnimInstance->SetLeftFootLocation(LeftIKFootLocation);
	}
}



float UParkourComponent::GetLeftHandIKModifierGetZ()
{
	float ZOffset = 0.f;

	if (UPSFunctionLibrary::CompGameplayTagName(ClimbDirectionTag, TEXT("Parkour.Direction.NoDirection")))
		return 0.f;
	else
	{
		if (UPSFunctionLibrary::CompGameplayTagName(ClimbDirectionTag, TEXT("Parkour.Direction.Left")))
		{
			ZOffset = UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag,
				BracedLeftHandModifier.SameDirection,
				FreeHangLeftHandModifier.SameDirection);
		}
		else
		{
			ZOffset = UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag,
				BracedLeftHandModifier.DifferenceDirection,
				FreeHangLeftHandModifier.DifferenceDirection);
		}
	}

	float CurveValue = AnimInstance->GetCurveValue(LeftHandModifierName);

	return FMath::Clamp(CurveValue - ZOffset, 0.f, ModifierHandMAXHeight);

}

float UParkourComponent::GetRightHandIKModifierGetZ()
{
	float ZOffset = 0.f;

	if (UPSFunctionLibrary::CompGameplayTagName(ClimbDirectionTag, TEXT("Parkour.Direction.NoDirection")))
		return 0.f;
	else
	{
		if (UPSFunctionLibrary::CompGameplayTagName(ClimbDirectionTag, TEXT("Parkour.Direction.Right")))
		{
			ZOffset = UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag,
				BracedRightHandModifier.SameDirection,
				FreeHangRightHandModifier.SameDirection);
		}
		else
		{
			ZOffset = UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag,
				BracedRightHandModifier.DifferenceDirection,
				FreeHangRightHandModifier.DifferenceDirection);
		}
	}

	float CurveValue = AnimInstance->GetCurveValue(RightHandModifierName);

	return FMath::Clamp(CurveValue - ZOffset, 0.f, ModifierHandMAXHeight);
}


/*-----------------------
		Movement
-------------------------*/
void UParkourComponent::AddMovementInput(float ScaleValue, bool bFront)
{
	if (bFront)
	{
		ForwardValue = ScaleValue;

		if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.NotBusy")))
		{
			FRotator ControlRoation_Yaw = FRotator(0.f, OwnerCharacter->GetControlRotation().Yaw, 0.f);
			FVector WorldDirection = GetForwardVector(ControlRoation_Yaw);
			OwnerCharacter->AddMovementInput(WorldDirection, ScaleValue);
		}
		else if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.Climb")))
		{
			#ifdef DEBUG_MOVEMENT
				LOG(Warning, TEXT("Movemenet Input Climb State ForwardValue"));
			#endif

			GetClimbForwardValue(ForwardValue, HorizontalClimbForwardValue, VerticalClimbForwardValue);

			if (AnimInstance->IsAnyMontagePlaying())
				StopClimbMovement();
			else
				ClimbMovement();
		}
	}
	else
	{
		RightValue = ScaleValue;

		if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.NotBusy")))
		{
			FRotator ControlRoation_Yaw = FRotator(0.f, OwnerCharacter->GetControlRotation().Yaw, 0.f);
			FVector WorldDirection = GetRightVector(ControlRoation_Yaw);
			OwnerCharacter->AddMovementInput(WorldDirection, ScaleValue);
		}
		else if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.Climb")))
		{
			#ifdef DEBUG_MOVEMENT
				LOG(Warning, TEXT("Movemenet Input Climb State RightValue"));
			#endif

			//HoriFWD, VertiFWD = CBR
			GetClimbRightValue(RightValue, HorizontalClimbRightValue, VerticalClimbRightValue);

			if (AnimInstance->IsAnyMontagePlaying())
				StopClimbMovement();
			else
				ClimbMovement();
		}
	}
}

float UParkourComponent::GetHorizontalAxis()
{
	// 움직임이 존재할 경우
	if (ForwardValue != 0.f || RightValue != 0.f)
	{
		float HorizontalValue = HorizontalClimbForwardValue + HorizontalClimbRightValue;
		return FMath::Clamp(HorizontalValue, -1.f, 1.f);
	}
	else
		return 0.f;
}

float UParkourComponent::GetVerticalAxis()
{
	// 움직임이 존재할 경우
	if (ForwardValue != 0.f || RightValue != 0.f)
	{
		float HorizontalValue = VerticalClimbForwardValue + VerticalClimbRightValue;
		return FMath::Clamp(HorizontalValue, -1.f, 1.f);
	}
	else
		return 0.f;
}


void UParkourComponent::ClimbMovement()
{
#ifdef DEBUG_MOVEMENT
	LOG(Warning, TEXT("ClimbMovement"));
#endif

	if (UPSFunctionLibrary::CompGameplayTagName(ParkourActionTag, TEXT("Parkour.Action.CornerMove")))
		return;

	float HorizontalAxis = GetHorizontalAxis();
	
	if(UKismetMathLibrary::Abs(HorizontalAxis) > 0.7f)
	{
		FGameplayTag SelectDirectionTag = HorizontalAxis > 0.f ?
			UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.Direction.Right")) : UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.Direction.Left"));

		SetClimbDirection(SelectDirectionTag);
	}
	else
	{
		StopClimbMovement();
		return;
	}
		
	FVector ArrowWorldLocation = ArrowActor->GetArrowWorldLocation();
	FRotator ArrowWorldRotation = ArrowActor->GetArrowWorldRotation();
	FVector ForwardVector = GetForwardVector(ArrowWorldRotation);
	FVector RightVectorClimbDistance = GetRightVector(ArrowWorldRotation)
		* (ClimbMovementDistance * HorizontalAxis); 

	// ClimbMoveCheckDistance 변수에 입력된 값만큼 해당 위치를 검사하여 ClimbMove가 가능하면 움직이는 과정
	// 코너 뿐만아니라 살짝 돌출되거나 둥근 모양의 모서리부분의 ClimbMove 계산하기 위함
	// 위에서 부터 아래로 CapsuleTrace로 검사를 한다.

	bool bCheckFirstForBreak = false;
	bool bCheckSecondForBreak = false;
	bool bCheckThirdForBreak = false;


	// 벽을 찾는 과정
	for (int32 FirstIndex = 0; FirstIndex <= 2 && !bCheckFirstForBreak; FirstIndex++)
	{
		int32 LocalIndex = FirstIndex * -15;
		FVector ClimbMoveCheckStartPos = FVector(0.f, 0.f, LocalIndex) + ArrowWorldLocation + RightVectorClimbDistance + (ForwardVector * -10.f);
		FVector ClimbMoveCheckEndPos = ClimbMoveCheckStartPos + (ForwardVector * 60.f);

		FHitResult ClimbMovementHitResult;
		UKismetSystemLibrary::SphereTraceSingle(GetWorld(), ClimbMoveCheckStartPos, ClimbMoveCheckEndPos, 5.f, ParkourTraceType, false,
			TArray<AActor*>(), DDT_ClimbMovementCheck, ClimbMovementHitResult, true);

		// bStartPenetrating이 true라는 것은 Hit하기도 전에 겹쳐버렸다는 뜻
		// 모퉁이에서 살짝 모난부분이 코너를 돌기에도 애매하고 바로 잡기도 애매한 위치인 경우를 뜻함
		// Top을 구하거나 Corner 함수를 실행한다.

		if (!ClimbMovementHitResult.bStartPenetrating)
		{
			if (CheckOutCorner()) // 캐릭터 바로 옆에 코너돌수 있는 벽 체크
				OutCornerMovement(); 
			else // 코너가 아닌경우 or 코너긴 하지만 캐릭터 바로 옆에 막히는 벽이 없는 경우
			{				
				// bStartPenetrating은 아예 검사가 되지않았을때도 false를 출력하기때문에 bBlockingHit을 검사
				if (ClimbMovementHitResult.bBlockingHit) // ★★ Corner가 아니고 ClimbMove가 가능함
				{
					LastLeftHandIKTargetLocation = ClimbMovementHitResult.ImpactPoint;
					LastRightHandIKTargetLocation = ClimbMovementHitResult.ImpactPoint;

					// ClimbMove가 가능한 모서리인경우 Trace할 위치에 FVector(0.f, 0.f, SecondIndex * 5 + float) 만큼 해당 Top을 체크
					for (int32 SecondIndex = 0; SecondIndex <= 6 && !bCheckSecondForBreak; SecondIndex++)
					{
						FRotator NoramlizeDeltaRotator = UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(ClimbMovementHitResult.ImpactNormal);
						FVector NormalizeForwardVector = GetForwardVector(NoramlizeDeltaRotator);
						FVector NormalizeRightVector = GetRightVector(NoramlizeDeltaRotator);

						FVector ClimbMoveTopCheckStartPos = ClimbMovementHitResult.ImpactPoint + (NormalizeForwardVector * 2.f)
							+ FVector(0.f, 0.f, (SecondIndex * 5) + 5.f);
						FVector ClimbMoveTopCheckEndPos = ClimbMoveTopCheckStartPos - FVector(0.f, 0.f, (SecondIndex * 5) + 50.f);

						FHitResult ClimbMovementTopHitResult;
						UKismetSystemLibrary::SphereTraceSingle(GetWorld(), ClimbMoveTopCheckStartPos, ClimbMoveTopCheckEndPos, 5.f, ParkourTraceType, false,
							TArray<AActor*>(), DDT_ClimbMovementCheck, ClimbMovementTopHitResult, true);


						if (ClimbMovementTopHitResult.bStartPenetrating)
						{
							// ClimbMoveCheck, ClimbMoveTopCheck 이중 for문 마지막까지 검사를 진행한 경우
							if (SecondIndex == 6 && FirstIndex == 2)
							{
								StopClimbMovement();
							}							
						}
						else
						{			
							bCheckSecondForBreak = true;

							if (!ClimbMovementTopHitResult.bBlockingHit)
							{
								StopClimbMovement();
							}						
							else // Climb를 하며 이동할때 살짝 높이가 높은 부분이 있는 경우 LineTrace로 검사하여 해당 위치 Check하는 for문
							{
								bCheckFirstForBreak = true; // break;

								// 높이 체크 ★
								for (int32 ThirdIndex = 0; ThirdIndex <= 5 && !bCheckThirdForBreak; ThirdIndex++)
								{
									float Index_Z = (ThirdIndex * 5.f) + 2.f;
									FVector HeightCheckStartPos = FVector(0.f, 0.f, Index_Z) + ClimbMovementTopHitResult.ImpactPoint;
									FVector CheckRightVector = (HorizontalAxis * 15.f) * NormalizeRightVector;
									FVector HeightCheckEndPos = HeightCheckStartPos + CheckRightVector;

									FHitResult ClimbHeightHitResult;
									bool bHit = UKismetSystemLibrary::LineTraceSingle(GetWorld(), HeightCheckStartPos, HeightCheckEndPos, ParkourTraceType, false, TArray<AActor*>(), DDT_ClimbMovementHeightCheck, ClimbHeightHitResult, true);

									if (bHit)
									{
										if (ThirdIndex == 5)
										{
											StopClimbMovement();
										}
											
									}
									else
									{
										bCheckThirdForBreak = true;

										if(!CheckClimbMovementSurface(ClimbMovementHitResult))
										{
											StopClimbMovement();
											return; // 옆에 무언가 있다는 것이기 때문에 더 검사할 필요가 없다.
										}
										else 
										{
											SetClimbMovementTransform(ClimbMovementHitResult.ImpactPoint,
												ClimbMovementTopHitResult.ImpactPoint,
												NoramlizeDeltaRotator,
												NormalizeForwardVector);
											
											return;
										}	
									}
								}
							}
						}							
					}
				}
				else // ClimbMovementHitResult.bBlockingHit가 false인 경우. 캐릭터를 막지않는 코너를 만난 경우
				{
					if (FirstIndex == 2)
					{
						if (!UPSFunctionLibrary::CompGameplayTagName(ParkourActionTag, TEXT("Parkour.Action.CornerMove")))
							InCornerMovement(); // Corner Move 상태가 아닌경우
						else
						{
							#ifdef DEBUG_MOVEMENT
								LOG(Warning, TEXT("StopClimbMovement 6"));
							#endif

							StopClimbMovement();
						}							
					}
					else
					{
						#ifdef DEBUG_MOVEMENT
							LOG(Warning, TEXT("StopClimbMovement 7"));
						#endif
					}						
				}
			}
		}
		else
		{
			#ifdef DEBUG_MOVEMENT
				LOG(Warning, TEXT("Climb Movement Else"));
			#endif
		}
	}	
}

void UParkourComponent::SetClimbMovementTransform(FVector ClimbFirstHitImpactPoint, FVector ClimbTopHitImpactPoint, FRotator NormalizeDeltaRotator, FVector NormalizeForwardVector)
{
#ifdef DEBUG_MOVEMENT
	LOG(Warning, TEXT("SetClimbMovementTransform"));
#endif

	WallRotation = NormalizeDeltaRotator;
	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FRotator CharacterRotation = OwnerCharacter->GetActorRotation();
	float DeltaSeconds = GetWorld()->GetDeltaSeconds();

	FVector TargetClimbMovementVector = NormalizeForwardVector * UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag, ClimbStyleBracedXYPosition, ClimbStyleFreeHangXYPosition)
		+ ClimbFirstHitImpactPoint;


	float GetX = FMath::FInterpTo(CharacterLocation.X, TargetClimbMovementVector.X, DeltaSeconds, GetClimbMoveSpeed());
	float GetY = FMath::FInterpTo(CharacterLocation.Y, TargetClimbMovementVector.Y, DeltaSeconds, GetClimbMoveSpeed());


	float TargetZ = ClimbTopHitImpactPoint.Z + UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag, ClimbStyleBracedZPosition, ClimbStyleFreeHangZPosition) - -CharacterHeightDiff;
	float ClimbTypeInterpSpeed = UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag, ClimbStyleBracedInterpSpeed, ClimbStyleFreeHangInterpSpeed);
	float GetZ = FMath::FInterpTo(CharacterLocation.Z, TargetZ, DeltaSeconds, ClimbTypeInterpSpeed);

	FVector InterpToCharacterLocation = FVector(GetX, GetY, GetZ);

	FRotator InterpToCharacterRotation = FMath::RInterpTo(CharacterRotation, WallRotation, DeltaSeconds, 4.f);

	OwnerCharacter->SetActorLocationAndRotation(InterpToCharacterLocation, InterpToCharacterRotation);
	CheckClimbBracedStyleMoving(ClimbTopHitImpactPoint, WallRotation);
	bFirstClimbMove = true;

}

void UParkourComponent::StopClimbMovement()
{
#ifdef DEBUG_MOVEMENT
	LOG(Warning, TEXT("StopClimbMovement"));
#endif

	if (!CharacterMovement)
		UPSFunctionLibrary::CrashLog(TEXT("CharacterMovement NULL"), TEXT("CharacterMovement NULL"));
	
	CharacterMovement->StopMovementImmediately();
	SetClimbDirection(TEXT("Parkour.Direction.NoDirection"));
}

void UParkourComponent::SetClimbDirection(FGameplayTag NewClimbDirectionTag)
{
	if (ClimbDirectionTag != NewClimbDirectionTag)
	{
		ClimbDirectionTag = NewClimbDirectionTag;

		if (AnimInstance)
			AnimInstance->SetClimbDirection(NewClimbDirectionTag);
	}
}

void UParkourComponent::SetClimbDirection(FName NewClimbDirectionName)
{
	FGameplayTag NewClimbDirectionTag = UPSFunctionLibrary::GetGameplayTag(NewClimbDirectionName);

	if (ClimbDirectionTag != NewClimbDirectionTag)
	{
		ClimbDirectionTag = NewClimbDirectionTag;

		if (AnimInstance)
			AnimInstance->SetClimbDirection(NewClimbDirectionTag);
	}
}


void UParkourComponent::GetClimbForwardValue(float ScaleValue, float& HorizontalClimbForward, float& VerticalClimbForward)
{
	// Climb상태에서 움직일 때 컨트롤러의 상대적 방향에따라 움직일 것인지
	if (bUseControllerRotation_Climb)
	{
		FRotator ControlRotation = OwnerCharacter->GetControlRotation();
		FRotator CharacterRotation = OwnerCharacter->GetActorRotation();

		float DeltaYaw = UKismetMathLibrary::NormalizedDeltaRotator(ControlRotation, CharacterRotation).Yaw;

		/* 
		Cos0도는 값이 1이다.즉, 0도는 같은 방향으로 눌렀다는 뜻이므로
		전방, 후방을 나타내는 Vertical에는 Cos을, 그 반대인 Horizontal은 Sin을 구한다. 
		Cos과 Sin은 반비례 하기때문.
		*/
		HorizontalClimbForward = ScaleValue * UKismetMathLibrary::DegSin(DeltaYaw);
		VerticalClimbForward = ScaleValue * UKismetMathLibrary::DegCos(DeltaYaw);
	}
	else
	{
		HorizontalClimbForward = 0.f;
		VerticalClimbForward = ScaleValue;
	}
	
}

void UParkourComponent::GetClimbRightValue(float ScaleValue, float& HorizontalClimbRight, float& VerticalClimbRight)
{
	// Climb상태에서 움직일 때 컨트롤러의 상대적 방향에따라 움직일 것인지
	if (bUseControllerRotation_Climb)
	{
		FRotator ControlRotation = OwnerCharacter->GetControlRotation();
		FRotator CharacterRotation = OwnerCharacter->GetActorRotation();

		float DeltaYaw = 0.f - UKismetMathLibrary::NormalizedDeltaRotator(ControlRotation, CharacterRotation).Yaw;

		/*
		Cos0도는 값이 1이다.즉, 0도는 같은 방향으로 눌렀다는 뜻이므로
		왼쪽, 오른쪽을 나타내는 Horizontal에는 Cos을, 그 반대인 Vertical은 Sin을 구한다.
		*/
		HorizontalClimbRight = ScaleValue * UKismetMathLibrary::DegCos(DeltaYaw);
		VerticalClimbRight = ScaleValue * UKismetMathLibrary::DegSin(DeltaYaw);
	}
	else
	{
		HorizontalClimbRight = ScaleValue;
		VerticalClimbRight = 0.f;
	}
	
}

float UParkourComponent::GetClimbMoveSpeed()
{
	float OutputValue = 1.f;
	bool bFindName = AnimInstance->GetCurveValue(ClimbMoveSpeedCurveName, OutputValue);

	if (bFindName)
	{
		float BracedSpeed = FMath::Clamp(OutputValue, 0.f, ClimbSpeedMaxValue) * ClimbSpeedBraced;
		float FreeHangSpeed = FMath::Clamp(OutputValue, 0.f, ClimbSpeedMaxValue) * ClimbSpeedFreeHang;

		return UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag, BracedSpeed, FreeHangSpeed);
	}

	return OutputValue;
}

void UParkourComponent::CheckClimbBracedStyleMoving(FVector ClimbTopHitImpactPoint, FRotator NormalizeDeltaRotator)
{
	FVector ForwardVector = GetForwardVector(WallRotation);
	FVector StartPos = ClimbTopHitImpactPoint + FVector(0.f, 0.f, CheckClimbBracedStyle) + ForwardVector * -10.f;
	FVector EndPos = ClimbTopHitImpactPoint + FVector(0.f, 0.f, CheckClimbBracedStyle) + ForwardVector * 30.f;

	FHitResult ClimbStyleCheckHitResult;
	bool bClimbStyleCheck = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartPos, EndPos, 10.f, ParkourTraceType,
		false, TArray<AActor*>(), DDT_CheckClimbBracedStyleMoving, ClimbStyleCheckHitResult, true);

	if (bClimbStyleCheck)
		SetClimbStyle(TEXT("Parkour.ClimbStyle.Braced"));
	else
		SetClimbStyle(TEXT("Parkour.ClimbStyle.FreeHang"));

}

void UParkourComponent::ResetMovement()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("ResetMovement"));
#endif

	ForwardValue = 0.f;
	RightValue = 0.f;
	SetClimbDirection(TEXT("Parkour.Direction.NoDirection"));
}

// 캐릭터의 Head Socket 위치를 이용하여 ClimbLedgeResult() 함수에서 구한 HitResult.ImpactPoint.Z까지의
// Height를 통해 현재 캐릭터가 땅에 있는지 공중에있는지 판단
bool UParkourComponent::CheckAirHang()
{
	if (ClimbLedgeHitResult.bBlockingHit)
	{
		if (!OwnerCharacter)
			return false;

		float HeadZ = OwnerCharacter->GetMesh()->GetSocketLocation("Head").Z;
		float LedgeZ = ClimbLedgeHitResult.ImpactPoint.Z;

		if (HeadZ - LedgeZ > 30.f)
		{
			if (bInGround)
				return false;
			else
				return true;
		}
		else
			return false;
	}
	else
		return false;
}



/*-----------------
		Drop
-------------------*/
void UParkourComponent::ParkourDrop()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Error, TEXT("ParkourDrop"));
#endif

	if (bInGround)
	{
		if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.NotBusy")))
			FindDropDownHangLocation();
	}
	else
	{
		if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.Climb")))
		{
			SetParkourState(UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.State.NotBusy")));
			bCanAutoClimb = false;
			bCanManuelClimb = false;
			FTimerHandle CanManuelClimbHandle;
			GetWorld()->GetTimerManager().SetTimer(CanManuelClimbHandle, this, &UParkourComponent::SetCanManuelClimb, Drop_CanManuelClimbBecomesTrueTime, false);
		}
	}
}


void UParkourComponent::FindDropDownHangLocation()
{
	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FVector CharacterEndPos = CharacterLocation + FVector(0.f, 0.f, CheckCharacterDropHeight);
	FHitResult FindDropDownHitResult;
	UKismetSystemLibrary::SphereTraceSingle(GetWorld(), CharacterLocation, CharacterEndPos, 35.f, ParkourTraceType, false, TArray<AActor*>(), DDT_FindDropDownHangLocation, FindDropDownHitResult, true);

	if (FindDropDownHitResult.bBlockingHit && !FindDropDownHitResult.bStartPenetrating)
	{
		// 회전값의 -5.f 만큼 아래쪽의 ForwardVector 방향쪽으로 SphereTrace
		// 캐릭터가 Climb를 시도 할수있는 충분한 공간이 있는지 확인 
		// 위에서 검사한 Trace는 캐릭터의 바로 아래 땅의 정보를, 해당 정보를 토대로 모서리를 찾는 역할
		FVector ForwardVector = GetForwardVector(GetDesireRotation());
		FVector StartPos_Second = (FindDropDownHitResult.ImpactPoint + FVector(0.f, 0.f, -5.f)) + (ForwardVector * 100.f);
		FVector EndPos_Second = StartPos_Second + (ForwardVector * -125.f);

		FHitResult FindDropDownHitResult_Second;
		UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartPos_Second, EndPos_Second, 5.f, ParkourTraceType, false, TArray<AActor*>(), DDT_FindDropDownHangLocation, FindDropDownHitResult_Second, true);

		// 최종적으로 Climb가 가능한 한 곳을 찾는 과정
		if (FindDropDownHitResult_Second.bBlockingHit && !FindDropDownHitResult_Second.bStartPenetrating)
		{
			// Climb가 가능한 곳을 탐색
			WallHitTraces.Empty();
			FRotator NormalRotation = UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(FindDropDownHitResult_Second.ImpactNormal);
			FVector ForwardNormalVector = GetForwardVector(NormalRotation);
			FVector NormalRightVector = GetRightVector(NormalRotation);
			FVector ImpactPoint = FindDropDownHitResult_Second.ImpactPoint;
			for (int32 LineCnt = 0; LineCnt <= 4; LineCnt++)
			{
				int32 Width = (LineCnt * 20) - 40;
				FVector BasePos_LineWidth = ImpactPoint + (NormalRightVector * Width);
				FVector StartPos_LineWidth = BasePos_LineWidth + (ForwardNormalVector * -40.f);
				FVector EndPos_LineWidth = BasePos_LineWidth + (ForwardNormalVector * 30.f);

				FHitResult FindDropDownHitResult_Third;
				UKismetSystemLibrary::LineTraceSingle(GetWorld(), StartPos_LineWidth, EndPos_LineWidth, ParkourTraceType, false, TArray<AActor*>(), DDT_FindDropDownHangLocation,  FindDropDownHitResult_Third, true);
				HopHitTraces.Empty();

				// 해당 라인에서 Z축 위 쪽으로 Line Trace 진행
				for (int32 HopCnt = 0; HopCnt <= 12; HopCnt++)
				{
					FVector StartPos_HopTrace = FVector(0.f, 0.f, (HopCnt * 8)) + FindDropDownHitResult_Third.TraceStart;
					FVector EndPos_HopTrace = FVector(0.f, 0.f, (HopCnt * 8)) + FindDropDownHitResult_Third.TraceEnd;

					FHitResult FindDropDownHitResult_Hop;
					UKismetSystemLibrary::LineTraceSingle(GetWorld(), StartPos_HopTrace, EndPos_HopTrace, ParkourTraceType, false, TArray<AActor*>(), DDT_FindDropDownHangLocation, FindDropDownHitResult_Hop, true);

					HopHitTraces.Emplace(FindDropDownHitResult_Hop);
				}

				// 위 반복문에서 찾아낸 곳에서 Distance를 판별하여 하나를 WallHitTraces에 저장
				for (int32 Num = 1; Num < HopHitTraces.Num(); Num++)
				{
					double VectorDistance_Current = UKismetMathLibrary::Vector_Distance(HopHitTraces[Num].TraceStart, HopHitTraces[Num].TraceEnd);
					double VectorDistance_Prev = UKismetMathLibrary::Vector_Distance(HopHitTraces[Num - 1].TraceStart, HopHitTraces[Num - 1].TraceEnd);
					
					float CurrentDistance = UKismetMathLibrary::SelectFloat(HopHitTraces[Num].Distance, VectorDistance_Current, HopHitTraces[Num].bBlockingHit);
					float PrevDistance = UKismetMathLibrary::SelectFloat(HopHitTraces[Num - 1].Distance, VectorDistance_Prev, HopHitTraces[Num - 1].bBlockingHit);
					
					if (CurrentDistance - PrevDistance > 5.f)
					{
						WallHitTraces.Emplace(HopHitTraces[Num - 1]);
						break;
					}
				}
			}

			for (int32 i = 0; i < WallHitTraces.Num(); i++)
			{
				if (i == 0)
					WallHitResult = WallHitTraces[0];
				else
				{
					float CurrentDistance = UKismetMathLibrary::Vector_Distance(WallHitTraces[i].ImpactPoint, CharacterLocation);
					float PrevDistance = UKismetMathLibrary::Vector_Distance(WallHitResult.ImpactPoint, CharacterLocation);

					// WallHitTraces[i]가 Distance가 더 작은경우 해당 값으로 WallHitResult 갱신
					// 아닌경우에는 WallHitResult 그대로 사용 (선택정렬)
					if (CurrentDistance <= PrevDistance)
						WallHitResult = WallHitTraces[i];
					
				}
			}

			// 마지막으로 해당위치에 Climb 실행을 하기위해 Trace하여 Top을 검사한뒤 Climb 함수 실행
			if (WallHitResult.bBlockingHit && !WallHitResult.bStartPenetrating)
			{
				WallRotation = UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(WallHitResult.ImpactNormal);
				FVector BasePos_ClimbTrace = WallHitResult.ImpactPoint + (UKismetMathLibrary::GetForwardVector(WallRotation) * 2.f);
				FVector StartPos_ClimbTrace = BasePos_ClimbTrace + FVector(0.f, 0.f, 7.f);
				FVector EndPos_ClimbTrace = BasePos_ClimbTrace + FVector(0.f, 0.f, -7.f);

				FHitResult FindDropClimbTraceHitResult;
				bool bHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartPos_ClimbTrace, EndPos_ClimbTrace, 2.f, ParkourTraceType, false, TArray<AActor*>(), DDT_FindDropDownHangLocation, FindDropClimbTraceHitResult, true);

				if (bHit)
				{
					WallTopHitResult = FindDropClimbTraceHitResult;
					if (CheckClimbSurface())
					{
						CheckClimbStyle();
						ClimbLedgeResult();
						if (UPSFunctionLibrary::CompGameplayTagName(ClimbStyleTag, TEXT("Parkour.ClimbStyle.Braced")))
							SetParkourAction(TEXT("Parkour.Action.DropDown"));
						else
							SetParkourAction(TEXT("Parkour.Action.FreeHangDropDown"));
					}
					else
						ResetParkourHitResult();
				}

			}

		}

	}	
}

void UParkourComponent::SetCanManuelClimb()
{
	bCanManuelClimb = true;
}



/*------------------
		Corner
--------------------*/
bool UParkourComponent::CheckOutCorner()
{
	FVector ArrowWorldLocation = ArrowActor->GetArrowWorldLocation();
	FRotator ArrowWorldRotation = ArrowActor->GetArrowWorldRotation();
	FVector ArrowForwardVector = GetForwardVector(ArrowWorldRotation);
	FVector ArrowRightVector = GetRightVector(ArrowWorldRotation);
	
	float HoriozontalAxis = GetHorizontalAxis();
	int32 LocalOutCornerIndex = 0;

	for (int32 i = 0; i <= 5; i++)
	{
		LocalOutCornerIndex = i * -10;
		float CheckHorizontalWidth = HoriozontalAxis * 35.f;
		FVector StartPos = FVector(0.f, 0.f, LocalOutCornerIndex) + ArrowWorldLocation 
			+ (CheckHorizontalWidth * ArrowRightVector) + (ArrowForwardVector * -20.f);
		FVector EndPos = StartPos + (ArrowForwardVector * 60.f);

		FHitResult CornerOutHitResult;
		UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartPos, EndPos, 5.f, ParkourTraceType, false,
			TArray<AActor*>(), DDT_CheckOutCorner, CornerOutHitResult, true);

		if (CornerOutHitResult.bStartPenetrating)
		{
			bOutCornerReturn = true;
			break;
		}
		else
			bOutCornerReturn = false;
	}

	if (bOutCornerReturn)
		OutCornerIndex = LocalOutCornerIndex;

	return bOutCornerReturn; // true -> ClimbMovement 함수에서 OutCornerMovement 함수 실행
}

void UParkourComponent::OutCornerMovement()
{
#ifdef DEBUG_MOVEMENT
	LOG(Warning, TEXT("OutCornerMovement"));
#endif

	FVector ArrowWorldLocation = ArrowActor->GetArrowWorldLocation();
	FVector ArrowRightVector = GetRightVector(ArrowActor->GetArrowWorldRotation());

	FVector OutCornerCheckStartPos = FVector(0.f, 0.f, OutCornerIndex * -10) + ArrowWorldLocation;
	FVector OutCornerCheckEndPos = OutCornerCheckStartPos + ((GetHorizontalAxis() * 60.f) * ArrowRightVector);

	FHitResult OutCornerCheckHitResult;
	bool bHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), OutCornerCheckStartPos, OutCornerCheckEndPos, 5.f, ParkourTraceType, false, TArray<AActor*>(), DDT_OutCornerMovement, OutCornerCheckHitResult, true);

	if (bHit)
	{
		for (int32 i = 0; i <= 4; i++)
		{
			int32 Cnt = i * 5;
			FVector StartPos = OutCornerCheckHitResult.ImpactPoint + FVector(0.f, 0.f, Cnt + 5);
			FVector EndPos = StartPos - FVector(0.f, 0.f, Cnt + 50);

			
			FHitResult OutCornerCheckTopHitResult;
			UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartPos, EndPos, 2.5, ParkourTraceType, false,
				TArray<AActor*>(), DDT_OutCornerMovement, OutCornerCheckTopHitResult, true);

			/* 
				위에서 Corner를 탐지한 ImpactPoint에서 바로 Z축만 더하고 빼서 수직으로 검사한다.
				때문에 검사하는 Trace의 첫 StartPos가 벽보다 위에있지 않으면 전부다 bStartPenetrating이 True 처리가 된다.
				그래서 해당 벽이 가능한 벽인지 아닌지는 bStartPenetrating 변수 하나로 판단이 가능하다. (★중요)
			*/
			if (OutCornerCheckTopHitResult.bStartPenetrating) // Trace가 충돌된 상태로 시작되는 경우
			{
				if (i == 4) 
					StopClimbMovement(); 
			}
			else
			{
				if (OutCornerCheckTopHitResult.bBlockingHit)
				{
					
					FRotator NormalizeRotator = UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(OutCornerCheckHitResult.ImpactNormal);
					FVector CornerForwardVector = GetForwardVector(NormalizeRotator);

					FVector BaseXYVector = CornerForwardVector * (UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag, ClimbStyleBracedXYPosition, ClimbStyleFreeHangXYPosition) - 10.f)
						+ OutCornerCheckTopHitResult.ImpactPoint;

					FVector CornerMoveTargetReletiveLocation =
						FVector(BaseXYVector.X, BaseXYVector.Y, OutCornerCheckTopHitResult.ImpactPoint.Z - CornerMovementHeight);

					WallRotation = NormalizeRotator;
					CornerMove(CornerMoveTargetReletiveLocation, WallRotation);
				}
				else
					StopClimbMovement();


				break;
			}
		}
	}
}

void UParkourComponent::InCornerMovement()
{
#ifdef DEBUG_MOVEMENT
	LOG(Warning, TEXT("InCornerMovement"));
#endif

	FGameplayTag DirectionTag = GetClimbDesireRotation();
	if(!(UPSFunctionLibrary::CompGameplayTagName(DirectionTag, TEXT("Parkour.Direction.Forward"))
		|| UPSFunctionLibrary::CompGameplayTagName(DirectionTag, TEXT("Parkour.Direction.Backward"))))
	{
		FHitResult LocalCornerDepthHitResult;

		FVector ArrowActorWorldLocation = ArrowActor->GetArrowWorldLocation();
		FRotator ArrowActorWorldRotator = ArrowActor->GetArrowWorldRotation();
		FVector ArrowForwardVector = GetForwardVector(ArrowActorWorldRotator);
		FVector ArrowRightVector = GetRightVector(ArrowActorWorldRotator);

		float HorizontalAxis = GetHorizontalAxis();

		bool bFirstCheckHit = false;
		bool bBreak = false;
		int32 CheckHeight = 0; // ArrowActor가 벽보다 위에있는 경우를 방지
		int32 HorizontalWidth = 0; //  Horizontal 방향으로 Check할 Cnt
		for (int32 i = 0; i <= 5 && !bBreak; i++)
		{
			/*
				몇번째부터 검사되지않는지를 Check하는 과정이다.
				Check 되지않을 때까지 검사한다. 안쪽으로 휜 코너라고 판단하기 위함이다.
			*/
			
			
			FVector InCornerCheckFirstBasePos = FVector(0.f, 0.f, CheckHeight) + ArrowActorWorldLocation + (ArrowRightVector * (HorizontalAxis * HorizontalWidth));
			FVector InCornerCheckFirstStartPos = InCornerCheckFirstBasePos + (ArrowForwardVector * -10.f);
			FVector InCornerCheckFirstEndPos = InCornerCheckFirstBasePos + (ArrowForwardVector * 80.f);

			FHitResult InCornerCheckFirstHitResult;
			bool bHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), InCornerCheckFirstStartPos, InCornerCheckFirstEndPos, 10.f, ParkourTraceType, false, TArray<AActor*>(), DDT_InCornerMovement, InCornerCheckFirstHitResult, true);

			if (bHit)
			{
				// bCheckFirstHit을 따로 사용하는 이유는 모종의 이유로 ArrowActor가 벽보다 위에 있는 경우를 위함 ex)ClimbStyleBracedZPosition의 변경 등
				// 그럴경우 아래쪽으로 내려오며 검사해야한다.
				if (!bFirstCheckHit) 
					bFirstCheckHit = true;

				if (i == 5)
				{
					LocalCornerDepthHitResult = FHitResult(); // In Corner 불가
					StopClimbMovement();
				}					
				else
					LocalCornerDepthHitResult = InCornerCheckFirstHitResult;

				HorizontalWidth = i * 10;
					
			}
			else // 검사 되지 않으므로 InCorner 함수들 실행
			{
				if (bFirstCheckHit) // 한번이라도 Hit한 경우
				{
					bBreak = true;
					CheckInCornerSide(LocalCornerDepthHitResult,
						ArrowActorWorldLocation,
						ArrowActorWorldRotator,
						ArrowForwardVector,
						ArrowRightVector,
						HorizontalAxis);
				}
				else
				{
					CheckHeight += -10; // 아예 체크되지 않았으므로 아래로
					i = 0; // for문 처음부터 다시
				}
			}
		}
	}
}

void UParkourComponent::CheckInCornerSide(const FHitResult& CornerDepthHitResult, FVector ArrowActorWorldLocation, FRotator ArrowActorWorldRotation, FVector ArrowForwardVector, FVector ArrowRightVector, float HorizontalAxis)
{
	/*
		InCornerMovement함수에서 벽이 휜 곳의 정보를 알았다면 그 위치를 수직으로 검사하여 Side를 체크하는 과정이다.
	*/

	bool bBreak = false;
	for (int32 CheckSideCnt = 0; CheckSideCnt <= 2 && !bBreak; CheckSideCnt++)
	{
		// 위로 올라가면서 체크
		FVector InCornerSideCheckBasePos = CornerDepthHitResult.ImpactPoint
			+ (ArrowForwardVector * 10.f)
			+ FVector(0.f, 0.f, CheckSideCnt * -10);

		FVector InCornerSideCheckStartPos = InCornerSideCheckBasePos + (ArrowRightVector * (HorizontalAxis * 20.f));
		FVector InCornerSideCheckEndPos = InCornerSideCheckBasePos + (ArrowRightVector * (HorizontalAxis * -50.f));

		FHitResult InCornerSideCheckHitResult;
		bool bSideCheckHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), InCornerSideCheckStartPos, InCornerSideCheckEndPos, 5.f, ParkourTraceType, false, TArray<AActor*>(), DDT_InCornerMovement, InCornerSideCheckHitResult, true);

		if (!bSideCheckHit)
		{
			if (CheckSideCnt == 2)
				StopClimbMovement();
		}
		else
		{
			bBreak = true;
			CheckInCornerSideTop(InCornerSideCheckHitResult);
		}
	}
}

void UParkourComponent::CheckInCornerSideTop(const FHitResult& InCornerSideCheckHitResult)
{
	/*
		이동할 In Corner 모서리의 Top을 체크한다
	*/
	FRotator NormalizeRotator = UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(InCornerSideCheckHitResult.ImpactNormal);
	FVector ForwardNormalVector = GetForwardVector(NormalizeRotator);

	bool bBreak = false;
	for (int32 CheckSideTopCnt = 0; CheckSideTopCnt <= 4 && !bBreak; CheckSideTopCnt++)
	{

		int32 IndexCnt = CheckSideTopCnt * 5;
		FVector TopCheckStartPos = InCornerSideCheckHitResult.ImpactPoint + (ForwardNormalVector * 2.f)
			+ FVector(0.f, 0.f, IndexCnt + 5.f);

		FVector TopCheckEndPos = TopCheckStartPos - FVector(0.f, 0.f, IndexCnt + 50.f);

		FHitResult TopCheckHitResult;
		UKismetSystemLibrary::SphereTraceSingle(GetWorld(), TopCheckStartPos, TopCheckEndPos, 5.f, ParkourTraceType, false, TArray<AActor*>(), DDT_InCornerMovement, TopCheckHitResult, true);

		if (!TopCheckHitResult.bStartPenetrating)
		{
			bBreak = true;
			if (TopCheckHitResult.bBlockingHit)
			{
				/*
					최종적으로 캐릭터 CapsuleComponent가 이동가능한 충분한 공간이 있는지 Check하는 과정
				*/
				FVector CapsuleTracePos = TopCheckHitResult.ImpactPoint
					+ FVector(0.f, 0.f, -CharacterCapsuleCompHalfHeight)
					+ ForwardNormalVector * -50.f;

				FHitResult CapsuleTraceHitResult;
				bool bCapsuleHit = UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(), CapsuleTracePos, CapsuleTracePos,
					CharacterCapsuleCompRadius - CornerCheckCapsuleRadius_Subtraction,
					CharacterCapsuleCompHalfHeight - CornerCheckCapsuleHalfHeight_Subtraction,
					ParkourTraceType, false, TArray<AActor*>(),
					DDT_InCornerMovement, CapsuleTraceHitResult, true);

				// true : CapsuleComponent가 움직일 공간이 없어서 Stop
				if (bCapsuleHit)
					StopClimbMovement();
				else
				{
					WallRotation = NormalizeRotator;

					FVector CornerMove_XY = InCornerSideCheckHitResult.ImpactPoint
						+ ForwardNormalVector * (UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag, ClimbStyleBracedXYPosition, ClimbStyleFreeHangXYPosition) - 10.f);

					float CornerMove_Z = TopCheckHitResult.ImpactPoint.Z
						+ UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag, ClimbStyleBracedZPosition, ClimbStyleFreeHangZPosition)
						+ CharacterHeightDiff;

					FVector CornerMoveTargetLocation = FVector(CornerMove_XY.X, CornerMove_XY.Y, CornerMove_Z);

					CornerMove(CornerMoveTargetLocation, WallRotation);

				}				
			}
		}
	}
}



void UParkourComponent::CornerMove(FVector TargetRelativeLocation, FRotator TargetRelativeRotation)
{
#ifdef DEBUG_MOVEMENT
	LOG(Warning, TEXT("CornerMove"));
#endif

	bFirstClimbMove = true;
	SetParkourAction(TEXT("Parkour.Action.CornerMove"));

	SetClimbDirection(UPSFunctionLibrary::SelectClimbDirection(GetHorizontalAxis() > 0.f));

	FLatentActionInfo LatenActionInfo;
	LatenActionInfo.CallbackTarget = this; // ★
	LatenActionInfo.ExecutionFunction = FName(TEXT("CornerMoveCompleted")); // 완료 시 호출될 함수 이름
	LatenActionInfo.Linkage = 0; // 이 값은 대부분 0으로 설정합니다.
	LatenActionInfo.UUID = GetNextLatentActionUUID(); // 고유한 ID 할당

	UKismetSystemLibrary::MoveComponentTo(CapsuleComponent, TargetRelativeLocation, TargetRelativeRotation,
		false, false,
		UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag, CornerMoveComponentToOverTime_Braced, CornerMoveComponentToOverTime_FreeHang),
		false, EMoveComponentAction::Move, LatenActionInfo);
	
}

void UParkourComponent::CornerMoveCompleted()
{
	SetParkourAction(TEXT("Parkour.Action.NoAction"));
	StopClimbMovement();
}

int32 UParkourComponent::GetNextLatentActionUUID()
{
	return NextLatentActionUUID++;
}


/*----------------
		Hop
------------------*/
void UParkourComponent::CheckClimbUpOrHop()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("CheckClimbUpOrHop"));
#endif
	FGameplayTag DirectionGameplayTag = GetClimbDesireRotation();
	if (UPSFunctionLibrary::CompGameplayTagName(DirectionGameplayTag, TEXT("Parkour.Direction.Forward"))
		|| UPSFunctionLibrary::CompGameplayTagName(DirectionGameplayTag, TEXT("Parkour.Direction.ForwardLeft"))
		|| UPSFunctionLibrary::CompGameplayTagName(DirectionGameplayTag, TEXT("Parkour.Direction.ForwardRight")))
	{
		// 위쪽으로 방향키를 지정하여 Parkour키를 누른 상태일 때, Climb 상태에서 올라갈수있는 벽인지 판단.
		// 아닌경우에 Hop할 수있는 벽을 찾아서 존재할시 Hop 진행
		if (CheckMantleSurface())
		{
			if (UPSFunctionLibrary::CompGameplayTagName(ClimbStyleTag, TEXT("Parkour.ClimbStyle.Braced")))
				SetParkourAction(TEXT("Parkour.Action.ClimbingUp"));
			else
				SetParkourAction(TEXT("Parkour.Action.FreeHangClimbUp"));
		}
		else
			HopAction();

	}
	else
		HopAction();

}

void UParkourComponent::HopAction()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("HopAction"));
#endif
	FirstClimbLedgeResult();
	
	FindHopLocation(); // Hop할 위치 찾기 (WallHitResult 및 WallTopHitResult 갱신)

	if (IsLedgeValid())
	{
		LOG(Error, TEXT("Is Valid"));
		CheckClimbStyle();
		ClimbLedgeResult();
		SetParkourAction(SelectHopAction());
	}
}


// WallTopResult와 WallHitResult는 ClimbMovement 중에는 Empty 되기때문에 Hop을 실행하면 Top 위치를 다시 계산해주어야한다.
void UParkourComponent::FirstClimbLedgeResult()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("FirstClimbLedgeResult"));
#endif

	FRotator NormalizeRotation = UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(WallHitResult.ImpactNormal);
	FVector ForwardNormalVector = UKismetMathLibrary::GetForwardVector(NormalizeRotation);

	FVector CheckHoldingWallStartPos = WallHitResult.ImpactPoint + (ForwardNormalVector * -30.f);
	FVector CheckHoldingWallEndPos = WallHitResult.ImpactPoint + (ForwardNormalVector * 30.f);

	// WallRotation 및 Top을 구하기위한 Holding Wall 위치 파악
	FHitResult CheckHoldingWallHitResult;
	UKismetSystemLibrary::SphereTraceSingle(GetWorld(), CheckHoldingWallStartPos, CheckHoldingWallEndPos, 
		5.f, ParkourTraceType, false, TArray<AActor*>(), DDT_CheckFirstHoldingWall, CheckHoldingWallHitResult, true);

	FRotator HoldingWallNormalizeRotation = UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(CheckHoldingWallHitResult.ImpactNormal);
	FVector HoldingWallForwardNormalVector = UKismetMathLibrary::GetForwardVector(HoldingWallNormalizeRotation);

	// Top
	WallRotation = HoldingWallNormalizeRotation;
	FVector CheckHoldingWallTopBasePos = CheckHoldingWallHitResult.ImpactPoint + (HoldingWallForwardNormalVector * 2.f);
	FVector CheckHoldingWallTopStartPos = CheckHoldingWallTopBasePos + FVector(0.f, 0.f, 5.f);
	FVector CheckHoldingWallTopEndPos = CheckHoldingWallTopBasePos + FVector(0.f, 0.f, 50.f);

	FHitResult CheckHoldingWallTopHitResult;
	UKismetSystemLibrary::SphereTraceSingle(GetWorld(), CheckHoldingWallTopStartPos, CheckHoldingWallTopEndPos,
		5.f, ParkourTraceType, false, TArray<AActor*>(), DDT_CheckFirstHoldingWall, CheckHoldingWallTopHitResult, true);

	HopClimbLedgeHitResult = CheckHoldingWallHitResult;
	HopClimbLedgeHitResult.Location = CheckHoldingWallTopHitResult.Location;
	HopClimbLedgeHitResult.ImpactPoint.Z = CheckHoldingWallTopHitResult.ImpactPoint.Z;

}

// 캐릭터의 수직으로 코너가 된 부분으로 Hop을 시도할때 중요한 함수
// 캐릭터 위치를 기반으로 Hop을 진행할 방향의 Depth를 구해서 코너가 꺾인 부분이 없는지 확인한다.
void UParkourComponent::CheckInCornerHop()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("CheckInCornerHop"));
#endif

	FGameplayTag HopDirectionTag = GetClimbDesireRotation();

	if(UPSFunctionLibrary::CompGameplayTagName(HopDirectionTag, TEXT("Parkour.Direction.Forward"))
		|| UPSFunctionLibrary::CompGameplayTagName(HopDirectionTag, TEXT("Parkour.Direction.Backward")))
	{
		bCanInCornerHop = false;
	}
	else
	{
		FVector ArrowWorldLocation = ArrowActor->GetArrowWorldLocation();
		FRotator ArrowWorldRotation = ArrowActor->GetArrowWorldRotation();
		FVector ArrowForwardVector = UKismetMathLibrary::GetForwardVector(ArrowWorldRotation);
		FVector ArrowRightVector = UKismetMathLibrary::GetRightVector(ArrowWorldRotation);

		float HorizontalAxis = GetHorizontalAxis();

		FHitResult LocalCornerDepthHitResult;

		// 코너가 꺾인 부분인지 판단하기 위한 for문
		// Depth
		for (int32 i = 0; i <= 7; i++)
		{
			int32 IndexCnt = i * 10.f;

			FVector RightCheckLength = ArrowRightVector * (HorizontalAxis * IndexCnt);

			FVector CheckWallDepthBasePos = ArrowWorldLocation + RightCheckLength;
			FVector CheckWallLengthStartPos = CheckWallDepthBasePos + ArrowForwardVector * -10.f;
			FVector CheckWallLengthEndPos = CheckWallDepthBasePos + ArrowForwardVector * 80.f;

			FHitResult CheckWallDepthHitResult;
			bool bCheckWallDepthHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), CheckWallLengthStartPos, CheckWallLengthEndPos,
				5.f, ParkourTraceType, false, TArray<AActor*>(), DDT_CheckInCornerHop, CheckWallDepthHitResult, true);

			if (bCheckWallDepthHit)
			{
				if (i == 5)
				{
					LocalCornerDepthHitResult = FHitResult();
					bCanInCornerHop = false;
				}
				else
				{
					// 마지막으로 Hit된 벽까지 저장
					LocalCornerDepthHitResult = CheckWallDepthHitResult;
				}
			}
			else
			{
				for (int32 i2 = 0; i <= 22; i++)
				{
					int32 IndexCnt2 = i2 * 8;

					FVector CornerDepthImpactPoint = 
						FVector(LocalCornerDepthHitResult.ImpactPoint.X,
						LocalCornerDepthHitResult.ImpactPoint.Y,
						OwnerCharacter->GetActorLocation().Z);
					FVector Height = FVector(0.f, 0.f, IndexCnt2);

					// 마지막으로 체크된 Corner부분에서 코너가 꺾이는 부분 모양대로 꺾이는 면 Trace 진행
					FVector CheckCornerBasePos = CornerDepthImpactPoint + Height + (ArrowForwardVector * 10.f);
					FVector CheckCornerStartPos = CheckCornerBasePos + (ArrowRightVector * (HorizontalAxis * 10.f));
					FVector CheckCornerEndPos = CheckCornerBasePos + (ArrowRightVector * (HorizontalAxis * -50.f));

					FHitResult CheckCornerHitResult;
					UKismetSystemLibrary::SphereTraceSingle(GetWorld(), CheckCornerStartPos, CheckCornerEndPos, 5.f,
						ParkourTraceType, false, TArray<AActor*>(), DDT_CheckInCornerHop, CheckCornerHitResult, true);

					if (CheckCornerHitResult.bBlockingHit && !CheckCornerHitResult.bStartPenetrating)
					{
						CornerHopRotation = UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(CheckCornerHitResult.ImpactNormal);
						bCanInCornerHop = true;
						break;
					}
					else
					{
						if (i2 == 22)
							bCanInCornerHop = false;
					}

				}

				// First for-loop break;
				break;
			}
		}
	}
}

// 캐릭터의 수평으로 막고있는 벽으로 Hop하기위한 위치를 얻는 함수
void UParkourComponent::CheckOutCornerHop()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("CheckOutCornerHop"));
#endif

	FVector ArrowWorldLocation = ArrowActor->GetArrowWorldLocation();
	FRotator ArrowWorldRotation = ArrowActor->GetArrowWorldRotation();
	FVector ArrowForwardVector = UKismetMathLibrary::GetForwardVector(ArrowWorldRotation);
	FVector ArrowRightVector = UKismetMathLibrary::GetRightVector(ArrowWorldRotation);

	FVector CharacterLocation = OwnerCharacter->GetActorLocation();

	float HorizontalAxis = GetHorizontalAxis();
	if (HorizontalAxis == 0.f)
		return; // 수평 입력 축 값이 없으므로 예외처리

	for (int32 i = 0; i <= 22; i++)
	{
		int32 IndexCnt = i * 8;

		FVector BaseLocation = FVector(ArrowWorldLocation.X, ArrowWorldLocation.Y, CharacterLocation.Z) + (ArrowForwardVector * -20.f);
		
		FVector CheckOutCornerBasePos = BaseLocation + FVector(0.f, 0.f, IndexCnt);
		FVector CheckOutCornerStartPos = CheckOutCornerBasePos + (ArrowRightVector * (HorizontalAxis * -20.f));
		FVector CheckOutCornerEndPos = CheckOutCornerBasePos + (ArrowRightVector * (HorizontalAxis * 100.f));

		FHitResult CheckOutCornerHitResult;
		UKismetSystemLibrary::SphereTraceSingle(GetWorld(), CheckOutCornerStartPos, CheckOutCornerEndPos,
			5.f, ParkourTraceType, false, TArray<AActor*>(), DDT_CheckOutCornerHop, CheckOutCornerHitResult, true);

		if (CheckOutCornerHitResult.bBlockingHit && !CheckOutCornerHitResult.bStartPenetrating)
		{
			CornerHopRotation = UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(CheckOutCornerHitResult.ImpactNormal);
			bCanOutCornerHop = true;
			break;
		}
		else
			bCanOutCornerHop = false;

	}
}

FGameplayTag UParkourComponent::SelectHopAction()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("SelectHopAction"));
#endif

	switch (GetHopDirection())
	{
	case FORWARD: 
		// FORWARD인 경우 먼저 코너 체크를 하고 코너인 경우 ClimbUp 보다는 Corner Hop을 먼저 시도한다.
		if(!(bCanInCornerHop || bCanOutCornerHop))
			return ReturnBracedHopAction(FORWARD);
		else
		{
			if (GetHorizontalAxis() > 0.f)
			{
				if (UPSFunctionLibrary::CompGameplayTagName(ClimbStyleTag, TEXT("Parkour.ClimbStyle.Braced")))
					return ReturnBracedHopAction(RIGHT);
				else
					return ReturnFreeHangHopAction(LEFT);
			}
		}	
		break;
	case BACKWARD:
		if (UPSFunctionLibrary::CompGameplayTagName(ClimbStyleTag, TEXT("Parkour.ClimbStyle.Braced")))
			return ReturnBracedHopAction(BACKWARD);
		else
			return ReturnFreeHangHopAction(BACKWARD);
	case LEFT:
		if (UPSFunctionLibrary::CompGameplayTagName(ClimbStyleTag, TEXT("Parkour.ClimbStyle.Braced")))
			return ReturnBracedHopAction(LEFT);
		else
			return ReturnFreeHangHopAction(LEFT);
	case RIGHT:
		if (UPSFunctionLibrary::CompGameplayTagName(ClimbStyleTag, TEXT("Parkour.ClimbStyle.Braced")))
			return ReturnBracedHopAction(RIGHT);
		else
			return ReturnFreeHangHopAction(RIGHT);
	case FORWARD_LEFT:
		if (UPSFunctionLibrary::CompGameplayTagName(ClimbStyleTag, TEXT("Parkour.ClimbStyle.Braced")))
			return ReturnBracedHopAction(FORWARD_LEFT);
		else
			return ReturnFreeHangHopAction(FORWARD_LEFT);
	case FORWARD_RIGHT:
		if (UPSFunctionLibrary::CompGameplayTagName(ClimbStyleTag, TEXT("Parkour.ClimbStyle.Braced")))
			return ReturnBracedHopAction(FORWARD_RIGHT);
		else
			return ReturnFreeHangHopAction(FORWARD_RIGHT);
	case BACKWARD_LEFT:
		if (UPSFunctionLibrary::CompGameplayTagName(ClimbStyleTag, TEXT("Parkour.ClimbStyle.Braced")))
			return ReturnBracedHopAction(BACKWARD_LEFT);
		else
			return ReturnFreeHangHopAction(BACKWARD_LEFT);
	case BACKWARD_RIGHT:
		if (UPSFunctionLibrary::CompGameplayTagName(ClimbStyleTag, TEXT("Parkour.ClimbStyle.Braced")))
			return ReturnBracedHopAction(BACKWARD_RIGHT);
		else
			return ReturnFreeHangHopAction(BACKWARD_RIGHT);
	}

	return UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.Action.NoAction"));
}

int32 UParkourComponent::GetHopDirection()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("GetHopDirection"));
#endif
	
	FVector HopStartLocation = HopClimbLedgeHitResult.ImpactPoint; // Hop 시도할 때 위치
	FVector HopEndLocation = ClimbLedgeHitResult.ImpactPoint; // Hop할 최종위치

	FRotator RelativeRotation = UKismetMathLibrary::FindRelativeLookAtRotation(OwnerCharacter->GetActorTransform(), HopEndLocation);
	
	int32 HorizontalDirection;
	int32 VerticalDirection;
	if (UKismetMathLibrary::InRange_FloatFloat(RelativeRotation.Yaw, -135.f, -35.f, true, false))
		HorizontalDirection = LEFT;
	else
	{
		if (UKismetMathLibrary::InRange_FloatFloat(RelativeRotation.Yaw, 35.f, 135.f, false, true))
			HorizontalDirection = RIGHT;
		else
			HorizontalDirection = NONE;
	}

			
	if (HopEndLocation.Z - HopStartLocation.Z > 37.f)
		VerticalDirection = FORWARD;
	else
	{
		if (HopEndLocation.Z - HopStartLocation.Z < -37.f)
			VerticalDirection = BACKWARD;
		else
			VerticalDirection = NONE;
	}

#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("Horizontal : %d"), HorizontalDirection);
	LOG(Warning, TEXT("Vertical : %d"), VerticalDirection);
	LOG(Warning, TEXT("Vertical + Horizontal : %d"), VerticalDirection + HorizontalDirection);
#endif
	

	return GetHopDesireRotation(HorizontalDirection, VerticalDirection);
}

int32 UParkourComponent::GetHopDesireRotation(int32 HorizontalDirection, int32 VerticalDirection)
{
	/*
		UP : 10, Down : -11 // Left : -1, Right : 1
		이렇게 측정하여서 각 값을 더하여 8방향으로 swich문 구현
		[기존 if문에서 else if를 통한 하드 코딩식에서 개선 (ex. if(...) else if(...) * 7)]
	*/

	return VerticalDirection + HorizontalDirection;
	
}

FGameplayTag UParkourComponent::ReturnBracedHopAction(int32 Direction)
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("ReturnBracedHopAction"));
	LOG(Warning, TEXT("Direction : %d"), Direction);
#endif

	switch (Direction)
	{
	case FORWARD:
		return UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.Action.ClimbHopUp"));
	case BACKWARD:
		return UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.Action.ClimbHopDown"));
	case LEFT:
		return UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.Action.ClimbHopLeft"));
	case RIGHT:
		return UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.Action.ClimbHopRight"));
	case FORWARD_LEFT:
		return UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.Action.ClimbHopLeftUp"));
	case FORWARD_RIGHT:
		return UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.Action.ClimbHopRightUp"));
	case BACKWARD_LEFT:
		return UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.Action.ClimbHopLeft"));
	case BACKWARD_RIGHT:
		return UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.Action.ClimbHopRight"));
	}

	return UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.Action.NoAction"));
}

FGameplayTag UParkourComponent::ReturnFreeHangHopAction(int32 Direction)
{
	switch (Direction)
	{
	case FORWARD:
		// FreeHang은 Up이 없다. (물리적으로)
		return UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.Action.ClimbHopUp"));
	case BACKWARD:
		return UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.Action.FreeClimbHopDown"));
	case LEFT:
	case FORWARD_LEFT:
	case BACKWARD_LEFT:
		return UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.Action.FreeClimbHopLeft"));
	case RIGHT:
	case FORWARD_RIGHT:
	case BACKWARD_RIGHT:
		return UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.Action.FreeClimbHopRight"));
	}

	return UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.Action.NoAction"));
}


/*--------------------------
	Find Hop Location
----------------------------*/
void UParkourComponent::FindHopLocation()
{
#ifndef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("FindHopLocation"));
#endif

	VerticalHopDistance = GetSelectVerticalHopDistance() * VerticalDistanceMultiplier;
	HorizontalHopDistance = GetSelectHorizontalHopDisance() * HorizontalDistanceMultiplier;

	FVector WallForwardVector = UKismetMathLibrary::GetForwardVector(WallRotation);
	FVector WallRightVector = UKismetMathLibrary::GetRightVector(WallRotation);
	
	FRotator CharacterRotation = OwnerCharacter->GetActorRotation();
	FVector CharacterUpVector = UKismetMathLibrary::GetUpVector(CharacterRotation);
	FVector CharacterRightVector = UKismetMathLibrary::GetRightVector(CharacterRotation);

	FVector TraceStart;
	FVector TraceEnd;
	WallHitTraces.Empty();
	int32 CenterAdjust = CheckHopWidth * 3;

	for (int32 i = 0; i <= 6; i++)
	{
		int32 IndexCnt = (i * CheckHopWidth) - CenterAdjust;
		
		FVector BaseTracePos = (WallRightVector * IndexCnt)
			+ WallTopHitResult.ImpactPoint
			+ (CharacterUpVector * VerticalHopDistance)
			+ (CharacterRightVector * HorizontalHopDistance);

		TraceStart = BaseTracePos + (WallForwardVector * -CheckHopDepth);
		TraceEnd = BaseTracePos + (WallForwardVector * CheckHopDepth);

		HopHitTraces.Empty();

		// Hop 가능한 위치들 탐색
		for (int32 i2 = 0; i2 <= 20; i2++)
		{
			int32 IndexCnt2 = (i2 * CheckHopHeight) - 20;
			FVector CheckHopStartPos = FVector(0.f, 0.f, IndexCnt2) + TraceStart;
			FVector CheckHopEndPos = FVector(0.f, 0.f, IndexCnt2) + TraceEnd;

			FHitResult FirstCheckHopHitResult;
			UKismetSystemLibrary::LineTraceSingle(GetWorld(), CheckHopStartPos, CheckHopEndPos, ParkourTraceType, false, TArray<AActor*>(), DDT_CheckFindHopLocation, FirstCheckHopHitResult, true);
			if (!FirstCheckHopHitResult.bStartPenetrating)
				HopHitTraces.Emplace(FirstCheckHopHitResult);
		}

		CheckHopCapsuleComponent();
	}

	CheckHopWallTopHitResult();
}

void UParkourComponent::CheckHopWallTopHitResult()
{
#ifndef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("CheckHopWallTopHitResult"));
#endif

	/*
		WallHitTraces에 아무것도 없다는 뜻은
		1. 양 옆으로 Hop이 가능한 위치가 Check 되지않음.
		2. 캐릭터 바로 옆 꺾이는 코너라서 bStartPenetrating이 true.
		3. 아래에 Hop할 수있는 벽이 존재하지않아서 Drop하려는 상태.
	*/
	if (WallHitTraces.Num() == 0)
	{
		if (UPSFunctionLibrary::CompGameplayTagName(GetClimbDesireRotation(), TEXT("Parkour.Direction.Backward")))
			ParkourDrop();
		else
		{
			if (bUseCornerHop)
			{
				CheckInCornerHop();
				CheckOutCornerHop();
				FindCornerHopLocation();
				return;
			}
		}
	}
	else
	{
		FVector CharacterLocation = OwnerCharacter->GetActorLocation();
		WallHitResult = WallHitTraces[0];
		for (int32 WallHitIndex = 1; WallHitIndex < WallHitTraces.Num(); WallHitIndex++)
		{
			// WallHitTraces[WallHitIndex]와 WallHitResult의 CharacterLocation과의 Distance 비교
			float CurrentWallDistance = UKismetMathLibrary::Vector_Distance(WallHitTraces[WallHitIndex].ImpactPoint, CharacterLocation);
			float PrevWallDistance = UKismetMathLibrary::Vector_Distance(WallHitResult.ImpactPoint, CharacterLocation);

			if (CurrentWallDistance <= PrevWallDistance) // 현재 Distance가 더 작거나 같으면 교체
				WallHitResult = WallHitTraces[WallHitIndex];
		}

		if (!WallHitResult.bStartPenetrating)
		{
			if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.Climb")))
			{
				WallRotation = UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(WallHitResult.ImpactNormal);
				FVector CheckWallTopStartPos = WallHitResult.ImpactPoint + FVector(0.f, 0.f, 3.f);
				FVector CheckWallTopEndPos = CheckWallTopStartPos - FVector(0.f, 0.f, 3.f);

				FHitResult CheckWallTopHitResult;
				bool bTopHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), CheckWallTopStartPos, CheckWallTopEndPos, 5.f,
					ParkourTraceType, false, TArray<AActor*>(), DDT_CheckHopWallTopLocation, CheckWallTopHitResult, true);

				// Hop할 위치의 Top정보 갱신
				if (bTopHit)
					WallTopHitResult = CheckWallTopHitResult;
			}
		}
	}
}

void UParkourComponent::FindCornerHopLocation()
{
	if (bCanInCornerHop || bCanOutCornerHop)
	{
		WallRotation = CornerHopRotation;
		WallHitTraces.Empty(); // 초기화
		
		FVector WallForwardVector = GetForwardVector(WallRotation);
		FVector WallRightVector = GetRightVector(WallRotation);

		FRotator CharacterRotation = OwnerCharacter->GetActorRotation();
		FVector CharacterUpVector = GetUpVector(CharacterRotation);
		FVector CharacterRightVector = GetRightVector(CharacterRotation);

		FVector CornerTraceStart;
		FVector CornerTraceEnd;

		// Left or Right 방향에 따른 탐색 범위 변화
		float HorizontalAxis = GetHorizontalAxis();
		int32 StartIndex = HorizontalAxis < 0.f ? 0 : 4;
		int32 EndIndex = HorizontalAxis < 0.f ? 2 : 6;

		float CornerHopSelectWidth = (bCanOutCornerHop ? 50.f : 20.f) * HorizontalAxis; // Out Corner는 상대적으로 거리가 짧기 때문
		int32 CenterAdjust = CheckCornerHopWidth * 3;

		for (; StartIndex <= EndIndex; StartIndex++)
		{
			int32 IndexCnt = (StartIndex * CheckCornerHopWidth) - CenterAdjust;
			
			FVector BaseTracePos = (WallRightVector * IndexCnt)
				+ WallTopHitResult.ImpactPoint
				+ (CharacterUpVector * VerticalHopDistance)
				+ (CharacterRightVector * CornerHopSelectWidth);

			CornerTraceStart = BaseTracePos + (WallForwardVector * -CheckCornerHopDepth);
			CornerTraceEnd = BaseTracePos + (WallForwardVector * CheckCornerHopDepth);

			HopHitTraces.Empty();

			for (int32 i2 = 0; i2 <= 20; i2++) 
			{
				int32 IndexCnt2 = i2 * CheckCornerHopHeight - 20;
				FVector CheckCornerHopTraceStartPos = FVector(0.f, 0.f, IndexCnt2) + CornerTraceStart;
				FVector CheckCornerHopTraceEndPos = FVector(0.f, 0.f, IndexCnt2) + CornerTraceEnd;

				FHitResult CheckCornerHopHitResult;
				UKismetSystemLibrary::LineTraceSingle(GetWorld(), CheckCornerHopTraceStartPos, CheckCornerHopTraceEndPos,
					ParkourTraceType, false, TArray<AActor*>(), DDT_CheckFindCornerHop, CheckCornerHopHitResult, true);
				
				if (!CheckCornerHopHitResult.bStartPenetrating)
					HopHitTraces.Emplace(CheckCornerHopHitResult);
			}

			CheckHopCapsuleComponent();
		}

		CheckCornerHopWallTopHitResult();
	}
}

// CheckHopWallTopHitResult 함수와 거의 유사
void UParkourComponent::CheckCornerHopWallTopHitResult()
{
	if (IsLedgeValid())
	{
		FVector CharacterLocation = OwnerCharacter->GetActorLocation();
		WallHitResult = WallHitTraces[0];
		for (int32 WallHitIndex = 1; WallHitIndex < WallHitTraces.Num(); WallHitIndex++)
		{
			// WallHitTraces[WallHitIndex]와 WallHitResult의 CharacterLocation과의 Distance 비교
			float NextWallDistance = UKismetMathLibrary::Vector_Distance(WallHitTraces[WallHitIndex].ImpactPoint, CharacterLocation);
			float CurrentWallDistance = UKismetMathLibrary::Vector_Distance(WallHitResult.ImpactPoint, CharacterLocation);

			if (NextWallDistance <= CurrentWallDistance) // 현재 Distance가 더 작거나 같으면 교체
				WallHitResult = WallHitTraces[WallHitIndex];
			else
				WallHitResult = WallHitResult;
		}

		if (!WallHitResult.bStartPenetrating)
		{
			if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.Climb")))
			{
				WallRotation = UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(WallHitResult.ImpactNormal);
				FVector CheckWallTopStartPos = WallHitResult.ImpactPoint + FVector(0.f, 0.f, 5.f);
				FVector CheckWallTopEndPos = CheckWallTopStartPos - FVector(0.f, 0.f, 5.f);

				FHitResult CheckWallTopHitResult;
				bool bTopHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), CheckWallTopStartPos, CheckWallTopEndPos, 5.f,
					ParkourTraceType, false, TArray<AActor*>(), DDT_CheckHopWallTopLocation, CheckWallTopHitResult, true);

				// Hop할 위치의 Top정보 갱신
				if (bTopHit)
					WallTopHitResult = CheckWallTopHitResult;
			}
		}
	}
}



// 찾은 Hop 위치에 캐릭터의 CapsuleComponent가 이동가능한지 지정한 캡슐 크기만큼 검사
// 이동 가능한 위치들을 WallHitTraces에 Emplace 진행
void UParkourComponent::CheckHopCapsuleComponent()
{
	for (int32 HopIndex = 1; HopIndex < HopHitTraces.Num(); HopIndex++)
	{
		// bBlockingHit == true: Distance / bBlockingHit == false: TraceStart~End Distance
		float CurrentDistance = GetHopResultDistance(HopHitTraces[HopIndex]);
		float PrevDistance = GetHopResultDistance(HopHitTraces[HopIndex - 1]);

		// Distance를 비교연산하여 5.f차이가 발생하면 그 위치가 Hop이 가능한 최소 부분이다.
		if (CurrentDistance - PrevDistance > 5.f)
		{
			FHitResult CanHopHitResult = HopHitTraces[HopIndex - 1];
			FRotator CanHopNormalizeRotator = UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(CanHopHitResult.ImpactNormal);
			FVector CanHopNomalizeForwardVector = GetForwardVector(CanHopNormalizeRotator);
			FVector CanHopNormalizeRightVector = GetRightVector(CanHopNormalizeRotator);

			FVector CheckCapsuleBasePos =
				CanHopHitResult.ImpactPoint
				+ FVector(0.f, 0.f, -(CharacterCapsuleCompHalfHeight - HopCharacterCapsuleHalfHeight_Subtraction))
				+ (CanHopNomalizeForwardVector * (-CheckHopCharacterCapsuleDepth_Subtraction));

			FVector CheckCapsuleStartPos = CheckCapsuleBasePos + (CanHopNormalizeRightVector * 4.f);
			FVector CheckCapsuleEndPos = CheckCapsuleBasePos + (CanHopNormalizeRightVector * -4.f);

			FHitResult CheckCapsuleTrace;
			bool bCapsuleHit = UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(), CheckCapsuleStartPos, CheckCapsuleEndPos,
				CharacterCapsuleCompRadius - HopCharacterCapsuleRadius_Subtraction,
				CharacterCapsuleCompHalfHeight - HopCharacterCapsuleHalfHeight_Subtraction,
				ParkourTraceType, false, TArray<AActor*>(), DDT_CheckHopCapsuleComponent, CheckCapsuleTrace, true);

			if (!bCapsuleHit)
				WallHitTraces.Emplace(HopHitTraces[HopIndex - 1]);

			break;
		}

	}
}



// Vertical(수직)방향 Return
float UParkourComponent::GetSelectVerticalHopDistance()
{
	return UPSFunctionLibrary::SelectDirectionFloat(
		GetClimbDesireRotation(),
		Forward_Vertical,
		Backward_Vertical,
		LeftRight_Vertical,
		LeftRight_Vertical,
		ForwardLeftRight_Vertical,
		ForwardLeftRight_Vertical,
		BackwardLeftRight_Vertical,
		BackwardLeftRight_Vertical);

}

// Horizontal(수평)방향 Return
float UParkourComponent::GetSelectHorizontalHopDisance()
{

	return UPSFunctionLibrary::SelectDirectionFloat(
		GetClimbDesireRotation(),
		0.f,
		0.f,
		-LeftRight_Horizontal,
		LeftRight_Horizontal,
		-ForwardAndBackWardLeftRight_Horizontal,
		ForwardAndBackWardLeftRight_Horizontal,
		-ForwardAndBackWardLeftRight_Horizontal,
		ForwardAndBackWardLeftRight_Horizontal); // Left방향은 역수를 보내야한다. 
}

float UParkourComponent::GetHopResultDistance(const FHitResult& HopHitResult)
{
	return UKismetMathLibrary::SelectFloat(HopHitResult.Distance,
		UKismetMathLibrary::Vector_Distance(HopHitResult.TraceStart, HopHitResult.TraceEnd),
		HopHitResult.bBlockingHit);
}



/*------------------
		ETC.
-------------------*/
FRotator UParkourComponent::GetDesireRotation()
{
	if (ForwardValue == 0.f && RightValue == 0.f)
	{
		if (bUseControllerRotation_DropDown)
		{
			FRotator ControllerRotation = OwnerCharacter->GetControlRotation();
			return FRotator(0.f, ControllerRotation.Yaw, 0.f);
		}

		else
			return OwnerCharacter->GetActorRotation();
	}
	else
	{
		FRotator ControllerRotation = OwnerCharacter->GetControlRotation();
		FVector ForwardAddRightVector = (ForwardValue * GetForwardVector(FRotator(0.f, ControllerRotation.Yaw, 0.f)))
			+ (RightValue * GetRightVector(FRotator(0.f, ControllerRotation.Yaw, 0.f)));

		FVector NormalizedVector = UKismetMathLibrary::Normal(ForwardAddRightVector);
		return MakeXRotator(NormalizedVector);
	}
}


FGameplayTag UParkourComponent::GetClimbDesireRotation()
{
	// Vertical Axis - UP : 1, Down : -1
	// Horizontal Axis - Left : -1, Right : 1
	float DesireRotation_Vertical = GetVerticalAxis();
	float DesireRotation_Horizontal = GetHorizontalAxis();

	/*
		UP : 10, Down : -10 // Left : -1, Right : 1
		이렇게 측정하여서 각 값을 더하여 8방향으로 swich문 구현
		[기존 if문에서 else if를 통한 하드 코딩식에서 개선 (ex. if(...) else if(...) * 7)]
	*/

	int32 VericalState = 0;
	int32 HorizontalState = 0;

	// Vertical
	if (DesireRotation_Vertical >= 0.5f)
		VericalState = FORWARD;
	else if (DesireRotation_Vertical <= -0.5f)
		VericalState = BACKWARD;
	else
		VericalState = NONE;

	//Horizontal
	if (DesireRotation_Horizontal >= 0.5f)
		HorizontalState = RIGHT;
	else if (DesireRotation_Horizontal <= -0.5f)
		HorizontalState = LEFT;
	else
		HorizontalState = NONE;


	switch (VericalState + HorizontalState)
	{
	case FORWARD:
		return UPSFunctionLibrary::GetCheckGameplayTag(TEXT("Parkour.Direction.Forward"));
		break;
	case BACKWARD:
		return UPSFunctionLibrary::GetCheckGameplayTag(TEXT("Parkour.Direction.Backward"));
		break;
	case LEFT:
		return UPSFunctionLibrary::GetCheckGameplayTag(TEXT("Parkour.Direction.Left"));
		break;
	case RIGHT:
		return UPSFunctionLibrary::GetCheckGameplayTag(TEXT("Parkour.Direction.Right"));
		break;
	case FORWARD_RIGHT:
		return UPSFunctionLibrary::GetCheckGameplayTag(TEXT("Parkour.Direction.ForwardRight"));
		break;
	case FORWARD_LEFT:
		return UPSFunctionLibrary::GetCheckGameplayTag(TEXT("Parkour.Direction.ForwardLeft"));
		break;
	case BACKWARD_RIGHT:
		return UPSFunctionLibrary::GetCheckGameplayTag(TEXT("Parkour.Direction.BackwardRight"));
		break;
	case BACKWARD_LEFT:
		return UPSFunctionLibrary::GetCheckGameplayTag(TEXT("Parkour.Direction.BackwardLeft"));
		break;
	default:
	case NONE:
		return UPSFunctionLibrary::GetCheckGameplayTag(TEXT("Parkour.Direction.NoDirection"));
		break;
	}

}

bool UParkourComponent::IsLedgeValid()
{
	if (WallHitTraces.Num() == 0)
	{
		ResetParkourHitResult();
		return false;
	}
	return true;
}

ECharacterState UParkourComponent::GetCharacterState()
{
	if (CharacterMovement->Velocity.Size() >= SprintTypeParkour_Speed)
		return ECharacterState::Sprint;
	else if (CharacterMovement->Velocity.Size() >= WalkingTypeParkour_Speed)
		return ECharacterState::Walking;
	else
		return ECharacterState::Idle;
}

void UParkourComponent::SetCharacterSprintSpeed(float Speed)
{
	SprintTypeParkour_Speed = Speed;
}

void UParkourComponent::SetCharacterWalkingSpeed(float Speed)
{
	WalkingTypeParkour_Speed = Speed;
}





FRotator UParkourComponent::MakeXRotator(FVector TargetVector)
{
	return UKismetMathLibrary::MakeRotFromX(TargetVector);
}

FVector UParkourComponent::GetForwardVector(FRotator Rotator)
{
	return UKismetMathLibrary::GetForwardVector(Rotator);
}

FVector UParkourComponent::GetRightVector(FRotator Rotator)
{
	return UKismetMathLibrary::GetRightVector(Rotator);
}

FVector UParkourComponent::GetUpVector(FRotator Rotator)
{
	return UKismetMathLibrary::GetUpVector(Rotator);
}

FVector UParkourComponent::GetCharacterForwardVector()
{
	if (OwnerCharacter)
		return OwnerCharacter->GetActorForwardVector();
	else
	{
		UPSFunctionLibrary::CrashLog("OwnerChracter is NULL", "OwnerChracter is NULL");
		return FVector(0.f, 0.f, 0.f);
	}
}

FVector UParkourComponent::GetCharacterRightVector()
{
	if(OwnerCharacter)
		return OwnerCharacter->GetActorRightVector();
	else
	{
		UPSFunctionLibrary::CrashLog("OwnerChracter is NULL", "OwnerChracter is NULL");
		return FVector(0.f, 0.f, 0.f);
	}
}

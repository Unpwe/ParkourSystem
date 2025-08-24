// Fill out your copyright notice in the Description page of Project Settings.


#include "ParkourComponent/ParkourComponent.h"
#include "ParkourComponent/ArrowActor.h"
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

	if(bCanParkour && CapsuleComponent->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics)
		ParkourAction();
}


void UParkourComponent::ParkourCancleCallable()
{
	if (UPSFunctionLibrary::CompGameplayTagName(ParkourActionTag, TEXT("Parkour.State.Climb")))
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


// ĳ���Ϳ��� ������ BlueprintCallable �Լ�
void UParkourComponent::MovementInputCallable(float ScaleValue, bool bFront)
{
	// Ledge ���¿� �����ϰ� �Ǹ� Climb ���¿��� ù ������ false�� �ʱ�ȭ
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



/*----------------------
	Private Function
------------------------*/
/*---------------------------------------*/

/*------------------
	Check Parkour
--------------------*/
// ������ �����ϴ� ���� �Լ� 
void UParkourComponent::ParkourAction()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("ParkourAction"));
#endif

	// No Action �����϶� ���డ��
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


// ���� ���� GameplayTag ������Ʈ
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

	// �ƹ��� ���°� �ƴҽÿ�
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
					LOG(Warning, TEXT("Jump ����"));
				#endif
				// �� ���̰� �ſ� ����. ������ �Ұ��� �Ͽ� ������ ������ ����
				if (bAutoJump)
					BP_AutoJump();
				else
					OwnerCharacter->Jump(); // ����������� ������ �ƿ� �� �� ���ٴ� ���̹Ƿ� ���� ó��
			}
			else if (CheckClimbSurface())
			{
				// Climb ���� ����
				//if (WallHeight >= CanClimbHeight)
				#ifdef DEBUG_PARKOURCOMPONENT
					LOG(Warning, TEXT("Climb ���� ����"));
				#endif
			
					CheckClimb();
			}
			else
				SetParkourAction(TEXT("Parkour.Action.NoAction"));
		}
		else if (CheckClimbSurface())
		{
			// ���� �ƴϹǷ� Climb ���� ����	
			CheckClimb();
		}
	}
	// ���� Climb ������ ��,
	else if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, "Parkour.State.Climb"))
	{
		// ���� Climb ���� �϶� Hop ���� ����
		// ���� �߰�
		CheckClimbUpOrHop();
	}
}

bool UParkourComponent::ParkourType_VaultOrMantle()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("ParkourType_VaultOrMantle"));
#endif

	// WallHeight = ĳ���� root���� ���� Top������ ����
	// WallDepth = ���� ù Top �������� �������� �Ÿ�
	// VaultHeight = ���� Top.Z���� Vault �����κ��� ���� Z������� ����

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

		// ���� �����ʰ� Vault �� �������� ���� ���� ���� ���
		if (bThinVault)
		{
			if (CheckVaultSurface())
				SetParkourAction(TEXT("Parkour.Action.ThinVault"));
			else
				SetParkourAction(TEXT("Parkour.Action.NoAction"));
		}
		else if (CharacterMovement->Velocity.Length() > Velocity_VaultMantle)
		{
			// �ӵ� üũ
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
		// WallHeight�� ���� Mantle�� ����
		bool bLowMantle = CheckLowMantleMin <= WallHeight && WallHeight <= CheckLowMantleMax;

		if (bLowMantle)
		{
			LOG(Warning, TEXT("Low Mantle"));
			if (CheckMantleSurface())
				SetParkourAction(TEXT("Parkour.Action.LowMantle"));
			else
				SetParkourAction(TEXT("Parkour.Action.NoAction"));

			return true; // Low Mantle�� ���� True ���
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
	// WallHeight = ĳ���� root���� ���� Top������ ����
	// WallDepth = ���� ù Top �������� �������� �Ÿ�
	// VaultHeight = ���� Top.Z���� Vault �����κ��� ���� Z������� ����

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



// ���� �������� Ʈ���̽��Ͽ� FHitResult�� ���� ����
void UParkourComponent::ParkourCheckWallShape()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("ParkourCheckWallShape"));
#endif

	// ĳ���� ���鿡 ���� �ִ��� �Ǵ��ϱ� ���� Trace
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
			// �ι�° Trace ����
			// ó�� ���� ��ġ�� �ľ��� FirstCheckHitResult�� ���� ���� �ι�° Trace�� ����
			WallHitTraces.Empty();

			// Climb ������ ��� FirstCheckHitResult.ImpactPoint.Z�� �ƴѰ�� OwnerCharacter�� Location.Z���� ���
			bool bClimbState = UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, FName(TEXT("Parkour.State.Climb")));
			FVector FirstHitTraceLocation = FVector(FirstCheckHitResult.ImpactPoint.X, FirstCheckHitResult.ImpactPoint.Y, (bClimbState ? FirstCheckHitResult.ImpactPoint.Z : CharacterLocation.Z));
			FVector NormalizeDeltaRightVector = GetRightVector(UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(FirstCheckHitResult.ImpactNormal));
			FVector NormalizeDeltaForwadVector = GetForwardVector(UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(FirstCheckHitResult.ImpactNormal));

			FHitResult SecondCheckHitResult;
			float PositionIndex = UPSFunctionLibrary::SelectParkourStateFloat(ParkourStateTag, -40.f, 0.f, 0.f, -20.f);
			int32 SecondCheck_LastIndex = UKismetMathLibrary::FTrunc(UPSFunctionLibrary::SelectParkourStateFloat(ParkourStateTag, 4.f, 0.f, 0.f, 2.f));
			for (int32 i = 0; i <= SecondCheck_LastIndex; i++)
			{
				// ���� ������ �յ��ϰ� �˻��ϱ� ���� i�� Ȧ�������Ѵ�.
				float StatePositionIndex = i * 20 + PositionIndex;
				FVector BasePos = FVector(0.f, 0.f, bClimbState ? 0.f : CheckParkourFromCharacterRootZ) + FirstHitTraceLocation + (NormalizeDeltaRightVector * StatePositionIndex);
				FVector StartPos = BasePos + (NormalizeDeltaForwadVector * -40.f);
				FVector EndPos = BasePos + (NormalizeDeltaForwadVector * 30.f);

				UKismetSystemLibrary::LineTraceSingle(GetWorld(), StartPos, EndPos, ParkourTraceType, false, TArray<AActor*>(), DDT_ParkourCheckWallShape, SecondCheckHitResult, true);
				HopHitTraces.Empty();

				// ����° Trace ����
				int32 ThirdCheck_LastIndex = UPSFunctionLibrary::SelectParkourStateFloat(ParkourStateTag, CheckParkourClimbHeight, 0.f, 0.f, 7.f);
				for (int32 j = 0; j < ThirdCheck_LastIndex; j++)
				{
					// ���� Second Check Hit Result�� �ڸ����� ���η� Line Trace�� �ϴ� ����
					FVector HopTraceStartPos = SecondCheckHitResult.TraceStart + FVector(0.f, 0.f, j * 8.f);
					FVector HopTraceEndPos = SecondCheckHitResult.TraceEnd + FVector(0.f, 0.f, j * 8.f);

					FHitResult HopCheckHitResult;
					UKismetSystemLibrary::LineTraceSingle(GetWorld(), HopTraceStartPos, HopTraceEndPos, ParkourTraceType, false, TArray<AActor*>(), DDT_ParkourCheckWallShape, HopCheckHitResult, true);
					HopHitTraces.Emplace(HopCheckHitResult); // ��� ����
				}

				for (int32 Index = 1; Index < HopHitTraces.Num(); Index++)
				{
					FHitResult PrevHopHitResult = HopHitTraces[Index - 1];
					FHitResult CurrentHopHitResult = HopHitTraces[Index];
					float PrevHtDistance = 0.f;
					float CurrentHtDistance = 0.f;

					// bBlocking ���¿� ���� Hit�� Distance����, Trace �� ��ü �������� ���ΰ� ������.
					if (CurrentHopHitResult.bBlockingHit)
						CurrentHtDistance = CurrentHopHitResult.Distance;
					else
						CurrentHtDistance = UKismetMathLibrary::Vector_Distance(CurrentHopHitResult.TraceStart, CurrentHopHitResult.TraceEnd);

					if (PrevHopHitResult.bBlockingHit)
						PrevHtDistance = PrevHopHitResult.Distance;
					else
						PrevHtDistance = UKismetMathLibrary::Vector_Distance(PrevHopHitResult.TraceStart, PrevHopHitResult.TraceEnd);

					/*
						Distance�� ����� ���Դٴ� ���� Prev�� ���������� �ּҰ��� �κ��̶�� �߸��̴�.
						ĳ���Ϳ��� ��ȿ�� ���� �������� ������ �߿� ������ WallHitTrace�� �����Ѵ�.

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

		// �ּ� �Ÿ��϶� ����
		if (CurrnetHitResultDistance <= PrevHitResultDistance)
			WallHitResult = WallHitTraces[i];
	}

	// ���� WallHitResult�� ���� Wall�� Top�� Depth�� ���Ѵ�
	ParkourCheckWallTopDepthShape();
}


// ParkourCheckWallShape �Լ����� ���� FHitResult ���� �̿��� Wall Top�� ���ϰ�,
// Vault�� ������ ��� Depth�� ���ϴ� �Լ�
void UParkourComponent::ParkourCheckWallTopDepthShape()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("ParkourCheckWallTopDepthShape"));
#endif

	if (WallHitResult.bBlockingHit && !WallHitResult.bStartPenetrating)
	{
		// Climb ���°� �ƴѰ�쿡�� Wall Rotation�� �ʿ��ϴ�.
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

			// ù��°�� ��� Hit�����ʾҴٸ� continue
			if (i == 0)
			{
				if (bHit)
				{
					// WallTopHitResult�� ó�� Top�� �ǹ��Ѵ�.
					WallTopHitResult = TopHitResult;
					TopHits = TopHitResult;
				}
				else
					continue;
			}
			else if (bHit)
				TopHits = TopHitResult; // �˻� �ȴٸ� ��� ����


			// ���̻� Hit �����ʾҴٸ� Vault ���ɼ��� �ִٴ� ��
			// ������ �� �ʸӿ� ������ �� �ִ� ���� �ִ��� �Ǵ��ϴ� ����
			if (!bHit)
			{
				if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.NotBusy")))
				{
					// �ּ����� �� �β� ����
					// ������ ��� �ʱ�ȭ ���� TopHits�� �̿��Ͽ� Depth ����
					FVector StartDepthPos = TopHits.ImpactPoint + (WallForwardVector * 30.f);
					FVector EndDepthPos = TopHits.ImpactPoint;
					FHitResult DepthHitResult;
					if (UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartDepthPos, EndDepthPos, 10.f, ParkourTraceType, false, TArray<AActor*>(), DDT_ParkourCheckWallTopDepthShape, DepthHitResult, true))
					{
						// Vault Result�� ���ϴ� ����.
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

	// ĳ������ ��Ʈ�� Top������ Z�� ���� ����
	if (WallTopHitResult.bBlockingHit)
	{
		if (OwnerCharacter)
		{
			float MeshRootLocation = Mesh->GetSocketLocation(FName(TEXT("root"))).Z;
			WallHeight = WallTopHitResult.ImpactPoint.Z - MeshRootLocation;
		}	
	}

	// Top�� Dpeth�� Distance.
	if (WallTopHitResult.bBlockingHit && WallDepthHitResult.bBlockingHit)
		WallDepth = UKismetMathLibrary::Vector_Distance(WallTopHitResult.ImpactPoint, WallDepthHitResult.ImpactPoint);
	else
		WallDepth = 0.f;


	// Depth���� Vault������ Z���� ���� ���� ����
	if (WallDepthHitResult.bBlockingHit && WallVaultHitResult.bBlockingHit)
		VaultHeight = WallDepthHitResult.ImpactPoint.Z - WallVaultHitResult.ImpactPoint.Z;
	else
		VaultHeight = 0.f;
}

// Tick���� �����ϴ� �Լ�
// ĳ���Ͱ� ���� ���� �� FHitResult���� Reset���ִ� ����
// bAuto Climb = true �����Ͻ� ĳ���� root �Ʒ��� Tick���� üũ�Ͽ� InGround�� �ƴϸ� PakrourAction�� �����Ѵ�. 
// (ParkourAction �ȿ��� bAutoClimb�� true �Ͻ� �ڵ� �����ϴ� �ϴ� ������ �ִ�.)
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
		// ClimbStyleBracedXYPosition, ClimbStyleFreeHangXYPosition ������ ���� ������ ��� Trace�� ���̵� ���� �ؾ��Ѵ�.
		float CustomClimbXYPostion = UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag, ClimbStyleBracedXYPosition, ClimbStyleFreeHangXYPosition);	
		FVector ClimbXYPostionVector = WallForwardVector * (CustomClimbXYPostion/2);
		CheckInGroundLocation = FVector(RootSocketLocation.X + ClimbXYPostionVector.X, RootSocketLocation.Y + ClimbXYPostionVector.Y, RootSocketLocation.Z + CheckAddZ);


		// Climb ���� Check Ground
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
			ResetParkourHitResult_Tick(); // ���� �����Ƿ� �ʱ�ȭ
		}
	}
	else if(bAlwaysParkour)
	{
		if(UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.NotBusy")))
			ParkourAction();
	}
}

// FHitResult ���� �ʱ�ȭ
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
	// #ifdef DEBUG_PARKOURCOMPONENT �����϶� Tick���� �����ϴ� ResetParkourHitResult Log�� ���� �ʱ� ����.

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

// ParkourCheckWallShape���� ���
// Climb ���¿� ������ ���踦 ������ �ִ� �Լ�. Parkour Climb ���°� �ƴ϶�� �׳� �����ص� ���� return �Ѵ�.
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
// ParkourActionTag�� ���Ӱ� Set �ϰ�, �׿� �ش��ϴ� DT�� �����ͼ� PlayMontage ���ִ� �Լ�.
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


	// ���� ��Ÿ�ָ� �����ϸ鼭 ������Ʈ ����
	// ParkourInState -> �ش� ��Ÿ���� ����
	// ParkourInOut -> ��Ÿ�ְ� ������ �ž��� ���� 
	// ex)In : Matle -> Out : NotBusy / in : ReachLedge -> Out :Climb
	SetParkourState(ParkourVariables.ParkourInState);

	bCanParkour = false; // ���� ������

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

// Parkour State�� ���� Movement Component ���� ��ȭ
// �Ǵ� Privious State�� ���� �� ParkourState ���¿� ���� Camera, Spring Arm ���� ��ȭ
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

	/* �� ���� ���� ���¸� ǥ�� �Ϸ��� ���⼭ widget �¾� */
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

/* Hand Location �Ǵ� Spring Arm Component�� ��ġ �� ���� ����*/
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
			// �� ó�� ��ġ �� ���̷� ����
			LerpTargetCameraRelativeLocation = FirstCameraLocation;
			LerpTargetArmLength = FirstTargetArmLength;
			LerpCameraTimerStart(CameraCurveTimeMax);
		}
		else if (UPSFunctionLibrary::CompGameplayTagName(NewParkourState, TEXT("Parkour.State.NotBusy")))
		{
			// �� ó�� ��ġ �� ���̷� ����
			LerpTargetCameraRelativeLocation = FirstCameraLocation;
			LerpTargetArmLength = FirstTargetArmLength;
			LerpCameraTimerStart(CameraCurveTimeMax);
		}

	}
	// Climb ���°� �� ���, ī�޶� ����
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

// LerpTargetCameraRelativeLocation, LerpTargetArmLength ������ ����� ������ Interp ����
void UParkourComponent::LerpCameraPosition()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("LerpCameraPosition"));
#endif

	if (ParkourCameraLerpCurve)
		CameraCurveAlpha = ParkourCameraLerpCurve->GetFloatValue(GetWorld()->GetTimerManager().GetTimerElapsed(CameraLerpFinishHandle));
	else
		CameraCurveAlpha = 0.5f;


	// Camera Location ����
	FVector CameraLocation = SpringArmComponent->GetRelativeLocation();
	FVector LocationDiff = LerpTargetCameraRelativeLocation - CameraLocation;
	SpringArmComponent->SetRelativeLocation(CameraLocation + (LocationDiff * CameraCurveAlpha));

	// Target Arm Length ����
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
		// InGround ���°� �ƴ϶�� �ٸ� StartPos�� ����ϴ� �ɼ�
		if (!bInGround)
			MontageStartTime = ParkourVariables.FallingMontageStartPos;
		else
			MontageStartTime = ParkourVariables.MontageStartPos;
	}
	else
		MontageStartTime = ParkourVariables.MontageStartPos;
}

// Parkour State Tag�� ����Ǹ鼭 ����Ǿ���� �ݸ��� �� �����Ʈ, ������ ���� �ݸ��� �׽�Ʈ ���� ������ �ô� �Լ�.
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
	// ���� �ƿ��� �ϸ鼭 ��Ÿ�ְ� �����鼭 �����ؾ��� Parkour State ����
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
// ��ó�� ���� Top Warp ��ġ�� ����ϴ� �Լ�
FVector UParkourComponent::FindWarpTopLocation(float WarpXOffset, float WarpZOffset)
{
	return WallTopHitResult.ImpactPoint + (GetForwardVector(WallRotation) * WarpXOffset)
		+ FVector(0.f, 0.f, WarpZOffset);
}

// ��ֹ��� ���κ� ��ġ�� �������� �� ��ġ ���
FVector UParkourComponent::FindWarpDepthLocation(float WarpXOffset, float WarpZOffset)
{
	return WallDepthHitResult.ImpactPoint + (GetForwardVector(WallRotation) * WarpXOffset)
		+ FVector(0.f, 0.f, WarpZOffset);
}

// ��ֹ� ���� ���� ��ġ�� �������� �� ���
FVector UParkourComponent::FindWarpVaultLocation(float WarpXOffset, float WarpZOffset)
{

	return WallVaultHitResult.ImpactPoint + (GetForwardVector(WallRotation) * WarpXOffset)
		+ FVector(0.f, 0.f, WarpZOffset);
}


// Mantle�� �������� �켱 ��ֹ��� WarpXOffset ��ġ��ŭ ���� �˻��Ͽ� ĳ���Ͱ� ���� ����� �ִ��� �Ǵ�
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

// Top Location�� �ʿ��ѵ� �ٸ� ������ �ʿ��� ��츦 ���� �ӽ� �Լ�
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

	// Braced������� FreeHang��������� üũ
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

	// ���� ClimbStyle�� ������� ret
	if (UPSFunctionLibrary::CompGameplayTagName(ClimbStyleTag, ClimbStyleName))
		return;

	ClimbStyleTag = UPSFunctionLibrary::GetGameplayTag(ClimbStyleName);
	AnimInstance->SetClimbStyle(ClimbStyleTag);
}

// ClimbMovement�� ���� �� �𼭸��� ����� ������ �˻�
void UParkourComponent::ClimbLedgeResult()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("ClimbLedgeResult"));
#endif

	// ĳ���Ϳ��� �Ÿ��� ���� ª�� ���� ������ ���� Wall Hit Result�� �̿���.
	// ĳ���� �ٷ� �� ��ֹ��� ForwardVector�� �̿��ϱ� ����.
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
	ClimbLedgeHitResult.Location = ClimbLedgeResult_Second.Location; // ��ü
	ClimbLedgeHitResult.ImpactPoint.Z = ClimbLedgeResult_Second.ImpactPoint.Z; // ��ü
	
}


/*---------------------------
		Check Surface
----------------------------*/
// Mantle �� ���ִ� ǥ������ Check
// �� ���� ���� ����ִ� ���»Ӹ� �ƴ�, Climb ���¿��� �ö󰥶��� Mantle�� ����ȴ�.
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

// Climb �� ���ִ� ǥ������ Check
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

// Vault �� �� ������ ������ �ִ��� Check
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
		CheckVaultCapsuleTraceHalfHeight, // Vault�� ������ �� ĳ���Ͱ� Ȥ�� ���� ��ֹ��� ����� �˻��ϱ� ����
		ParkourTraceType, false, TArray<AActor*>(), DDT_CheckVaultSurface, VaultHitResult, true);

	return !bHit;
}

bool UParkourComponent::CheckClimbMovementSurface(FHitResult MovementHitResult)
{
	FRotator ArrowActorWorldRotation = ArrowActor->GetArrowWorldRotation();
	FVector ArrowForwardVector = GetForwardVector(ArrowActorWorldRotation);
	FVector ArrowRightVector = GetRightVector(ArrowActorWorldRotation);

	// �Է¹��� üũ
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

	// ClimbLedgeHitResult : ã�Ƴ� ������Ʈ�� �������� �ٽ� ��˻��Ͽ� ���� �� �ִ� ��ġ�� ����.
	FHitResult LedgeResult = ClimbLedgeHitResult;
	FVector WallForwardVector = GetForwardVector(WallRotation);
	FVector WallRightVector = GetRightVector(WallRotation);

	if(LedgeResult.bBlockingHit)
	{
		/* ���������� AnimInstance->SetLeftHandLedgeLocation / Rotation�� �ϴ� ��ġ */
		FVector TargetLeftHandLedgeLocation;
		FRotator TagetLeftHandLedgeRotation;

		for (int32 i = 0; i <= 4; i++)
		{
			/* WallRotation�� LedgeResult.ImpactPoint + Forward Vector ������ �������� 
			Left�� LeftIndex * -2��ŭ, -CheckClimbWidth ��ŭ Trace*/ 
			int32 LeftIndex = i * -2;

			FVector WallRotation_Left = WallRightVector * (ClimbIKHandSpace - LeftIndex);  // Left�� ����, Right�� ���
			FVector StartPos = LedgeResult.ImpactPoint + (WallForwardVector * -CheckClimbForward) + WallRotation_Left;
			FVector EndPos = LedgeResult.ImpactPoint + (WallForwardVector * CheckClimbForward) + WallRotation_Left;
			
			FHitResult LeftClimbHitResult;
			UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartPos, EndPos, 5.f, ParkourTraceType, false, TArray<AActor*>(), DDT_LeftClimbHandIK, LeftClimbHitResult, true);

			// True�� ��� : IK�� ������.
			if (LeftClimbHitResult.bBlockingHit)
			{
				FRotator LeftClimbHitResultNormalRotator = UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(LeftClimbHitResult.ImpactNormal);
				FVector LeftClimbHitResultForwardVector = GetForwardVector(LeftClimbHitResultNormalRotator);
			
				FHitResult LeftClimbHitResult_ZResult;
				// ImpactNormal�� Rotation������ ForwardVector * 2 + ImpactPoint�� ��ġ���� j����ŭ j * 5�� Z����ġ�� �˻��Ѵ�.
				for (int32 j = 0; j <= 5; j++)
				{
					// ���� �� �ִ� ��ġ�������� (ImpactNormal_ForwardVector * 2.f) + FVector(0.f, 0.f, j * 5) ���ݸ�ŭ ���� �Ʒ������� Trace				
					FVector ImpactNormal_StartPos = LeftClimbHitResult.ImpactPoint + (LeftClimbHitResultForwardVector * 2.f) + FVector(0.f, 0.f, j * 5);
					FVector ImpactNormal_EndPos = ImpactNormal_StartPos - FVector(0.f, 0.f, (j * 5) + 50.f);
					UKismetSystemLibrary::SphereTraceSingle(GetWorld(), ImpactNormal_StartPos, ImpactNormal_EndPos, 2.5f, ParkourTraceType, false, TArray<AActor*>(), DDT_LeftClimbHandIK, LeftClimbHitResult_ZResult, true);
				

					// True�Ͻ� : bStartPenetrating�Ͻ� IK�� �ؾ��ϴ� ��ġ �� ȸ���� ���� 
					// �߿��� ���� LeftClimbHitResult_ZResult�� �ƴ� ���� LeftClimbHitResult.ImpactPoint ���� �̿��Ѵٴ� ���̴�.
					// bStartPenetrating�� �Ǵ��ϴ� ������ bBlockingHit�� �޸� bStartPenetratingrk true��� ���� Blocking�� �ƴ� �������� ħ���� �Ǿ��ٴ� ���̱⶧����
					// ���� �˻��ϴ� ��ġ���� ���� �� ���ų� �ϴ� ���� ������ �� ���κ��� �˻� �ؾ��Ѵٴ� ���� �ľ��� �� �ֱ� �����̴�.
					if (LeftClimbHitResult_ZResult.bStartPenetrating)
					{
						// ������ ���� �����Ͽ� 
						if (j == 5)
						{
							TargetLeftHandLedgeLocation =
								FVector(LeftClimbHitResult.ImpactPoint.X, LeftClimbHitResult.ImpactPoint.Y, LeftClimbHitResult.ImpactPoint.Z - 9.f);

							TagetLeftHandLedgeRotation = FRotator(LeftClimbHitResultNormalRotator + Additive_LeftClimbIKHandRotation); // �ִϸ��̼Ǹ��� ȸ���� �ٸ�

							LastLeftHandIKTargetLocation = TargetLeftHandLedgeLocation;

							AnimInstance->SetLeftHandLedgeLocation(TargetLeftHandLedgeLocation);
							AnimInstance->SetLeftHandLedgeRotation(TagetLeftHandLedgeRotation);
						}
					}
					else if (LeftClimbHitResult_ZResult.bBlockingHit)
					{
						// CharacterHandThickness�� ���� ������ ���� �� ���缭 IK��ġ�� ���Ǿ��� ������ �� �β��� �����ؼ� ���� �� �ڷ� ������ ��
						float HandFrontDiff = UKismetMathLibrary::SelectFloat(BracedHandFrontDiff, 0.f, UPSFunctionLibrary::CompGameplayTagName(ClimbStyleTag, TEXT("Parkour.ClimbStyle.Braced")))
							+ -CharacterHandThickness;

						FVector AddHandFrontLocation = LeftClimbHitResult.ImpactPoint + (LeftClimbHitResultForwardVector * HandFrontDiff);

						// Z�࿡�� CharacterHeightDiff, CharacterHandUpDiff ���� ���ؼ� �������� IK Location ���ϱ�
						TargetLeftHandLedgeLocation =
							FVector(AddHandFrontLocation.X, AddHandFrontLocation.Y,
								(LeftClimbHitResult.ImpactPoint.Z + CharacterHeightDiff + CharacterHandUpDiff - 9.f));

						TagetLeftHandLedgeRotation = FRotator(LeftClimbHitResultNormalRotator + Additive_LeftClimbIKHandRotation);


						LastLeftHandIKTargetLocation = TargetLeftHandLedgeLocation;

						AnimInstance->SetLeftHandLedgeLocation(TargetLeftHandLedgeLocation);
						AnimInstance->SetLeftHandLedgeRotation(TagetLeftHandLedgeRotation);

						break;						
					}					
				}

				break; // �ʼ�
			}
		}	
	}	
}

void UParkourComponent::MontageLeftFootIK()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("MontageLeftFootIK"));
#endif

	// Foot IK�� Climb ���°� Braced�� ���� ��ȿ�ϴ�.
	if (UPSFunctionLibrary::CompGameplayTagName(ClimbStyleTag, TEXT("Parkour.ClimbStyle.Braced")))
	{
		FVector WallForwardVector = GetForwardVector(WallRotation);
		FVector WallRightVector = GetRightVector(WallRotation);

		// ����Z�࿡ FootIK�ϴ� ��ġ ���ֱ�
		FVector FootIKLocation = FVector(0.f, 0.f, CharacterHeightDiff - CharacterFootIKLocation);

		FHitResult LedgeResult = ClimbLedgeHitResult;
		for (int32 cnt = 0; cnt <= 2; cnt++)
		{					
			// Z������ ���ذ���, Left Foot�� ��ġ�� ���⿡�� Z������ ���������� Trace ���� (�ʿ��� ����� ������ �ݺ� Trace�� �ʿ�x)
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
				//ã�Ƴ� Foot IK Location���� ����ڰ� ���� �� ���ִ� BracedFootAddForward_Left ������ ������ ���Ͽ� �ִ� �ν��Ͻ��� ���� ��ġ ����
				FVector ClimbFootHitResult_ForwardVector = GetForwardVector(UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(ClimbFootHitResult.ImpactNormal));
				FVector LeftFootLocation = ClimbFootHitResult.ImpactPoint + (ClimbFootHitResult_ForwardVector * -BracedFootAddThickness_Left);
				
				AnimInstance->SetLeftFootLocation(LeftFootLocation);
				break;
			}
			else if (cnt == 2) // false : ã�� ����
			{
				// �� ���� ������ �˻�
				for (int cnt2 = 0; cnt2 <= 4; cnt2++)
				{
					// for�� �ε��� ���� �̿��Ͽ� Z���� ���ذ��� �˻�
					FVector SecondBasePos = LedgeResult.ImpactPoint + FootIKLocation + FVector(0.f, 0.f,  (cnt2 * 5));
					FVector SecondStartPos = SecondBasePos + (WallForwardVector * -30.f);
					FVector SecondEndPos = SecondBasePos + (WallForwardVector * 30.f);

					FHitResult SecondClimbFootHitResult;
					bool bSecondHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), SecondStartPos, SecondEndPos, 15.f, ParkourTraceType, false, TArray<AActor*>(), DDT_LeftClimbFootIK, SecondClimbFootHitResult, true);

					if (bSecondHit)
					{
						FVector SecondClimbFootHitResult_ForwardVector = GetForwardVector(UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(SecondClimbFootHitResult.ImpactNormal));
						FVector LeftFootLocation = SecondClimbFootHitResult.ImpactPoint + (SecondClimbFootHitResult_ForwardVector * -BracedFootAddThickness_Left);

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
		/* ���������� AnimInstance->SetLeftHandLedgeLocation / Rotation�� �ϴ� ��ġ */
		FVector TargetRightHandLedgeLocation;
		FRotator TagetRightHandLedgeRotation;

		for (int32 i = 0; i <= 4; i++)
		{
			int32 RightIndex = i * 2;
		
			FVector WallRotation_Right = WallRightVector * (-ClimbIKHandSpace - RightIndex);
			FVector StartPos = LedgeResult.ImpactPoint + (WallForwardVector * -CheckClimbForward) + WallRotation_Right;
			FVector EndPos = LedgeResult.ImpactPoint + (WallForwardVector * CheckClimbForward) + WallRotation_Right;
			
			FHitResult RightClimbHitResult;
			UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartPos, EndPos, 5.f, ParkourTraceType, false, TArray<AActor*>(), DDT_RightClimbHandIK, RightClimbHitResult, true);

			if (RightClimbHitResult.bBlockingHit)
			{
				FRotator RightClimbHitResultNormalRotator = UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(RightClimbHitResult.ImpactNormal);
				FVector RightClimbHitResultForwardVector = GetForwardVector(RightClimbHitResultNormalRotator);
				
				FHitResult RightClimbHitResult_ZResult;
				// ImpactNormal�� Rotation������ ForwardVector * 2 + ImpactPoint�� ��ġ���� j����ŭ j * 5�� Z����ġ�� �˻��Ѵ�.
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

							TagetRightHandLedgeRotation = FRotator(RightClimbHitResultNormalRotator + Additive_RightClimbIKHandRotation); // �ִϸ��̼Ǹ��� ȸ������ �ٸ���.

							LastRightHandIKTargetLocation = TargetRightHandLedgeLocation;

							AnimInstance->SetRightHandLedgeLocation(TargetRightHandLedgeLocation);
							AnimInstance->SetRightHandLedgeRotation(TagetRightHandLedgeRotation);
						}
					}
					else if (RightClimbHitResult_ZResult.bBlockingHit)
					{
						// CharacterHandThickness�� ���� ������ ���� �� ���缭 IK��ġ�� ���Ǿ��� ������ �� �β��� �����ؼ� ���� �� �ڷ� ������ ��
						float HandFrontDiff = UKismetMathLibrary::SelectFloat(BracedHandFrontDiff, 0.f, UPSFunctionLibrary::CompGameplayTagName(ClimbStyleTag, TEXT("Parkour.ClimbStyle.Braced")))
							+ -CharacterHandThickness;
						FVector AddHandFrontLocation = RightClimbHitResult.ImpactPoint + (RightClimbHitResultForwardVector * HandFrontDiff);

						// Z�࿡�� CharacterHeightDiff, CharacterHandUpDiff ���� �� �ؼ� IK ����
						TargetRightHandLedgeLocation =
							FVector(AddHandFrontLocation.X, AddHandFrontLocation.Y, (RightClimbHitResult.ImpactPoint.Z + CharacterHeightDiff + CharacterHandUpDiff - 9.f));

						TagetRightHandLedgeRotation = FRotator(RightClimbHitResultNormalRotator + Additive_RightClimbIKHandRotation);

						LOG(Error, TEXT("TargetRightHandLedgeLocation %f, %f, %f"), TargetRightHandLedgeLocation.X, TargetRightHandLedgeLocation.Y, TargetRightHandLedgeLocation.Z);

						LastRightHandIKTargetLocation = TargetRightHandLedgeLocation;

						AnimInstance->SetRightHandLedgeLocation(TargetRightHandLedgeLocation);
						AnimInstance->SetRightHandLedgeRotation(TagetRightHandLedgeRotation);
						break;
					}
				}

				break; // �ʼ�
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

		// ����Z�࿡ FootIK�ϴ� ��ġ ���ֱ�
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
						// for�� �ε��� ���� �̿��Ͽ� Z���� ���ذ��� �˻�
						FVector SecondBasePos = LedgeResult.ImpactPoint + FootIKLocation + FVector(0.f, 0.f, (cnt2 * 5));
						FVector SecondStartPos = SecondBasePos + (WallForwardVector * -30.f);
						FVector SecondEndPos = SecondBasePos + (WallForwardVector * 30.f);

						FHitResult SecondClimbFootHitResult;
						bool bSecondHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), SecondStartPos, SecondEndPos, 15.f, ParkourTraceType, false, TArray<AActor*>(), DDT_RightClimbFootIK, SecondClimbFootHitResult, true);

						if (bSecondHit)
						{
							FVector SecondClimbFootHitResult_ForwardVector = GetForwardVector(UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(SecondClimbFootHitResult.ImpactNormal));
							FVector RightFootLocation = SecondClimbFootHitResult.ImpactPoint + (SecondClimbFootHitResult_ForwardVector * -BracedFootAddThickness_Right);
							
							AnimInstance->SetRightFootLocation(RightFootLocation);
							break;
						}
					}
				}
			}
		}
	}
}


/*----------------------
	Climb Movemnet IK
-----------------------*/
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

	// ClimbStyleBracedXYPosition, ClimbStyleFreeHangXYPosition ������ ���� ������ ��� Trace�� ���̵� ���� �ؾ��Ѵ�.
	float CustomClimbXYPostion = UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag, ClimbStyleBracedXYPosition, ClimbStyleFreeHangXYPosition);

	for (int32 i = 0; i <= 4; i++)
	{
		int32 IndexCnt = i * 2;
		
		FVector LeftIKHandBasePos =
			FVector(LeftIKHandSocketLoc.X, LeftIKHandSocketLoc.Y, LeftIKHandSocketLoc.Z - CharacterHeightDiff)
			+ (CharacterRightVec * (-ClimbIKHandSpace + IndexCnt - SameDirection));

		int32 IndexHeightCnt = i * 10;
		LeftIKHandBasePos -= FVector(0.f, 0.f, IndexHeightCnt); // Z�� ���� ����


		FVector LeftIKHandStartPos = LeftIKHandBasePos + (CharacterForwardVec * (-10 + CustomClimbXYPostion));
		FVector LeftIKHandEndPos = LeftIKHandBasePos + (CharacterForwardVec * (30 + -CustomClimbXYPostion));

		// �� üũ
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
			
				/* Wall Top ��� */
				FHitResult LeftHandWallTopHitResult;
				UKismetSystemLibrary::SphereTraceSingle(GetWorld(),
					TraceStartLoc, TraceEndLoc, 2.5f,
					ParkourTraceType, false, TArray<AActor*>(), DDT_ParkourHandIK, LeftHandWallTopHitResult, true);

				if (!LeftHandWallTopHitResult.bStartPenetrating && LeftHandWallTopHitResult.bBlockingHit)
				{
					float ClimbStyleHandHeight = UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag, BracedHandMovementHeight, FreeHangHandMovementHeight);

					/* Set Left Hand Ledge Location */
					// ���� Top ��ġ�� ����ڰ� ���������� �ۼ��� �������� ���� �������� ��ġ�� ������
					float LeftHandLedgeLocactionZ =
						LeftHandWallTopHitResult.ImpactPoint.Z + ClimbStyleHandHeight
						+ CharacterHeightDiff + CharacterHandUpDiff
						+ GetLeftHandIKModifierGetZ();

					

					// X�� ���⸸ ���ΰ� ���� ȸ���� ����
					// ImpactNormal Vector�� X���� ȸ�� ������ �޾ƿ´�.
					FRotator LeftHandWallMakeRotX = MakeXRotator(Ht_LeftHandWall.ImpactNormal);

					FVector LeftHandWallForwardVec = GetForwardVector(LeftHandWallMakeRotX);
					FVector CharacterHandFrontDiffV = Ht_LeftHandWall.ImpactPoint +
						(LeftHandWallForwardVec * (-1.f * UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag, BracedHandFrontDiff, FreeHangHandFrontDiff)));


					FVector LeftHandLedgeTargetLoc = FVector(CharacterHandFrontDiffV.X, CharacterHandFrontDiffV.Y, LeftHandLedgeLocactionZ);

					AnimInstance->SetLeftHandLedgeLocation(LeftHandLedgeTargetLoc);


					/* Set Left Hand Ledge Rotation */
					// ReversDeltaRotator�� �ؾ��ϴ� ������ ImpactNomal���� ȸ������ �ݴ�� ������ ���� ������ �±� ����
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

	// ClimbStyleBracedXYPosition, ClimbStyleFreeHangXYPosition ������ ���� ������ ��� Trace�� ���̵� ���� �ؾ��Ѵ�.
	float CustomClimbXYPostion = UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag, ClimbStyleBracedXYPosition, ClimbStyleFreeHangXYPosition);

	for (int32 i = 0; i <= 4; i++)
	{
		int32 IndexCnt = i * 2;

		FVector RightIKHandBasePos =
			FVector(RightIKHandSocketLoc.X, RightIKHandSocketLoc.Y, RightIKHandSocketLoc.Z - CharacterHeightDiff)
			+ (CharacterRightVec * (-1 * (-ClimbIKHandSpace + IndexCnt - SameDirection))); // Right�� ����

		
		int32 IndexHeightCnt = i * 10;
		RightIKHandBasePos -= FVector(0.f, 0.f, IndexHeightCnt); // Z�� ���� ����
		
		FVector RightIKHandStartPos = RightIKHandBasePos + (CharacterForwardVec * (-10 + CustomClimbXYPostion));
		FVector RightIKHandEndPos = RightIKHandBasePos + (CharacterForwardVec * (30 + -CustomClimbXYPostion));

		// �� üũ
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


					// X�� ���⸸ ���ΰ� ���� ȸ���� ����
					// ImpactNormal Vector�� X���� ȸ�� ������ �޾ƿ´�.
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
					 Left IK Foot (X,Y) ��ġ + (Left Hand - FootPositionToHand) * indexCnt2 (Z) ��ġ
					i_indexCnt2_l�� Z�࿡ �����ִ� ������ ù �˻��ϴ� �κ��� ����ִ� ���
					Z�� �������� �˻��ذ��� �� ������� ã�� ���� �����̴�.
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
					// ������ Offset ��ŭ ���� ��ġ
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
					 Right IK Foot (X,Y) ��ġ + (Right Hand - RightFootPositionToHand) * indexCnt2 (Z) ��ġ
					i_indexCnt2_l�� Z�࿡ �����ִ� ������ ù �˻��ϴ� �κ��� ����ִ� ���
					Z�� �������� �˻��ذ��� �� ������� ã�� ���� �����̴�.
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
					// ������ Offset ��ŭ ���� ��ġ
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
	// �������� ������ ���
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
	// �������� ������ ���
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

	// ClimbMoveCheckDistance ������ �Էµ� ����ŭ �ش� ��ġ�� �˻��Ͽ� ClimbMove�� �����ϸ� �����̴� ����
	// �ڳ� �Ӹ��ƴ϶� ��¦ ����ǰų� �ձ� ����� �𼭸��κ��� ClimbMove ����ϱ� ����
	// ������ ���� �Ʒ��� CapsuleTrace�� �˻縦 �Ѵ�.

	bool bCheckFirstForBreak = false;
	bool bCheckSecondForBreak = false;
	bool bCheckThirdForBreak = false;


	// ���� ã�� ����
	for (int32 FirstIndex = 0; FirstIndex <= 2 && !bCheckFirstForBreak; FirstIndex++)
	{
		int32 LocalIndex = FirstIndex * -15;
		FVector ClimbMoveCheckStartPos = FVector(0.f, 0.f, LocalIndex) + ArrowWorldLocation + RightVectorClimbDistance + (ForwardVector * -10.f);
		FVector ClimbMoveCheckEndPos = ClimbMoveCheckStartPos + (ForwardVector * 60.f);

		FHitResult ClimbMovementHitResult;
		UKismetSystemLibrary::SphereTraceSingle(GetWorld(), ClimbMoveCheckStartPos, ClimbMoveCheckEndPos, 5.f, ParkourTraceType, false,
			TArray<AActor*>(), DDT_ClimbMovementCheck, ClimbMovementHitResult, true);

		// bStartPenetrating�� true��� ���� Hit�ϱ⵵ ���� ���Ĺ��ȴٴ� ��
		// �����̿��� ��¦ �𳭺κ��� �ڳʸ� ���⿡�� �ָ��ϰ� �ٷ� ��⵵ �ָ��� ��ġ�� ��츦 ����
		// Top�� ���ϰų� Corner �Լ��� �����Ѵ�.

		if (!ClimbMovementHitResult.bStartPenetrating)
		{
			if (CheckOutCorner()) // ĳ���� �ٷ� ���� �ڳʵ��� �ִ� �� üũ
				OutCornerMovement(); 
			else // �ڳʰ� �ƴѰ�� or �ڳʱ� ������ ĳ���� �ٷ� ���� ������ ���� ���� ���
			{				
				// bStartPenetrating�� �ƿ� �˻簡 �����ʾ������� false�� ����ϱ⶧���� bBlockingHit�� �˻�
				if (ClimbMovementHitResult.bBlockingHit) // �ڡ� Corner�� �ƴϰ� ClimbMove�� ������
				{
					LastLeftHandIKTargetLocation = ClimbMovementHitResult.ImpactPoint;
					LastRightHandIKTargetLocation = ClimbMovementHitResult.ImpactPoint;

					// ClimbMove�� ������ �𼭸��ΰ�� Trace�� ��ġ�� FVector(0.f, 0.f, SecondIndex * 5 + float) ��ŭ �ش� Top�� üũ
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
							// ClimbMoveCheck, ClimbMoveTopCheck ���� for�� ���������� �˻縦 ������ ���
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
							else // Climb�� �ϸ� �̵��Ҷ� ��¦ ���̰� ���� �κ��� �ִ� ��� LineTrace�� �˻��Ͽ� �ش� ��ġ Check�ϴ� for��
							{
								bCheckFirstForBreak = true; // break;

								// ���� üũ ��
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
											return; // ���� ���� �ִٴ� ���̱� ������ �� �˻��� �ʿ䰡 ����.
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
				else // ClimbMovementHitResult.bBlockingHit�� false�� ���. ĳ���͸� �����ʴ� �ڳʸ� ���� ���
				{
					if (FirstIndex == 2)
					{
						if (!UPSFunctionLibrary::CompGameplayTagName(ParkourActionTag, TEXT("Parkour.Action.CornerMove")))
							InCornerMovement(); // Corner Move ���°� �ƴѰ��
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
	// Climb���¿��� ������ �� ��Ʈ�ѷ��� ����� ���⿡���� ������ ������
	if (bUseControllerRotation_Climb)
	{
		FRotator ControlRotation = OwnerCharacter->GetControlRotation();
		FRotator CharacterRotation = OwnerCharacter->GetActorRotation();

		float DeltaYaw = UKismetMathLibrary::NormalizedDeltaRotator(ControlRotation, CharacterRotation).Yaw;

		/* 
		Cos0���� ���� 1�̴�.��, 0���� ���� �������� �����ٴ� ���̹Ƿ�
		����, �Ĺ��� ��Ÿ���� Vertical���� Cos��, �� �ݴ��� Horizontal�� Sin�� ���Ѵ�. 
		Cos�� Sin�� �ݺ�� �ϱ⶧��.
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
	// Climb���¿��� ������ �� ��Ʈ�ѷ��� ����� ���⿡���� ������ ������
	if (bUseControllerRotation_Climb)
	{
		FRotator ControlRotation = OwnerCharacter->GetControlRotation();
		FRotator CharacterRotation = OwnerCharacter->GetActorRotation();

		float DeltaYaw = 0.f - UKismetMathLibrary::NormalizedDeltaRotator(ControlRotation, CharacterRotation).Yaw;

		/*
		Cos0���� ���� 1�̴�.��, 0���� ���� �������� �����ٴ� ���̹Ƿ�
		����, �������� ��Ÿ���� Horizontal���� Cos��, �� �ݴ��� Vertical�� Sin�� ���Ѵ�.
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
		float BracedSpeed = FMath::Clamp(OutputValue, 1.f, ClimbSpeedMaxValue) * ClimbSpeedBraced;
		float FreeHangSpeed = FMath::Clamp(OutputValue, 1.f, ClimbSpeedMaxValue) * ClimbSpeedFreeHang;

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

// ĳ������ Head Socket ��ġ�� �̿��Ͽ� ClimbLedgeResult() �Լ����� ���� HitResult.ImpactPoint.Z������
// Height�� ���� ���� ĳ���Ͱ� ���� �ִ��� ���߿��ִ��� �Ǵ�
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
		// ȸ������ -5.f ��ŭ �Ʒ����� ForwardVector ���������� SphereTrace
		// ĳ���Ͱ� Climb�� �õ� �Ҽ��ִ� ����� ������ �ִ��� Ȯ�� 
		// ������ �˻��� Trace�� ĳ������ �ٷ� �Ʒ� ���� ������, �ش� ������ ���� �𼭸��� ã�� ����
		FVector ForwardVector = GetForwardVector(GetDesireRotation());
		FVector StartPos_Second = (FindDropDownHitResult.ImpactPoint + FVector(0.f, 0.f, -5.f)) + (ForwardVector * 100.f);
		FVector EndPos_Second = StartPos_Second + (ForwardVector * -125.f);

		FHitResult FindDropDownHitResult_Second;
		UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartPos_Second, EndPos_Second, 5.f, ParkourTraceType, false, TArray<AActor*>(), DDT_FindDropDownHangLocation, FindDropDownHitResult_Second, true);

		// ���������� Climb�� ������ �� ���� ã�� ����
		if (FindDropDownHitResult_Second.bBlockingHit && !FindDropDownHitResult_Second.bStartPenetrating)
		{
			// Climb�� ������ ���� Ž��
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

				// �ش� ���ο��� Z�� �� ������ Line Trace ����
				for (int32 HopCnt = 0; HopCnt <= 12; HopCnt++)
				{
					FVector StartPos_HopTrace = FVector(0.f, 0.f, (HopCnt * 8)) + FindDropDownHitResult_Third.TraceStart;
					FVector EndPos_HopTrace = FVector(0.f, 0.f, (HopCnt * 8)) + FindDropDownHitResult_Third.TraceEnd;

					FHitResult FindDropDownHitResult_Hop;
					UKismetSystemLibrary::LineTraceSingle(GetWorld(), StartPos_HopTrace, EndPos_HopTrace, ParkourTraceType, false, TArray<AActor*>(), DDT_FindDropDownHangLocation, FindDropDownHitResult_Hop, true);

					HopHitTraces.Emplace(FindDropDownHitResult_Hop);
				}

				// �� �ݺ������� ã�Ƴ� ������ Distance�� �Ǻ��Ͽ� �ϳ��� WallHitTraces�� ����
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

					// WallHitTraces[i]�� Distance�� �� ������� �ش� ������ WallHitResult ����
					// �ƴѰ�쿡�� WallHitResult �״�� ��� (��������)
					if (CurrentDistance <= PrevDistance)
						WallHitResult = WallHitTraces[i];
					
				}
			}

			// ���������� �ش���ġ�� Climb ������ �ϱ����� Trace�Ͽ� Top�� �˻��ѵ� Climb �Լ� ����
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
		LocalOutCornerIndex = i;
		float CheckHorizontalWidth = HoriozontalAxis * 35.f;
		FVector StartPos = FVector(0.f, 0.f, LocalOutCornerIndex * -10) + ArrowWorldLocation 
			+ (CheckHorizontalWidth * ArrowRightVector);
		FVector EndPos = StartPos + (ArrowForwardVector * 100.f);

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

	return bOutCornerReturn; // true -> ClimbMovement �Լ����� OutCornerMovement �Լ� ����
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
				������ Corner�� Ž���� ImpactPoint���� �ٷ� Z�ุ ���ϰ� ���� �������� �˻��Ѵ�.
				������ �˻��ϴ� Trace�� ù StartPos�� ������ �������� ������ ���δ� bStartPenetrating�� True ó���� �ȴ�.
				�׷��� �ش� ���� ������ ������ �ƴ����� bStartPenetrating ���� �ϳ��� �Ǵ��� �����ϴ�. (���߿�)
			*/
			if (OutCornerCheckTopHitResult.bStartPenetrating) // Trace�� �浹�� ���·� ���۵Ǵ� ���
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

					FVector BaseXYVector = CornerForwardVector * UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag, ClimbStyleBracedXYPosition, ClimbStyleFreeHangXYPosition)
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
		int32 CheckHeight = 0; // ArrowActor�� ������ �����ִ� ��츦 ����
		int32 HorizontalWidth = 0; //  Horizontal �������� Check�� Cnt
		for (int32 i = 0; i <= 5 && !bBreak; i++)
		{
			/*
				���°���� �˻�����ʴ����� Check�ϴ� �����̴�.
				Check �������� ������ �˻��Ѵ�. �������� �� �ڳʶ�� �Ǵ��ϱ� �����̴�.
			*/
			
			
			FVector InCornerCheckFirstBasePos = FVector(0.f, 0.f, CheckHeight) + ArrowActorWorldLocation + (ArrowRightVector * (HorizontalAxis * HorizontalWidth));
			FVector InCornerCheckFirstStartPos = InCornerCheckFirstBasePos + (ArrowForwardVector * -10.f);
			FVector InCornerCheckFirstEndPos = InCornerCheckFirstBasePos + (ArrowForwardVector * 80.f);

			FHitResult InCornerCheckFirstHitResult;
			bool bHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), InCornerCheckFirstStartPos, InCornerCheckFirstEndPos, 10.f, ParkourTraceType, false, TArray<AActor*>(), DDT_InCornerMovement, InCornerCheckFirstHitResult, true);

			if (bHit)
			{
				// bCheckFirstHit�� ���� ����ϴ� ������ ������ ������ ArrowActor�� ������ ���� �ִ� ��츦 ���� ex)ClimbStyleBracedZPosition�� ���� ��
				// �׷���� �Ʒ������� �������� �˻��ؾ��Ѵ�.
				if (!bFirstCheckHit) 
					bFirstCheckHit = true;

				if (i == 5)
				{
					LocalCornerDepthHitResult = FHitResult(); // In Corner �Ұ�
					StopClimbMovement();
				}					
				else
					LocalCornerDepthHitResult = InCornerCheckFirstHitResult;

				HorizontalWidth = i * 10;
					
			}
			else // �˻� ���� �����Ƿ� InCorner �Լ��� ����
			{
				if (bFirstCheckHit) // �ѹ��̶� Hit�� ���
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
					CheckHeight += -10; // �ƿ� üũ���� �ʾ����Ƿ� �Ʒ���
					i = 0; // for�� ó������ �ٽ�
				}
			}
		}
	}
}

void UParkourComponent::CheckInCornerSide(const FHitResult& CornerDepthHitResult, FVector ArrowActorWorldLocation, FRotator ArrowActorWorldRotation, FVector ArrowForwardVector, FVector ArrowRightVector, float HorizontalAxis)
{
	/*
		InCornerMovement�Լ����� ���� �� ���� ������ �˾Ҵٸ� �� ��ġ�� �������� �˻��Ͽ� Side�� üũ�ϴ� �����̴�.
	*/

	bool bBreak = false;
	for (int32 CheckSideCnt = 0; CheckSideCnt <= 2 && !bBreak; CheckSideCnt++)
	{
		// ���� �ö󰡸鼭 üũ
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
		�̵��� In Corner �𼭸��� Top�� üũ�Ѵ�
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
					���������� ĳ���� CapsuleComponent�� �̵������� ����� ������ �ִ��� Check�ϴ� ����
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

				// true : CapsuleComponent�� ������ ������ ��� Stop
				if (bCapsuleHit)
					StopClimbMovement();
				else
				{
					WallRotation = NormalizeRotator;

					FVector CornerMove_XY = InCornerSideCheckHitResult.ImpactPoint
						+ ForwardNormalVector * UPSFunctionLibrary::SelectClimbStyleFloat(ClimbStyleTag, ClimbStyleBracedXYPosition, ClimbStyleFreeHangXYPosition);

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
	LatenActionInfo.CallbackTarget = this; // ��
	LatenActionInfo.ExecutionFunction = FName(TEXT("CornerMoveCompleted")); // �Ϸ� �� ȣ��� �Լ� �̸�
	LatenActionInfo.Linkage = 0; // �� ���� ��κ� 0���� �����մϴ�.
	LatenActionInfo.UUID = GetNextLatentActionUUID(); // ������ ID �Ҵ�

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
		// �������� ����Ű�� �����Ͽ� ParkourŰ�� ���� ������ ��, Climb ���¿��� �ö󰥼��ִ� ������ �Ǵ�.
		// �ƴѰ�쿡 Hop�� ���ִ� ���� ã�Ƽ� �����ҽ� Hop ����
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
	
	FindHopLocation(); // Hop�� ��ġ ã�� (WallHitResult �� WallTopHitResult ����)

	if (IsLedgeValid())
	{
		LOG(Error, TEXT("Is Valid"));
		CheckClimbStyle();
		ClimbLedgeResult();
		SetParkourAction(SelectHopAction());
	}
}


// WallTopResult�� WallHitResult�� ClimbMovement �߿��� Empty �Ǳ⶧���� Hop�� �����ϸ� Top ��ġ�� �ٽ� ������־���Ѵ�.
void UParkourComponent::FirstClimbLedgeResult()
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("FirstClimbLedgeResult"));
#endif

	FRotator NormalizeRotation = UPSFunctionLibrary::NormalizeDeltaRotator_Yaw(WallHitResult.ImpactNormal);
	FVector ForwardNormalVector = UKismetMathLibrary::GetForwardVector(NormalizeRotation);

	FVector CheckHoldingWallStartPos = WallHitResult.ImpactPoint + (ForwardNormalVector * -30.f);
	FVector CheckHoldingWallEndPos = WallHitResult.ImpactPoint + (ForwardNormalVector * 30.f);

	// WallRotation �� Top�� ���ϱ����� Holding Wall ��ġ �ľ�
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

// ĳ������ �������� �ڳʰ� �� �κ����� Hop�� �õ��Ҷ� �߿��� �Լ�
// ĳ���� ��ġ�� ������� Hop�� ������ ������ Depth�� ���ؼ� �ڳʰ� ���� �κ��� ������ Ȯ���Ѵ�.
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

		// �ڳʰ� ���� �κ����� �Ǵ��ϱ� ���� for��
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
					// ���������� Hit�� ������ ����
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

					// ���������� üũ�� Corner�κп��� �ڳʰ� ���̴� �κ� ����� ���̴� �� Trace ����
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

// ĳ������ �������� �����ִ� ������ Hop�ϱ����� ��ġ�� ��� �Լ�
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
		return; // ���� �Է� �� ���� �����Ƿ� ����ó��

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
		// FORWARD�� ��� ���� �ڳ� üũ�� �ϰ� �ڳ��� ��� ClimbUp ���ٴ� Corner Hop�� ���� �õ��Ѵ�.
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
	
	FVector HopStartLocation = HopClimbLedgeHitResult.ImpactPoint; // Hop �õ��� �� ��ġ
	FVector HopEndLocation = ClimbLedgeHitResult.ImpactPoint; // Hop�� ������ġ

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
		�̷��� �����Ͽ��� �� ���� ���Ͽ� 8�������� swich�� ����
		[���� if������ else if�� ���� �ϵ� �ڵ��Ŀ��� ���� (ex. if(...) else if(...) * 7)]
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
		// FreeHang�� Up�� ����. (����������)
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

		// Hop ������ ��ġ�� Ž��
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
	/*
		WallHitTraces�� �ƹ��͵� ���ٴ� ����
		1. �� ������ Hop�� ������ ��ġ�� Check ��������.
		2. ĳ���� �ٷ� �� ���̴� �ڳʶ� bStartPenetrating�� true.
		3. �Ʒ��� Hop�� ���ִ� ���� ���������ʾƼ� Drop�Ϸ��� ����.
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
			// WallHitTraces[WallHitIndex]�� WallHitResult�� CharacterLocation���� Distance ��
			float CurrentWallDistance = UKismetMathLibrary::Vector_Distance(WallHitTraces[WallHitIndex].ImpactPoint, CharacterLocation);
			float PrevWallDistance = UKismetMathLibrary::Vector_Distance(WallHitResult.ImpactPoint, CharacterLocation);

			if (CurrentWallDistance <= PrevWallDistance) // ���� Distance�� �� �۰ų� ������ ��ü
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

				// Hop�� ��ġ�� Top���� ����
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
		WallHitTraces.Empty(); // �ʱ�ȭ
		
		FVector WallForwardVector = GetForwardVector(WallRotation);
		FVector WallRightVector = GetRightVector(WallRotation);

		FRotator CharacterRotation = OwnerCharacter->GetActorRotation();
		FVector CharacterUpVector = GetUpVector(CharacterRotation);
		FVector CharacterRightVector = GetRightVector(CharacterRotation);

		FVector CornerTraceStart;
		FVector CornerTraceEnd;

		// Left or Right ���⿡ ���� Ž�� ���� ��ȭ
		float HorizontalAxis = GetHorizontalAxis();
		int32 StartIndex = HorizontalAxis < 0.f ? 0 : 4;
		int32 EndIndex = HorizontalAxis < 0.f ? 2 : 6;

		float CornerHopSelectWidth = (bCanOutCornerHop ? 50.f : 20.f) * HorizontalAxis; // Out Corner�� ��������� �Ÿ��� ª�� ����
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

// CheckHopWallTopHitResult �Լ��� ���� ����
void UParkourComponent::CheckCornerHopWallTopHitResult()
{
	if (IsLedgeValid())
	{
		FVector CharacterLocation = OwnerCharacter->GetActorLocation();
		WallHitResult = WallHitTraces[0];
		for (int32 WallHitIndex = 1; WallHitIndex < WallHitTraces.Num(); WallHitIndex++)
		{
			// WallHitTraces[WallHitIndex]�� WallHitResult�� CharacterLocation���� Distance ��
			float CurrentWallDistance = UKismetMathLibrary::Vector_Distance(WallHitTraces[WallHitIndex].ImpactPoint, CharacterLocation);
			float PrevWallDistance = UKismetMathLibrary::Vector_Distance(WallHitResult.ImpactPoint, CharacterLocation);

			if (CurrentWallDistance <= PrevWallDistance) // ���� Distance�� �� �۰ų� ������ ��ü
				WallHitResult = WallHitTraces[WallHitIndex];
			else
				WallHitResult = WallHitResult;
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

				// Hop�� ��ġ�� Top���� ����
				if (bTopHit)
					WallTopHitResult = CheckWallTopHitResult;
			}
		}
	}
}



// ã�� Hop ��ġ�� ĳ������ CapsuleComponent�� �̵��������� ������ ĸ�� ũ�⸸ŭ �˻�
// �̵� ������ ��ġ���� WallHitTraces�� Emplace ����
void UParkourComponent::CheckHopCapsuleComponent()
{
	for (int32 HopIndex = 1; HopIndex < HopHitTraces.Num(); HopIndex++)
	{
		// bBlockingHit == true: Distance / bBlockingHit == false: TraceStart~End Distance
		float CurrentDistance = GetHopResultDistance(HopHitTraces[HopIndex]);
		float PrevDistance = GetHopResultDistance(HopHitTraces[HopIndex - 1]);

		// Distance�� �񱳿����Ͽ� 5.f���̰� �߻��ϸ� �� ��ġ�� Hop�� ������ �ּ� �κ��̴�.
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



// Vertical(����)���� Return
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

// Horizontal(����)���� Return
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
		ForwardAndBackWardLeftRight_Horizontal); // Left������ ������ �������Ѵ�. 
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
		�̷��� �����Ͽ��� �� ���� ���Ͽ� 8�������� swich�� ����
		[���� if������ else if�� ���� �ϵ� �ڵ��Ŀ��� ���� (ex. if(...) else if(...) * 7)]
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

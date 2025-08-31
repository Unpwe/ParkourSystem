// Fill out your copyright notice in the Description page of Project Settings.


#include "Animations/ParkourAnimInstance.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "FunctionLibrary/PSFunctionLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"

UParkourAnimInstance::UParkourAnimInstance()
{
	ParkourStateTag = UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.State.NotBusy"));
	ParkourActionTag = UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.Action.NoAction"));
	ClimbDirectionTag = UPSFunctionLibrary::GetGameplayTag(TEXT("Parkour.Direction.NoDirection"));

	RootMotionMode = ERootMotionMode::RootMotionFromEverything;
}

void UParkourAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	OwnerCharacter = Cast<ACharacter>(TryGetPawnOwner());
	if (OwnerCharacter)
	{
		MovementComponent = OwnerCharacter->GetCharacterMovement();
	}
}

void UParkourAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (MovementComponent)
	{
		Velocity = MovementComponent->Velocity;
		GroundSpeed = Velocity.Size2D();
		bShouldMove = ((GroundSpeed > 3.f) && (MovementComponent->GetCurrentAcceleration() != FVector(0.f, 0.f, 0.f)));
		bIsFalling = MovementComponent->IsFalling();
	}
}

/* ------------  Left --------------- */
bool UParkourAnimInstance::SetLeftHandLedgeLocation(FVector TargetLeftHandLedgeLocation)
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("SetLeftHandLedgeLocation"));
#endif
	bool bClimbState = UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.Climb"));
	float InterpSpeed = UKismetMathLibrary::SelectFloat(IKInterpSpeed, IKNotClimbInterpSpeed, bClimbState);

	// Target Location으로 VInterp
	LeftHandLedgeLocation = UKismetMathLibrary::VInterpTo(LeftHandLedgeLocation, TargetLeftHandLedgeLocation, 
		GetWorld()->GetDeltaSeconds(), InterpSpeed);

	return false;
}

bool UParkourAnimInstance::SetLeftHandLedgeRotation(FRotator TargetLeftHandLedgeRotation)
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("SetLeftHandLedgeRotation"));
#endif

	bool bClimbState = UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.Climb"));
	float InterpSpeed = UKismetMathLibrary::SelectFloat(IKInterpSpeed, IKNotClimbInterpSpeed, bClimbState);

	// Target Rotation으로 VInterp
	LeftHandLedgeRotation  = UKismetMathLibrary::RInterpTo(LeftHandLedgeRotation, TargetLeftHandLedgeRotation,
		GetWorld()->GetDeltaSeconds(), InterpSpeed);

	return false;
}

bool UParkourAnimInstance::SetLeftFootLocation(FVector TargetLeftFootLocation)
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("SetLeftFootLocation"));
#endif

	bool bClimbState = UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.Climb"));
	float InterpSpeed = UKismetMathLibrary::SelectFloat(IKInterpSpeed, IKNotClimbInterpSpeed, bClimbState);

	// Target Location으로 VInterp
	LeftFootLocation = UKismetMathLibrary::VInterpTo(LeftFootLocation, TargetLeftFootLocation,
		GetWorld()->GetDeltaSeconds(), InterpSpeed);

	return false;
}

bool UParkourAnimInstance::CallSetLeftHandLedgeLocation(FVector TargetLeftHandLedgeLocation)
{
	return SetLeftHandLedgeLocation(TargetLeftHandLedgeLocation);
}

bool UParkourAnimInstance::CallSetLeftHandLedgeRotation(FRotator TargetLeftHandLedgeRotation)
{
	return SetLeftHandLedgeRotation(TargetLeftHandLedgeRotation);
}

bool UParkourAnimInstance::CallSetLeftFootLocation(FVector TargetLeftFootLocation)
{
	return SetLeftFootLocation(TargetLeftFootLocation);
}



/* ------------  Right --------------- */

bool UParkourAnimInstance::SetRightHandLedgeLocation(FVector TargetRightHandLedgeLocation)
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("SetRightHandLedgeLocation"));
#endif

	bool bClimbState = UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.Climb"));
	float InterpSpeed = UKismetMathLibrary::SelectFloat(IKInterpSpeed, IKNotClimbInterpSpeed, bClimbState);

	// Target Location으로 VInterp
	RightHandLedgeLocation = UKismetMathLibrary::VInterpTo(RightHandLedgeLocation, TargetRightHandLedgeLocation,
		GetWorld()->GetDeltaSeconds(), InterpSpeed);

	return false;
}


bool UParkourAnimInstance::SetRightHandLedgeRotation(FRotator TargetRightHandLedgeRotation)
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("SetRightHandLedgeRotation"));
#endif

	bool bClimbState = UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.Climb"));
	float InterpSpeed = UKismetMathLibrary::SelectFloat(IKInterpSpeed, IKNotClimbInterpSpeed, bClimbState);

	// Target Rotation으로 VInterp
	RightHandLedgeRotation = UKismetMathLibrary::RInterpTo(RightHandLedgeRotation, TargetRightHandLedgeRotation,
		GetWorld()->GetDeltaSeconds(), InterpSpeed);

	return false;
}


bool UParkourAnimInstance::SetRightFootLocation(FVector TargetRightFootLocation)
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Warning, TEXT("SetRightFootLocation"));
#endif

	bool bClimbState = UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.Climb"));
	float InterpSpeed = UKismetMathLibrary::SelectFloat(IKInterpSpeed, IKNotClimbInterpSpeed, bClimbState);

	// Target Location으로 VInterp
	RightFootLocation = UKismetMathLibrary::VInterpTo(RightFootLocation, TargetRightFootLocation,
		GetWorld()->GetDeltaSeconds(), InterpSpeed);

	return false;
}

bool UParkourAnimInstance::CallSetRightHandLedgeLocation(FVector TargetRightHandLedgeLocation)
{
	return SetRightHandLedgeLocation(TargetRightHandLedgeLocation);
}

bool UParkourAnimInstance::CallSetRightHandLedgeRotation(FRotator TargetRightHandLedgeRotation)
{
	return SetRightHandLedgeRotation(TargetRightHandLedgeRotation);
}

bool UParkourAnimInstance::CallRightFootLocation(FVector TargetRightFootLocation)
{
	return SetRightFootLocation(TargetRightFootLocation);
}


/*--------- Set FGameplayTag ---------*/

void UParkourAnimInstance::SetClimbDirection(FGameplayTag NewClimbDirection)
{
#ifdef DEBUG_PARKOURCOMPONENT
	LOG(Error, TEXT("Set ClimbDirectionTag : %s"), *Tagname.ToString());
#endif

	FName Tagname = UPSFunctionLibrary::GetGameplayTagName(NewClimbDirection);
	ClimbDirectionTag = NewClimbDirection;
}

void UParkourAnimInstance::SetClimbStyle(FGameplayTag NewClimbStyle)
{
	ClimbStyleTag = NewClimbStyle;
}

void UParkourAnimInstance::SetParkourAction(FGameplayTag NewAction)
{
	ParkourActionTag = NewAction;
}

void UParkourAnimInstance::SetParkourState(FGameplayTag NewState)
{
	ParkourStateTag = NewState;
}




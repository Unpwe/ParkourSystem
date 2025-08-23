// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ParkourSystem.h"
#include "Animation/AnimInstance.h"
#include "Animations/ParkourAnimInstanceInterface.h"
#include "ParkourAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class PARKOURSYSTEM_API UParkourAnimInstance : public UAnimInstance, public IParkourAnimInstanceInterface
{
	GENERATED_BODY()

public:
	UParkourAnimInstance();
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Call Hand Function")
	bool CallSetLeftHandLedgeLocation(FVector TargetLeftHandLedgeLocation);
	UFUNCTION(BlueprintCallable, Category = "Call Hand Function")
	bool CallSetLeftHandLedgeRotation(FRotator TargetLeftHandLedgeRotation);
	UFUNCTION(BlueprintCallable, Category = "Call Foot Function")
	bool CallSetLeftFootLocation(FVector TargetLeftFootLocation);

	UFUNCTION(BlueprintCallable, Category = "Call Hand Function")
	bool CallSetRightHandLedgeLocation(FVector TargetRightHandLedgeLocation);
	UFUNCTION(BlueprintCallable, Category = "Call Hand Function")
	bool CallSetRightHandLedgeRotation(FRotator TargetRightHandLedgeRotation);
	UFUNCTION(BlueprintCallable, Category = "Call Foot Function")
	bool CallRightFootLocation(FVector TargetRightFootLocation);

public:
	virtual bool SetLeftHandLedgeLocation(FVector TargetLeftHandLedgeLocation);
	virtual bool SetLeftHandLedgeRotation(FRotator TargetLeftHandLedgeRotation);

	virtual bool SetRightHandLedgeLocation(FVector TargetRightHandLedgeLocation);
	virtual bool SetRightHandLedgeRotation(FRotator TargetRightHandLedgeRotation);

	virtual bool SetLeftFootLocation(FVector TargetLeftFootLocation);
	virtual bool SetRightFootLocation(FVector TargetRightFootLocation);

	/*--------- Set FGameplayTag ---------*/
	virtual void SetClimbDirection(FGameplayTag NewClimbDirection);
	virtual void SetClimbStyle(FGameplayTag NewClimbStyle);
	virtual void SetParkourAction(FGameplayTag NewAction);
	virtual void SetParkourState(FGameplayTag NewState);


public:
	/*-----------------
		Init Values
	------------------*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init")
	class ACharacter* OwnerCharacter;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init")
	class UCharacterMovementComponent* MovementComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init")
	FVector Velocity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init")
	float GroundSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init")
	bool bIsFalling;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init")
	bool bShouldMove;


	/*--------------------
		Parkour Values
	---------------------*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayTag")
	FGameplayTag ClimbStyleTag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayTag")
	FGameplayTag ClimbDirectionTag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayTag")
	FGameplayTag ParkourStateTag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayTag")
	FGameplayTag ParkourActionTag;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IK")
	float IKInterpSpeed = 15.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IK")
	float IKNotClimbInterpSpeed = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IK|Left")
	FVector LeftHandLedgeLocation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IK|Left")
	FVector LeftFootLocation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IK|Left")
	FRotator LeftHandLedgeRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IK|Right")
	FVector RightHandLedgeLocation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IK|Right")
	FVector RightFootLocation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IK|Right")
	FRotator RightHandLedgeRotation;



	
};

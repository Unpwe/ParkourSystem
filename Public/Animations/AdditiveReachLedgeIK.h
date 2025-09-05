// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ParkourSystem.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AdditiveReachLedgeIK.generated.h"

/**
 * 
 */
UCLASS()
class PARKOURSYSTEM_API UAdditiveReachLedgeIK : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	
public:
	/* X : Forward / Y : Right / Z : Up */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Direction")
	FGameplayTag WhichTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand IK")
	bool bHand = false; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand IK")
	FVector AdditiveHandLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand IK")
	FRotator AdditiveHandRotation;

	/* Foot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foot IK")
	bool bFoot = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foot IK")
	FVector AdditiveFootLocation;

};

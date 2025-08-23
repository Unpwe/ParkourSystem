// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ParkourSystem.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ReachLedgeIK.generated.h"

/**
 * 
 */
UCLASS()
class PARKOURSYSTEM_API UReachLedgeIK : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IK")
	bool bIKStart = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Tag")
	FGameplayTag WhichHandTag;

};

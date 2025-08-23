// Fill out your copyright notice in the Description page of Project Settings.


#include "Animations/ReachLedgeIK.h"
#include "ParkourComponent/ParkourComponent.h"
#include "FunctionLibrary/PSFunctionLibrary.h"

void UReachLedgeIK::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
}

void UReachLedgeIK::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
}

void UReachLedgeIK::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp->GetOwner();
	if (Owner)
	{
		UParkourComponent* ParkourComponent = Owner->GetComponentByClass<UParkourComponent>();
		if (ParkourComponent && bIKStart)
		{
			if (UPSFunctionLibrary::CompGameplayTagName(WhichHandTag, TEXT("Parkour.Direction.Left")))
				ParkourComponent->CallMontageLeftIK(true);
			else
				ParkourComponent->CallMontageRightIK(true);
		}
	}
}

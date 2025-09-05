// Fill out your copyright notice in the Description page of Project Settings.


#include "Animations/AdditiveReachLedgeIK.h"
#include "ParkourComponent/ParkourComponent.h"
#include "FunctionLibrary/PSFunctionLibrary.h"

void UAdditiveReachLedgeIK::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
}

void UAdditiveReachLedgeIK::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
}

void UAdditiveReachLedgeIK::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp->GetOwner();
	if (Owner)
	{
		UParkourComponent* ParkourComponent = Owner->GetComponentByClass<UParkourComponent>();
		if (ParkourComponent && (bHand || bFoot))
		{
			bool bLeft = UPSFunctionLibrary::CompGameplayTagName(WhichTag, TEXT("Parkour.Direction.Left"));
			if (bHand)
			{
				ParkourComponent->CallableAdditiveHandIKLocation(bLeft, AdditiveHandLocation);
				ParkourComponent->CallableAdditiveHandIKRotation(bLeft, AdditiveHandRotation);
			}
			else if (bFoot)
			{
				ParkourComponent->CallableAdditiveFootIKLocation(bLeft, AdditiveFootLocation);
			}
		}
		
	}
}

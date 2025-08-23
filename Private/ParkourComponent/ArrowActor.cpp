// Fill out your copyright notice in the Description page of Project Settings.


#include "ParkourComponent/ArrowActor.h"
#include "Components/ArrowComponent.h"

// Sets default values
AArrowActor::AArrowActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Arrow = CreateDefaultSubobject<class UArrowComponent>(TEXT("Arrow"));
	Arrow->SetHiddenInGame(false);
	

}

// Called when the game starts or when spawned
void AArrowActor::BeginPlay()
{
	Super::BeginPlay();
	Arrow->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
}

void AArrowActor::SetHiddenInGame(bool bNewHidden)
{
	SetActorHiddenInGame(bNewHidden);
}

FVector AArrowActor::GetArrowWorldLocation()
{
	return Arrow->GetComponentLocation();
}

FRotator AArrowActor::GetArrowWorldRotation()
{
	return Arrow->GetComponentRotation();
}


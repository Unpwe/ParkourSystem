// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ParkourSystem.h"
#include "GameFramework/Actor.h"
#include "ArrowActor.generated.h"

UCLASS()
class PARKOURSYSTEM_API AArrowActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AArrowActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Arrow, meta = (AllowPrivateAccess = "true"))
	class UArrowComponent* Arrow;

protected:
	virtual void BeginPlay() override;

public:	
	void SetHiddenInGame(bool bHidden);
	FVector GetArrowWorldLocation();
	FRotator GetArrowWorldRotation();

};

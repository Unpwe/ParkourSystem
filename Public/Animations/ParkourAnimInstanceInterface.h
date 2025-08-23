// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ParkourAnimInstanceInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UParkourAnimInstanceInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PARKOURSYSTEM_API IParkourAnimInstanceInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual bool SetLeftHandLedgeLocation(FVector LeftHandLedgeLocation) = 0;
	virtual bool SetLeftHandLedgeRotation(FRotator SetLeftHandLedgeRotation) = 0;

	virtual bool SetRightHandLedgeLocation(FVector RightHandLedgeLocation) = 0;
	virtual bool SetRightHandLedgeRotation(FRotator RightHandLedgeRotation) = 0;

	virtual bool SetLeftFootLocation(FVector LeftFootLocation) = 0;
	virtual bool SetRightFootLocation(FVector RightFootLocation) = 0;

};

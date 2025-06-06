// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DebugManager.generated.h"

class ABreach;

UCLASS()
class INFERNALETESTING_API ADebugManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADebugManager();
	
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION(CallInEditor, Category = "BreachDebug") void DebugBreach();
	UFUNCTION(CallInEditor, Category = "BreachDebug") void DebugLonelyBreach();

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BreachDebug")
	TSubclassOf<ABreach> BreachClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BreachDebug")
	float DebugBreachDuration = 10.0f;
};

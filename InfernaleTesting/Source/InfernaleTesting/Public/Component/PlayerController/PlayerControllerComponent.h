// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerControllerComponent.generated.h"


class APlayerControllerInfernale;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class INFERNALETESTING_API UPlayerControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPlayerControllerComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


protected:
	UPROPERTY(BlueprintReadOnly)
	APlayerControllerInfernale* PlayerControllerInfernale;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerStateComponent.generated.h"


class APlayerStateInfernale;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class INFERNALETESTING_API UPlayerStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPlayerStateComponent();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

protected:
	APlayerStateInfernale* PlayerStateInfernale;
		
};

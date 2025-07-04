// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GRTestingDelegates.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTestDelegate);

UCLASS()
class INFERNALETESTING_API AGRTestingDelegates : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGRTestingDelegates();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void CallDelegate();

	UFUNCTION(BlueprintCallable)
	void Testing();
	

	UPROPERTY(BlueprintAssignable, BlueprintCallable);
	FTestDelegate TestDelegate;

};

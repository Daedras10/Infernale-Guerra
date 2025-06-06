// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategicViewComponent.generated.h"


UENUM(BlueprintType)
enum EStrategicViewLayer : uint8
{
	NoLayer,
	Domination,
	Power,
	Souls,
};

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class INFERNALETESTING_API UStrategicViewComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UStrategicViewComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// UFUNCTION(BlueprintCallable)
	// bool MatchFlags(UPARAM(meta = (Bitmask, BitmaskEnum = EStrategicViewLayer)) int32 Mask);

private:

	UPROPERTY(EditAnywhere, Category = "StrategicView")
	int32 LayerFlags = 0;

};

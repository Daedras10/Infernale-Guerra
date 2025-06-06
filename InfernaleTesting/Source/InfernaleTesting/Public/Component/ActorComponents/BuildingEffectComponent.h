// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuildingEffectComponent.generated.h"


class ABuilding;
enum class EBuildingEffectType : uint8;
struct FBuildingEffect;
class UBuildingEffectDataAsset;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class INFERNALETESTING_API UBuildingEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBuildingEffectComponent();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	float GetEffect(EBuildingEffectType BuildingEffectType, float InitalValue) const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	void SyncDataAsset();
	void ApplyEffects(const bool OverclockChange = false);
	void RemoveEffects(const bool Visual, const bool OverclockChange = false);

	UFUNCTION() void OnBuildingOverclocked(ABuilding* BuildingOverclocked);
	UFUNCTION() void OnBuildingDestroyed(ABuilding* BuildingDestroyed);
	UFUNCTION() void OnBuildingConstructed(ABuilding* BuildingConstructed);

	UFUNCTION(NetMulticast, Reliable) void BuildingOverclockedMulticast(const bool BuildingIsOverclocked);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Asset")
	UBuildingEffectDataAsset* BuildingEffectDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Asset")
	bool bUseDataAsset = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FBuildingEffect> BuildingEffects;

	bool bIsOverclocked = false;

		
};

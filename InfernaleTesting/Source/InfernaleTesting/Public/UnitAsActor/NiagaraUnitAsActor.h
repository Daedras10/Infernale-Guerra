// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraActor.h"
#include "NiagaraUnitAsActor.generated.h"

struct FOwner;
/**
 * 
 */
UCLASS()
class INFERNALETESTING_API ANiagaraUnitAsActor : public ANiagaraActor
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	
	void SpeedMultiplierUpdated(float SpeedMultiplier);
	void OwnerOnCreation(FOwner OwnerInfo);

	void InitOffsetAndShapes();
	FVector GetOffset();
	UFUNCTION(BlueprintCallable) bool WasInit();
	void SetNumberOfSpawners(int NewNumberOfSpawners);
	float GetBattleManagerSmoke();

	UStaticMesh* GetMesh(bool IsFighting);

	void Activate(bool Activate);


protected:
	UFUNCTION(NetMulticast, Reliable) void SpeedMultiplierUpdatedMulticast(float SpeedMultiplier);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Niagara") void SpeedMultiplierUpdatedBP(float SpeedMultiplier);
	UFUNCTION(BlueprintImplementableEvent, Category = "Niagara") void OwnerBP(FOwner OwnerInfo);
	UFUNCTION(BlueprintImplementableEvent, Category = "Niagara") void ActivateBP(bool Activate);


protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Niagara") bool Activated = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Niagara") bool TorchWasInit = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Niagara") bool UnitsWasInit = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Niagara") UCurveFloat* RandX;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Niagara") UCurveFloat* RandY;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Niagara") UCurveFloat* Shape1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Niagara") UCurveFloat* Shape2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Niagara") UCurveFloat* Shape3;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Niagara") UCurveFloat* BattleManagerSmokeCurve;
	
	float LocalSpeedMultiplier;
	FVector Offset = FVector::ZeroVector;
	FVector Shape = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Niagara") UStaticMesh* DefaultMesh;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Niagara") UStaticMesh* FightingMesh;

	bool bWasInit = false;
	int NumberOfSpawners = 1;
	float BattleManagerSmoke = 800;
};

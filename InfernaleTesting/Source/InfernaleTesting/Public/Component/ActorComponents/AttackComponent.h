// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttackComponent.generated.h"


USTRUCT(Blueprintable)
struct FAttackStruct
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackRange = 1000.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackAngle = 360.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackDamage = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackCD = 1.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool AttackUnits = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool AttackBuildings = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool AttackNeutral = true;
	
	float AttackCDLocal = 0;
};

UENUM()
enum class EAttackerType : uint8
{
	AttackerTypeUnit,
	AttackerTypeBuilding,
	AttackerTypeNeutralCamp,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAttackDelegate, FAttackStruct, Attack);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class INFERNALETESTING_API UAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAttackComponent();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void SetAttacks(TArray<FAttackStruct> NewAttacks);
	void SetStarted(bool NewStarted);

protected:
	virtual void BeginPlay() override;
	void InitAttacks();

public:	
	FAttackDelegate AttackReadyDelegate;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FAttackStruct> AttackStructs;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool Started = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bDebug = false;
};

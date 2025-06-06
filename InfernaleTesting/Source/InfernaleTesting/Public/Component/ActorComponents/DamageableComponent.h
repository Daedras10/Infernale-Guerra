// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Structs/SimpleStructs.h"
#include "DamageableComponent.generated.h"

// Damaged, Healed, Killed, Resurrected, 
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDamagedDelegate, AActor*, Actor, float, NewHealth, float, DamageAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FActorDelegate, AActor*, Actor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHealthDepletedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDestroyedActor, AActor*, Actor, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDestroyedOwner, AActor*, Actor, FOwner, DamageOwner);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCaptureDamageDelegate, ETeam, Team, float, Percent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCaptureCompleteDelegate, FOwner, Owner);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class INFERNALETESTING_API UDamageableComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDamageableComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	float GetCaptureHealthByTeam(const ETeam Team) const;
	ETeam GetCaptureTeam() const;
	bool IsCaptureMode() const;
	float GetHealth() const;
	float GetMaxHealth() const;
	void SetMaxHealthKeepPercent(const float NewMaxHealth);
	void SetMaxHealth(const float NewMaxHealth, const bool bHeal = true, const bool bHealToMax = true, const bool bHealPercent = false, const float NewHealth = -1.f);
	float HealHealth(const float HealAmount, const bool bHealPercent = false);
	float DamageHealth(const float DamageAmount, const bool bDamagePercent = false);
	float DamageHealthActor(const float DamageAmount, const bool bDamagePercent = false, TWeakObjectPtr<AActor> DamageCauser = nullptr);
	float DamageHealthOwner(const float DamageAmount, const bool bDamagePercent, const FOwner DamageOwner);

	void ResetCaptureByTeam();
	void CaptureDamage(const float DamageAmount, const FOwner DamageOwner);
	void SetCaptureMode(const bool bNewCaptureMode);

	void SetHealingAllowed(const bool bNewAllowedToHeal, const float NewHealing, const float NewHealingDelay, const float NewHealingDelaySinceLastAttack);

protected:
	float LocalDamage(const float DamageAmount, const bool bDamagePercent = false);
	int GetIndexByTeam(ETeam Team) const;
	ETeam GetTeamByIndex(int Index) const;

	void DamageTaken();
	
	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION() void OnHealthDepletedOwner(AActor* Actor, FOwner Depleter);
	UFUNCTION() void OnHealthDepletedActor(AActor* Actor, AActor* Depleter);

public:
	UPROPERTY(BlueprintAssignable) FHealthDepletedDelegate HealthDepelted;
	UPROPERTY(BlueprintAssignable) FHealthDepletedDelegate HealthFullyHealed;
	UPROPERTY(BlueprintAssignable) FDestroyedActor HealthDepeltedActor;
	UPROPERTY(BlueprintAssignable) FDestroyedOwner HealthDepeltedOwner;
	UPROPERTY(BlueprintAssignable) FDamagedDelegate Damaged;
	UPROPERTY(BlueprintAssignable) FCaptureDamageDelegate CaptureDamaged;
	UPROPERTY(BlueprintAssignable) FCaptureCompleteDelegate CaptureCompleted;
	UPROPERTY(BlueprintAssignable) FHealthDepletedDelegate CaptureCancelled;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCaptureMode = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<float> CaptureByTeam = TArray<float>();
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Health = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxHealth = 100.f;
	
	float Healing = 10.f;
	float HealingDelay = 1.f;
	float HealingDelaySinceLastAttack = 10.f;

	bool IsHealing = false;
	bool AllowedToHeal = false;
	float CurrentDelaySinceLastAttack = -1.f;
	float CurrentDelaySinceLastHeal = -1.f;
};

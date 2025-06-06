// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataAsset/BuildingEffectDataAsset.h"
#include "FogOfWar/FogOfWarComponent.h"
#include "FogOfWar/FogOfWarManager.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Damageable.h"
#include "Interfaces/FluxRepulsor.h"
#include "Interfaces/FluxStart.h"
#include "Interfaces/FluxTarget.h"
#include "Interfaces/Interactable.h"
#include "Interfaces/Ownable.h"
#include "Interfaces/UnitTargetable.h"
#include "Interfaces/VisuallyUpdatedByOwner.h"
#include "Structs/SimpleStructs.h"
#include "BuildingParent.generated.h"

class UAttacksDataAsset;
enum class EBuildingEffectType : uint8;
class AMainBuilding;
class AUnitActorManager;
class UDamageableComponent;
class AFlux;


USTRUCT(Blueprintable)
struct FEffectStruct
{
	GENERATED_BODY()
	TMap<EBuildingEffectType, FBuildingEffect> Effects = TMap<EBuildingEffectType, FBuildingEffect>();
	bool bOverclocked = false;
};



DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FBuildingOwnershipChanged, ABuildingParent*, Building, FOwner, OldOwner, FOwner, NewOwner);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBuildingParentDelegate, ABuildingParent*, Building);

UCLASS()
class INFERNALETESTING_API ABuildingParent : public AActor, public IOwnable, public IVisuallyUpdatedByOwner, public IInteractable, public IFluxStart, public IFluxTarget, public IDamageable, public IFluxRepulsor, public IUnitTargetable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABuildingParent();

	virtual void Tick(float DeltaTime) override;
	virtual FOwner GetOwner() override;
	virtual void SetOwner(FOwner NewOwner) override;
	virtual void ChangeOwner(FOwner NewOwner) override;
	virtual float DamageHealthOwner(const float DamageAmount, const bool bDamagePercent, const FOwner DamageOwner) override;
	virtual float GetTargetableRange() override;

	virtual bool CanCreateAFlux() const;
	virtual TArray<TWeakObjectPtr<AFlux>> GetFluxes();
	virtual float GetOffsetRange() override;
	virtual float GetAttackOffsetRange() override;
	virtual float GetRepulsorRange() const override;
	virtual AMainBuilding* GetMainBuilding();
	virtual void ApplyBuildingEffect(ABuilding* Source, FBuildingEffect BuildingEffect, bool UseOverclockEffect);
	virtual void RemoveBuildingEffect(ABuilding* Source, FBuildingEffect BuildingEffect, bool UpdateVisual);
	virtual void UpdateMaxHealth(float NewMaxHealth);
	virtual bool IsMainBuilding() const;
	virtual float GetThornDamage() const;
	
	virtual void InteractStartHover(APlayerControllerInfernale* Interactor) override;
	virtual void InteractEndHover(APlayerControllerInfernale* Interactor) override;

	void AddFlux(TWeakObjectPtr<AFlux> Flux);
	void RemoveFluxDelegate(TWeakObjectPtr<AFlux> Flux);
	void SetTransmutationComponent(TWeakObjectPtr<UTransmutationComponent> TransmutationComponent);

	UDamageableComponent* GetDamageableComponent();
	UFogOfWarComponent* GetFogOfWarComponent() const;

	int GetId();
	UFUNCTION(BlueprintCallable, Category = "FogOfWar") void SetId(int ID);

	UFUNCTION(BlueprintCallable, BlueprintPure) virtual FString GetBuildingName();

	UFUNCTION(BlueprintCallable, Category = "Fluxes") TArray<AFlux*> GetFluxesArrayBP();
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void RegisterBuilding();
	void FogOfWarOwnershipChanged();

	UFUNCTION() void BroadcastBuildingParentFluxesUpdated(ABuildingParent* _);
	UFUNCTION() void OnFluxUpdated(AFlux* Flux);
	UFUNCTION() void RemoveFlux(AFlux* Flux);
	UFUNCTION() void UpdateOwnerVisuals(ABuildingParent* Actor, FOwner OldOwner, FOwner NewOwner);

	UFUNCTION() virtual void OnHealthDepleted();
	UFUNCTION() virtual void OnHealthDepletedActor(AActor* Actor, AActor* DepleterActor);
	UFUNCTION() virtual void OnHealthDepletedOwner(AActor* Actor, FOwner Depleter);
	UFUNCTION() virtual void OnDamageTaken(AActor* Actor, float NewHealth, float DamageAmount);

	UFUNCTION(NetMulticast, Reliable) void SetOwnerMulticast(FOwner NewOwner);

public:
	FBuildingOwnershipChanged BuildingOwnershipChanged;
	UPROPERTY(BlueprintAssignable) FBuildingOwnershipChanged LocalBuildingOwnershipChanged;
	FBuildingParentDelegate BuildingParentDestroyed;
	FBuildingParentDelegate BuildingParentFluxesUpdated;
	UPROPERTY(BlueprintAssignable) FPCBoolDelegate Hovered;

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UDamageableComponent* DamageableComponent;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings") UAttacksDataAsset* AttacksDataAsset;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UGameSettingsDataAsset* GameSettings;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FOwner OwnerWithTeam;

	UPROPERTY(BlueprintReadWrite, EditAnywhere) float OffsetRange = 75;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float AttackOffsetRange = 335;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float RepulsorRange = 400;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float TargetableRange = 500;

	UPROPERTY()
	TArray<TWeakObjectPtr<AFlux>> Fluxes = TArray<TWeakObjectPtr<AFlux>>();

	UPROPERTY(BlueprintReadWrite, EditAnywhere) float BlowUpRadius = 1500.f; // Curently int as it refers to a number of cells

	bool bDebug = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bDebugOwnerShip = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FogOfWar")
	UFogOfWarComponent* FogOfWarComponent;
	
	TSubclassOf<AFogOfWarManager> FogOfWarManagerClass;
	bool bUseForgOfWar = true;

	AUnitActorManager* UnitActorManager;
	TMap<ABuildingParent*,FEffectStruct> EffectsPerSource = TMap<ABuildingParent*,FEffectStruct>();
	TWeakObjectPtr<UTransmutationComponent> LocalTransmutationComponent;

	int StaticBuildID;
};

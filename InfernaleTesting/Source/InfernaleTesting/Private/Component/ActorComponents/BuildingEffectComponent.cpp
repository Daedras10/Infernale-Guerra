// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/ActorComponents/BuildingEffectComponent.h"

#include "DataAsset/BuildingEffectDataAsset.h"
#include "LD/Breach.h"
#include "LD/Buildings/Building.h"
#include "LD/Buildings/MainBuilding.h"

// Sets default values for this component's properties
UBuildingEffectComponent::UBuildingEffectComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UBuildingEffectComponent::BeginPlay()
{
	Super::BeginPlay();
	SyncDataAsset();

	if (!GetOwner()->HasAuthority()) return;
	
	auto Building = Cast<ABuilding>(GetOwner());
	if (!Building) return;
	Building->BuildingOverclocked.AddDynamic(this, &UBuildingEffectComponent::OnBuildingOverclocked);
	Building->BuildingDestroyed.AddDynamic(this, &UBuildingEffectComponent::OnBuildingDestroyed);
	Building->BuildingConstructed.AddDynamic(this, &UBuildingEffectComponent::OnBuildingConstructed);
	
}

void UBuildingEffectComponent::OnBuildingOverclocked(ABuilding* BuildingOverclocked)
{
	const auto BuildingIsOverclocked = BuildingOverclocked->IsOverclocked();
	BuildingOverclockedMulticast(BuildingIsOverclocked);
	RemoveEffects(false, true);
	ApplyEffects(true);
}

void UBuildingEffectComponent::OnBuildingDestroyed(ABuilding* BuildingDestroyed)
{
	RemoveEffects(true);
}

void UBuildingEffectComponent::OnBuildingConstructed(ABuilding* BuildingConstructed)
{
	ApplyEffects();
}

void UBuildingEffectComponent::BuildingOverclockedMulticast_Implementation(const bool BuildingIsOverclocked)
{
	bIsOverclocked = BuildingIsOverclocked;
}

void UBuildingEffectComponent::SyncDataAsset()
{
	if (!bUseDataAsset) return;
	if (!BuildingEffectDataAsset)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("BuildingEffectDataAsset not found in UBuildingEffectComponent::SyncDataAsset"));
		return;
	}

	BuildingEffects = BuildingEffectDataAsset->BuildingEffects;
}

void UBuildingEffectComponent::ApplyEffects(const bool OverclockChange)
{
	const auto Building = Cast<ABuilding>(GetOwner());
	const auto MainBuilding = Building->GetMainBuilding();
	if (!MainBuilding) return;

	/* ApplyEffects to Building */
	/* ApplyEffects to MainBuilding */

	for (const auto BuildingEffect : BuildingEffects)
	{
		switch (BuildingEffect.Type) {
		case EBuildingEffectType::BuildingEffectTypeNone:
			break;
		case EBuildingEffectType::BuildingEffectTypeHealth:
			MainBuilding->ApplyBuildingEffect(Building, BuildingEffect, bIsOverclocked);
			break;
		case EBuildingEffectType::BuildingEffectTypeRange:
			MainBuilding->ApplyBuildingEffect(Building, BuildingEffect, bIsOverclocked);
			break;
		case EBuildingEffectType::BuildingEffectTypeSummonBoss:
			MainBuilding->ApplyBuildingEffect(Building, BuildingEffect, bIsOverclocked);
			break;
		case EBuildingEffectType::BuildingEffectTypeAttack:
			Building->SetAttackOverclocked(true);
			break;
		case EBuildingEffectType::BuildingEffectTypeFlux:
			if (!OverclockChange) MainBuilding->ApplyBuildingEffect(Building, BuildingEffect, bIsOverclocked);
			break;
		}
	}

	Building->Breach->BuildingAddedOnBreach.Broadcast(Building, true);
	
}

void UBuildingEffectComponent::RemoveEffects(const bool Visual, const bool OverclockChange)
{
	auto Building = Cast<ABuilding>(GetOwner());
	auto MainBuilding = Building->GetMainBuilding();
	if (!MainBuilding) return;

	/* ApplyEffects to Building */
	/* ApplyEffects to MainBuilding */

	for (auto BuildingEffect : BuildingEffects)
	{
		switch (BuildingEffect.Type) {
		case EBuildingEffectType::BuildingEffectTypeNone:
			break;
		case EBuildingEffectType::BuildingEffectTypeHealth:
			MainBuilding->RemoveBuildingEffect(Building, BuildingEffect, Visual);
			break;
		case EBuildingEffectType::BuildingEffectTypeRange:
			MainBuilding->RemoveBuildingEffect(Building, BuildingEffect, Visual);
			break;
		case EBuildingEffectType::BuildingEffectTypeSummonBoss:
			MainBuilding->RemoveBuildingEffect(Building, BuildingEffect, Visual);
		case EBuildingEffectType::BuildingEffectTypeAttack:
			Building->SetAttackOverclocked(false);
			break;
		case EBuildingEffectType::BuildingEffectTypeFlux:
			if (!OverclockChange) MainBuilding->RemoveBuildingEffect(Building, BuildingEffect, Visual);
			break;
		}
	}
}


// Called every frame
void UBuildingEffectComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

float UBuildingEffectComponent::GetEffect(EBuildingEffectType BuildingEffectType, float InitalValue) const
{
	auto Found = false;
	FBuildingEffect BuildingEffect = FBuildingEffect();
	
	for (const auto& OneBuildingEffect : BuildingEffects)
	{
		if (OneBuildingEffect.Type != BuildingEffectType) continue;
		BuildingEffect = OneBuildingEffect;
		Found = true;
	}
	if (!Found) return InitalValue;

	auto ValueEffect = bIsOverclocked ? BuildingEffect.ValueOverclocked : BuildingEffect.Value;
	if (BuildingEffect.bIsPercent) return (InitalValue * ValueEffect);
	return (InitalValue + ValueEffect);
}


// Fill out your copyright notice in the Description page of Project Settings.


#include "UnitAsActor/NiagaraUnitAsActor.h"

#include "NiagaraComponent.h"
#include "Structs/SimpleStructs.h"


void ANiagaraUnitAsActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void ANiagaraUnitAsActor::SpeedMultiplierUpdated(float SpeedMultiplier)
{
	if (SpeedMultiplier < 0) return;
	auto LastLocalSpeedMultiplier = LocalSpeedMultiplier;
	LocalSpeedMultiplier = SpeedMultiplier;
	if (LocalSpeedMultiplier != LastLocalSpeedMultiplier)
	{
		SpeedMultiplierUpdatedBP(LocalSpeedMultiplier);
		//SpeedMultiplierUpdatedMulticast(LocalSpeedMultiplier);
	}
}

void ANiagaraUnitAsActor::OwnerOnCreation(FOwner OwnerInfo)
{
	OwnerBP(OwnerInfo);
}

void ANiagaraUnitAsActor::InitOffsetAndShapes()
{
	const auto LocalRandX = RandX->GetFloatValue(NumberOfSpawners);
	const auto LocalRandY = RandY->GetFloatValue(NumberOfSpawners);
	
	const auto OffsetX = FMath::FRandRange(-LocalRandX, LocalRandX);
	const auto OffsetY = FMath::FRandRange(-LocalRandY, LocalRandY);
	Offset = FVector(OffsetX, OffsetY, 0);

	Shape = FVector(
		Shape1->GetFloatValue(NumberOfSpawners),
		Shape2->GetFloatValue(NumberOfSpawners),
		Shape3->GetFloatValue(NumberOfSpawners)
	);

	BattleManagerSmoke = BattleManagerSmokeCurve->GetFloatValue(NumberOfSpawners);

	const auto Component = GetNiagaraComponent();
	if (!Component) return;
	Component->SetFloatParameter(TEXT("Shape1"), Shape.X);
	Component->SetFloatParameter(TEXT("Shape2"), Shape.Y);
	Component->SetFloatParameter(TEXT("Shape3"), Shape.Z);

	bWasInit = true;
}

FVector ANiagaraUnitAsActor::GetOffset()
{
	return Offset;
}

bool ANiagaraUnitAsActor::WasInit()
{
	return bWasInit;
}

void ANiagaraUnitAsActor::SetNumberOfSpawners(int NewNumberOfSpawners)
{
	NumberOfSpawners = NewNumberOfSpawners;
	InitOffsetAndShapes();
}

float ANiagaraUnitAsActor::GetBattleManagerSmoke()
{
	return BattleManagerSmoke;
}

UStaticMesh* ANiagaraUnitAsActor::GetMesh(bool IsFighting)
{
	if (IsFighting) return FightingMesh;
	return DefaultMesh;
}

void ANiagaraUnitAsActor::Activate(bool Activate)
{
	ActivateBP(Activate);
}

void ANiagaraUnitAsActor::SpeedMultiplierUpdatedMulticast_Implementation(float SpeedMultiplier)
{
	SpeedMultiplierUpdatedBP(SpeedMultiplier);
}

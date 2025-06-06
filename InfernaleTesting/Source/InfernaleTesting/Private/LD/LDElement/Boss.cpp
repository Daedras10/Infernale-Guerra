// Fill out your copyright notice in the Description page of Project Settings.


#include "LD/LDElement/Boss.h"

#include "Component/ActorComponents/AttackComponent.h"
#include "Component/ActorComponents/DamageableComponent.h"
#include "Component/ActorComponents/EffectAfterDelayComponent.h"
#include "DataAsset/NeutralCampDataAsset.h"
#include "FunctionLibraries/FunctionLibraryInfernale.h"
#include "Mass/Army/AmalgamFragments.h"
#include "Mass/Collision/SpatialHashGrid.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ABoss::ABoss()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// DynamicMaterial = UMaterialInstanceDynamic::Create(StaticMesh->GetMaterial(0), StaticMesh);
	// StaticMesh->SetMaterial(0, DynamicMaterial);

	CapturePlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>("CapturePlaneMesh");
	CapturePlaneMesh->SetupAttachment(RootComponent);
	CapturePlaneMesh->SetVisibility(false);
	
}

void ABoss::CaptureHintUpdateServer_Implementation()
{
	CaptureHintUpdateReplicated();
	MaxValue = 0.f;
	MaxKey = ETeam::NatureTeam;
	for (auto CaptureData : TeamCaptureData)
	{
		if (CaptureData.Value > MaxValue)
		{
			MaxValue = CaptureData.Value;
			MaxKey = CaptureData.Key;
		}
	}
}


void ABoss::CaptureHintUpdateReplicated_Implementation()
{
	// auto Capture = FMath::Lerp(4.f, 1.f, MaxValue);
	// if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Capture Value: %f Max Value: %f"), Capture, MaxValue));
	FVector ColorToVector{UFunctionLibraryInfernale::GetOldTeamColorCpp(MaxKey)};
	// UpdateCapturePlaneMaterialReplicated(Capture, ColorToVector);
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Capture Value: %f"), MaxValue));
	UpdateCaptureBarBP(MaxValue, ColorToVector);
}

void ABoss::CaptureBoss()
{
	if (GetWorld()->GetTimeSeconds() - LastCaptureTickTime < NeutralCampDataAsset->BossCaptureTickTime || bCaptured) return;
	LastCaptureTickTime = GetWorld()->GetTimeSeconds();
	
	auto Teams = GetTeamInCaptureRange();
	if (Teams.Num() == 0)
	{
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("No team in capture range"));
		return;
	}
	if (Teams.Num() == 1)
	{
		ETeam Team = Teams[0];
		if (Team == ETeam::NatureTeam) return;
		bIsContested = false;
		if (TeamCaptureData.Contains(Team))
		{
			if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Team %s is capturing the boss"), *UEnum::GetValueAsString(Team)));
			for (auto CaptureData : TeamCaptureData)
			{
				if (CaptureData.Key != Team)
				{
					CaptureData.Value -= (NeutralCampDataAsset->BossCapturePointPerTick/TeamCaptureData.Num());
					TeamCaptureData[CaptureData.Key] = FMath::Clamp(CaptureData.Value, 0.f, 1.f);
				}
				else
				{
					CaptureData.Value += NeutralCampDataAsset->BossCapturePointPerTick;
					TeamCaptureData[CaptureData.Key] = FMath::Clamp(CaptureData.Value, 0.f, 1.f);
					if (CaptureData.Value >= 1.f)
					{
						FOwner CaptureOwner;
						for (auto CurrentOwner : OwnerForCapture)
						{
							if (CurrentOwner.Team == Team)
							{
								CaptureOwner = CurrentOwner;
								break;
							}
						}
						OnCaptureCompleted(CaptureOwner);

						bTickRequiredForBoss = false;
						EnableTickForActor(false);
						bCaptured = true;
						CaptureHintUpdateServer();
						return;
					}
				}
			}
		}
		else
		{
			if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("New Team %s is capturing the boss"), *UEnum::GetValueAsString(Team)));
			for (auto CaptureData : TeamCaptureData)
			{
				CaptureData.Value -= (NeutralCampDataAsset->BossCapturePointPerTick/TeamCaptureData.Num());
				TeamCaptureData[CaptureData.Key] = FMath::Clamp(CaptureData.Value, 0.f, 1.f);
			}
			TeamCaptureData.Add(Team, NeutralCampDataAsset->BossCapturePointPerTick);
		}
	}
	else
	{
		bIsContested = true;
	}
	CaptureHintUpdateServer();
}

// Called when the game starts or when spawned
void ABoss::BeginPlay()
{
	Super::BeginPlay();
	EnableTickForActor(true);
	EffectAfterDelayComponent->Stop();
	DamageableComponent->CaptureCompleted.AddDynamic(this, &ABoss::OnCaptureCompleted);

	ChargePerKill = NeutralCampDataAsset->BossSummonPointsPerSacrifice;
	ASpatialHashGrid::SetBossAsKnown(this);
	
}


void ABoss::OnDeath(AActor* _, FOwner Depleter)
{
	IsAlive = false;
	VisualDeathMulticast(Depleter);
	KillInGrid();
	bDead = true;
	OnDeadChangedMulticast(bDead);
	bTickRequiredForBoss = true;
	EnableTickForActor(true);
	// UpdateCapturePlaneVisibilityReplicated(true);
	// SpawnInGrid();
	// DamageableComponent->SetCaptureMode(true);
	// DamageableComponent->HealHealth(1.f, true);
	// DamageableComponent->ResetCaptureByTeam();
	// StaticMesh->SetVectorParameterValueOnMaterials("Color", FVector(1.0f, 0.0f, 0.0f));
}

void ABoss::UpdateCapturePlaneVisibilityReplicated_Implementation(bool bVisible)
{
	CapturePlaneMesh->SetVisibility(bVisible);
}


void ABoss::OnCaptureCompletedMulticast_Implementation(FOwner CaptureOwner)
{
	OnCaptureCompletedBP(CaptureOwner);
}

void ABoss::CaptureEffects(FOwner CaptureOwner)
{
	DeathEffects(CaptureOwner);
}

void ABoss::OnCaptureCompleted(FOwner CaptureOwner)
{
	bAwake = false;
	CaptureEffects(CaptureOwner);
	OnCaptureCompletedMulticast(CaptureOwner);
	ASpatialHashGrid::Instance->RemoveBossAsKnown(this);
}

void ABoss::OnDeadChangedMulticast_Implementation(bool bDeadVal)
{
	OnDeadChangedBP(bDeadVal);
}

void ABoss::OnAwakeChangedMulticast_Implementation(bool bAwakeVal)
{
	OnAwakeChangedBP(bAwakeVal);
}

void ABoss::OnAttackChangedMulticast_Implementation(bool bAttackingVal)
{
	OnAttackChangedBP(bAttackingVal);
}

// Called every frame
void ABoss::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bDead)
	{
		CaptureBoss();
	}

	
}

void ABoss::SetSpawnCharge(float Charge)
{
	if (bHasBeenSpawned) return;
	bTickRequiredForBoss = true;
	EnableTickForActor(true);
	SpawnCharge = Charge;
	OnSpawnChargeChangedBP(SpawnCharge);
	if (SpawnCharge >= 100.f)
	{
		SpawnCharge = 0.f;
		bHasBeenSpawned = true;
		bAwake = true;
		OnAwakeChangedMulticast(bAwake);
		// StaticMesh->SetVectorParameterValueOnMaterials("Color", FVector(0.f, 1.f, 0.0f));
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Boss has been spawned"));
		OnBossSpawned.Broadcast();
		if (!HasAuthority()) return;
		Respawn();
	}
}

void ABoss::AddToSpawnCharge(float Charge)
{
	AddToSpawnChargeMulticast(Charge);
}

void ABoss::AskAddToSpawnCharge(float Charge)
{
	const auto PlayerController = Cast<APlayerControllerInfernale>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (!PlayerController) return;
	PlayerController->BossAddToSpawnCharge(this, Charge);
	
}

void ABoss::AddToSpawnChargeMulticast_Implementation(float Charge)
{
	if (bHasBeenSpawned)
	{
		return;
	}
	SpawnCharge += Charge;
	UpdateChargeAddedPerTick(SpawnCharge, SpawnCharge + Charge);
	
	bTickRequiredForBoss = true;
	EnableTickForActor(true);
	OnSpawnChargeChangedBP(SpawnCharge);
	if (SpawnCharge >= 100.f)
	{
		SpawnCharge = 0.f;
		bHasBeenSpawned = true;
		bAwake = true;
		OnAwakeChangedMulticast(bAwake);
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Boss has been spawned"));
		// StaticMesh->SetVectorParameterValueOnMaterials("Color", FVector(0.f, 1.f, 0.0f));
		if (!HasAuthority()) return;
		Respawn();
	}
}


float ABoss::GetSpawnCharge()
{
	return SpawnCharge;
}

void ABoss::UpdateChargeAddedPerTick(float OldValue, float NewValue)
{
	ChargeAddedPerTick += (NewValue - OldValue);
}

void ABoss::DebugBossCaptureRange()
{
	float Radius = 300.f * NeutralCampDataAsset->BossCaptureRadius;
	DrawDebugSphere(GetWorld(), GetActorLocation(), Radius, 12, FColor::Red, false, 5.f);
}

float ABoss::GetSummonRange()
{
	return NeutralCampDataAsset->BossCaptureRadius * ASpatialHashGrid::Instance->CellSizeX;
}

bool ABoss::OnAttackReady(FAttackStruct AttackStruct)
{
	if (!bAwake) return false;
	const auto bWasAttacking = bAttacking;
	bAttacking = Super::OnAttackReady(AttackStruct);
	if (bWasAttacking != bAttacking) OnAttackChangedMulticast(bAttacking);

	return bAttacking;
}

bool ABoss::IsAttackableBy(ETeam Team)
{
	return bAwake;
}

EEntityType ABoss::GetEntityType() const
{
	return EEntityType::EntityTypeBoss;
}

void ABoss::EnableTickForActor(bool bEnable)
{
	Super::EnableTickForActor(bEnable || bTickRequiredForBoss);
}

void ABoss::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABoss, SpawnCharge);
	DOREPLIFETIME(ABoss, bHasBeenSpawned);
	DOREPLIFETIME(ABoss, bCaptured);
	DOREPLIFETIME(ABoss, MaxValue);
	DOREPLIFETIME(ABoss, MaxKey);
	DOREPLIFETIME(ABoss, bIsContested);
}

TArray<ETeam> ABoss::GetTeamInCaptureRange()
{
	TArray<ETeam> TeamsInRange;
	auto UnitsInCaptureRange = ASpatialHashGrid::FindEntitiesAroundCell(GetActorLocation(),NeutralCampDataAsset->BossCaptureRadius);
	if (UnitsInCaptureRange.Num() == 0)
	{
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("No units in capture range"));
		return TeamsInRange;
	}
	for (auto Unit : UnitsInCaptureRange)
    {
		const auto Data = ASpatialHashGrid::GetMutableEntityData(Unit.Key);
		if (Data == nullptr) continue;
		auto TeamOfUnit = Data->Owner.Team;
		OwnerForCapture.AddUnique(Data->Owner);
		TeamsInRange.AddUnique(TeamOfUnit);
    }
	return TeamsInRange;
}

void ABoss::UpdateCapturePlaneMaterialReplicated_Implementation(float Capture, FVector ColorToVector)
{
	CapturePlaneMesh->SetScalarParameterValueOnMaterials("TextureScale", Capture);
	CapturePlaneMesh->SetVectorParameterValueOnMaterials("Color", ColorToVector);
}
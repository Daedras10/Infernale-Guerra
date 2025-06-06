// Fill out your copyright notice in the Description page of Project Settings.


#include "UnitAsActor/UnitActor.h"

#include "NiagaraComponent.h"
#include "Component/ActorComponents/DamageableComponent.h"
#include "Component/PlayerState/TransmutationComponent.h"
#include "Components/SplineComponent.h"
#include "DataAsset/GameSettingsDataAsset.h"
#include "Flux/Flux.h"
#include "FogOfWar/FogOfWarManager.h"
#include "FunctionLibraries/FunctionLibraryInfernale.h"
#include "GameMode/Infernale/PlayerStateInfernale.h"
#include "Kismet/GameplayStatics.h"
#include "LD/Buildings/BuildingParent.h"
#include "LD/LDElement/NeutralCamp.h"
#include "Manager/UnitActorManager.h"
#include "UnitAsActor/NiagaraUnitAsActor.h"

// Sets default values
AUnitActor::AUnitActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	DamageableComponent = CreateDefaultSubobject<UDamageableComponent>(TEXT("DamageableComponent"));
	FogOfWarComponent = CreateDefaultSubobject<UFogOfWarComponent>(TEXT("FogOfWarComponent"));
	// FogOfWarComponent->FogHoleMesh->SetupAttachment(RootComponent);
	FogOfWarComponent->SetVisibilityOfActorWithFog(false, false);

	Path = TArray<FVector>();
}

// Called every frame
void AUnitActor::Tick(float DeltaTime)
{
	if (bIsGettingDestroyed) return;
	Super::Tick(DeltaTime);
	SyncNiagaraActor();

	if (!bWasInit) return;
	MovementAI(DeltaTime);
	
	
	if (!HasAuthority()) return;
	CheckRangeTimer -= DeltaTime;
	AttackTimer -= DeltaTime;
	
	if (CheckRangeTimer <= 0)
	{
		CheckRangeTimer = BaseCheckRangeTimer;
		UnitStateAsk.Broadcast(this, EUnitStateAsk::UnitStateAskRange);
	}
	if (AttackTimer <= 0)
	{
		AttackTimer = UnitStruct.BaseAttackCD;
		if (bTargetIsValid) UnitStateAsk.Broadcast(this, EUnitStateAsk::UnitStateAskAttack);
	}

}

void AUnitActor::SetOwnerMulticast_Implementation(FOwner NewOwner)
{
	OwnerInfo = NewOwner;
	OwnerChanged.Broadcast(this, OwnerInfo);
	RefreshVisualOwned();
	UpdateFogOfWar();
}

FOwner AUnitActor::GetOwner()
{
	return OwnerInfo;
}

void AUnitActor::SetOwner(FOwner NewOwner)
{
	OwnerInfo = NewOwner;
	SetOwnerMulticast(NewOwner);
}

void AUnitActor::ChangeOwner(FOwner NewOwner)
{
	SetOwner(NewOwner);
}

float AUnitActor::DamageHealthOwner(const float DamageAmount, const bool bDamagePercent, const FOwner DamageOwner)
{
	return DamageableComponent->DamageHealthOwner(DamageAmount, bDamagePercent, DamageOwner);
}

float AUnitActor::GetTargetableRange()
{
	return UnitStruct.TargetableRange;
}

void AUnitActor::Init(FUnitStruct NewUnitStruct, FOwner NewOwner, TWeakObjectPtr<AFlux> FluxTarget, TWeakObjectPtr<AUnitActorManager> NewUnitActorManager)
{
	UnitStruct = NewUnitStruct;
	OwnerInfo = NewOwner;
	SetOwner(NewOwner);
	SetNiagaraActorMulticast(UnitStruct.UnitNiagaraActorClasses[FMath::RandRange(0, UnitStruct.UnitNiagaraActorClasses.Num() - 1)]);
	SetUnitStructMulticast(UnitStruct);
	UnitActorManager = NewUnitActorManager;
	
	Flux = FluxTarget;
	bFluxValid = true;
	Flux->FluxFinishUpdate.AddDynamic(this, &AUnitActor::AskRefreshFluxPath);
	// Flux->FluxNodeAdded.AddDynamic(this, &AUnitActor::OnFluxNodeAdded);
	// Flux->FluxNodeRemoved.AddDynamic(this, &AUnitActor::OnFluxNodeRemoved);
	Flux->FluxDestroyed.AddDynamic(this, &AUnitActor::OnFluxDestroyed);
	
	CurrentSplineIndex = 0;
	RefreshPath(true);
	//SetMovementLocallyMulticast(true);
	SetReplicateMovementServer(false);

	CheckRangeTimer = BaseCheckRangeTimer;
	AttackTimer = UnitStruct.BaseAttackCD;
	TransmutationComponent = NewUnitActorManager->GetTransmutationComponent(OwnerInfo.Player);

	auto MaxHealth = TransmutationComponent->ApplyEffect(UnitStruct.BaseHealth, ENodeEffect::NodeEffectHealthUnit);
	if (!DamageableComponent)
	{
		DestroyThisUnit();
		return;
	}
	DamageableComponent->SetMaxHealth(UnitStruct.BaseHealth, true, true, false, MaxHealth);
	bWasInit = true;
	InitFogMulticast(OwnerInfo);
	InitDoneMulticast();
}

FUnitStruct AUnitActor::GetUnitStruct() const
{
	return UnitStruct;
}

void AUnitActor::SetTarget(TWeakObjectPtr<AActor> NewTarget, EUnitTargetType NewTargetType)
{
	if (bIsGettingDestroyed) return;
	const auto Authority = HasAuthority();
	const auto WasValid = bTargetIsValid;

	if (NewTarget.Get() == TargetActor.Get()) return;
	if (!TargetActor.IsValid()) RemoveLastTarget();
	
	TargetActor = NewTarget;
	TargetType = NewTargetType;
	bTargetIsValid = TargetActor != nullptr;

	if (!Authority) return;
	
	if (!bTargetIsValid)
	{
		AttackTimer = UnitStruct.BaseAttackCD;
		if (WasValid)
		{
			SetReplicateMovementServer(false);
			//SetMovementLocallyMulticast(true);
		}
		return;
	}

	if (!WasValid && NewTargetType != EUnitTargetType::UTargetNone)
	{
		//SetMovementLocallyMulticast(false);
		SetReplicateMovementServer(true);
	}

	switch (TargetType) {
	case EUnitTargetType::UTargetNone:
		TargetType = EUnitTargetType::UTargetNone;
		TargetActor = nullptr;
		bTargetIsValid = false;
		return;
	case EUnitTargetType::UTargetBuilding:
		TargetBuilding();
		break;
	case EUnitTargetType::UTargetUnit:
		TargetUnit();
		break;
	case EUnitTargetType::UTargetNeutralCamp:
		TargetNeutralCamp();
		break;
	}
}

void AUnitActor::SetTargetToReplicate(TWeakObjectPtr<AActor> NewTarget, EUnitTargetType NewTargetType)
{
	SetTargetMulticast(NewTarget.Get(), NewTargetType);
}

void AUnitActor::TargetBuilding()
{
	if (!HasAuthority()) return;
	auto Building = Cast<ABuildingParent>(TargetActor);
	if (!Building) return;
	Building->BuildingParentDestroyed.AddDynamic(this, &AUnitActor::OnBuildingParentDestroyed);
}

void AUnitActor::TargetUnit()
{
	if (!HasAuthority()) return;
	auto UnitActor = Cast<AUnitActor>(TargetActor);
	if (!UnitActor) return;
	UnitActor->AddAttackers(TWeakObjectPtr<AUnitActor>(this));
	UnitActor->UnitActorDestroyed.AddDynamic(this, &AUnitActor::OnUnitActorDestroyed);
}

void AUnitActor::TargetNeutralCamp()
{
	if (!HasAuthority()) return;
	auto NeutralCamp = Cast<ANeutralCamp>(TargetActor);
	if (!NeutralCamp) return;
	NeutralCamp->LDElementRemoved.AddDynamic(this, &AUnitActor::OnNeutralCampRemoved);
}

void AUnitActor::CreatePathArray(TWeakObjectPtr<AFlux> FluxTarget, bool StartAtBeginning)
{
	Path.Empty();
	if (!FluxTarget.IsValid())
	{
		if (HasAuthority()) DestroyThisUnit();
		return;
	}
	const auto SplineComponent = FluxTarget->GetSplineForAmalgamsComponent();
	if (!SplineComponent)
	{
		if (HasAuthority()) DestroyThisUnit();
		return;
	}
	if (!this) return;
	const auto Location = GetActorLocation();
	const auto ClosestPointAlongSpline = StartAtBeginning ?
		SplineComponent->GetLocationAtDistanceAlongSpline(0.f, ESplineCoordinateSpace::World) :
		SplineComponent->FindLocationClosestToWorldLocation(Location, ESplineCoordinateSpace::World);
	const auto MaxDistance = SplineComponent->GetSplineLength();
	auto CurrentDistance = SplineComponent->GetDistanceAlongSplineAtLocation(ClosestPointAlongSpline, ESplineCoordinateSpace::World);
	if (StartAtBeginning) CurrentDistance = 0.f;
	
	Path.Empty();
	Path.Add(ClosestPointAlongSpline);

	while (CurrentDistance < MaxDistance)
	{
		CurrentDistance += SpaceBetweenPathfindingNodes;
		CurrentDistance = FMath::Clamp(CurrentDistance, 0.f, MaxDistance);
		auto NextPoint = SplineComponent->GetLocationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::World);
		Path.Add(NextPoint);
	}
	
}

void AUnitActor::CopyPathFromFlux(TWeakObjectPtr<AFlux> FluxTarget, bool StartAtBeginning)
{
	Path.Empty();
	if (!FluxTarget.IsValid())
	{
		if (HasAuthority()) DestroyThisUnit();
		return;
	}
	auto FluxPath = FluxTarget->GetPath();
	if (FluxPath.Num() == 0)
    {
        if (HasAuthority()) DestroyThisUnit();
        return;
    }

	auto StartIndex = StartAtBeginning ? 0 : -1;
	if (!StartAtBeginning)
	{
		if (!this) return;
		const auto Location = GetActorLocation();
		auto ClosestPoint = FluxPath[0];
		auto ClosestDistance = FVector::Dist(Location, ClosestPoint);
		StartIndex = 0;
		for (int i = 1; i < FluxPath.Num(); i++)
        {
            const auto Distance = FVector::Dist(Location, FluxPath[i]);
            if (Distance >= ClosestDistance) continue;
            ClosestDistance = Distance;
            ClosestPoint = FluxPath[i];
            StartIndex = i;
        }
	}

	Path = TArray<FVector>();
	for (int i = StartIndex; i < FluxPath.Num(); i++)
    {
        Path.Add(FluxPath[i]);
    }
}

void AUnitActor::RemoveLastTarget()
{
	if (!HasAuthority()) return;
	if (!TargetActor.IsValid()) return;
	switch (TargetType) {
		case EUnitTargetType::UTargetBuilding:
		{
			auto Building = Cast<ABuildingParent>(TargetActor);
			if (Building) Building->BuildingParentDestroyed.RemoveDynamic(this, &AUnitActor::OnBuildingParentDestroyed);
			break;
		}
		case EUnitTargetType::UTargetUnit:
		{
			auto UnitActor = Cast<AUnitActor>(TargetActor);
			if (UnitActor)
			{
				UnitActor->RemoveAttackers(TWeakObjectPtr<AUnitActor>(this));
				UnitActor->UnitActorDestroyed.RemoveDynamic(this, &AUnitActor::OnUnitActorDestroyed);
			}
			break;
		}
		case EUnitTargetType::UTargetNeutralCamp:
		{
			auto NeutralCamp = Cast<ANeutralCamp>(TargetActor);
			if (NeutralCamp) NeutralCamp->LDElementRemoved.RemoveDynamic(this, &AUnitActor::OnNeutralCampRemoved);
			break;
		}
	case EUnitTargetType::UTargetNone:
		break;
	}
}

TWeakObjectPtr<AActor> AUnitActor::GetTarget() const
{
	return TargetActor;
}

EUnitTargetType AUnitActor::GetTargetType() const
{
	return TargetType;
}

TWeakObjectPtr<UFogOfWarComponent> AUnitActor::GetFogOfWarComponent() const
{
	return FogOfWarComponent;
}

void AUnitActor::AddUnitForPathfindingRefresh()
{
	if (!HasAuthority()) return;
	UnitActorManager->AddUnitToPathfind(this);
}

void AUnitActor::UpdateMaxHealth(float NewMaxHealth)
{
	DamageableComponent->SetMaxHealthKeepPercent(NewMaxHealth);
}

void AUnitActor::UpdateSightFogOfWar(float NewSight)
{
	//TODO (and replicate on client, will be called on server)
	//Should be called on init when fog of war is setup as well with
	// const auto NewSight = TransmutationComponent->ApplyEffect(UnitStruct.BaseSightRange, ENodeEffect::NodeEffectUnitSight);
	// UpdateSightFogOfWar(NewSight);
	// So it's setup initialy on new units as well
	UpdateSightFogOfWarMulticast(NewSight);
}

bool AUnitActor::CanHaveAdditionalAttackers() const
{
	return Attackers.Num() < UnitStruct.BaseMaxAttackers;
}

bool AUnitActor::IsAttacker(TWeakObjectPtr<AUnitActor> Attacker) const
{
	return Attackers.Contains(Attacker);
}

void AUnitActor::AddAttackers(TWeakObjectPtr<AUnitActor> Attacker)
{
	Attackers.Add(Attacker);
	Attacker->UnitActorDestroyed.AddDynamic(this, &AUnitActor::OnAttackersUnitActorDestroyed);
}

void AUnitActor::RemoveAttackers(TWeakObjectPtr<AUnitActor> Attacker)
{
	Attackers.Remove(Attacker);
	Attacker->UnitActorDestroyed.RemoveDynamic(this, &AUnitActor::OnAttackersUnitActorDestroyed);
}

bool AUnitActor::IsGettingDestroyed() const
{
	return bIsGettingDestroyed;
}

void AUnitActor::SetReplicateMovementServer_Implementation(bool bInReplicateMovement)
{
	SetReplicatingMovement(bInReplicateMovement);
	SetReplicateMovementMulticast(bInReplicateMovement);
}

// Called when the game starts or when spawned
void AUnitActor::BeginPlay()
{
	Super::BeginPlay();
	DamageableComponent->Damaged.AddDynamic(this, &AUnitActor::OnDamaged);
	DamageableComponent->HealthDepelted.AddDynamic(this, &AUnitActor::OnHealthDepleted);

	if (HasAuthority()) return;
	auto PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	TransmutationComponent = Cast<APlayerControllerInfernale>(PlayerController)->GetTransmutationComponent();
}

void AUnitActor::RefreshPath(bool StartAtBeginning)
{
	if (bIsGettingDestroyed) return;
	Path = TArray<FVector>();
	if (bFluxValid) CalculatePath(StartAtBeginning);
	if (bIsGettingDestroyed) return;
	
	if (Path.Num() == 0)
	{
		DestroyThisUnit();
		return;
	}
}

void AUnitActor::CalculatePath(bool StartAtBeginning)
{
	if (bIsGettingDestroyed) return;
	//CreatePathArray(Flux, StartAtBeginning); // Locally calculate pathfinding
	CopyPathFromFlux(Flux, StartAtBeginning); // Copy of the pathfinding from flux
	if (bIsGettingDestroyed) return;
	//ReplicatePathMulticast(GetActorLocation(), Path);
	ReplicateCalculatePathMulticast(Flux.Get(), StartAtBeginning);
	//FVector NewLocation = Path[0];
}

void AUnitActor::UpdateFogOfWar(bool bRemove)
{
	TWeakObjectPtr<AUnitActor> WeakThis = TWeakObjectPtr<AUnitActor>(this);
	bUseForgOfWar = UFunctionLibraryInfernale::GetGameSettingsDataAsset()->UseFogOfWar;
	FogOfWarManagerClass = UFunctionLibraryInfernale::GetGameSettingsDataAsset()->FogOfWarManagerClass;
	if (!bUseForgOfWar) return;
	auto LocalPlayer = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	auto LocalPlayerInfernale = Cast<APlayerControllerInfernale>(LocalPlayer);
	if (LocalPlayerInfernale->GetOwnerInfo().Team != OwnerInfo.Team) return;
	
	auto ActorArray = TArray<AActor*>();
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), FogOfWarManagerClass, ActorArray);
	if (ActorArray.Num() == 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AUnitActor Error : \n\t Unable to find FogOfWarManager"));
		return;
	}
	const auto FogManager = Cast<AFogOfWarManager>(ActorArray[0]);
	if (!FogManager)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AUnitActor Error : \n\t Unable to cast FogOfWarManager"));
		return;
	}
	if (!bRemove)
	{
		FogManager->AddMovingActorVision(WeakThis, UnitStruct.BaseSightRange, UnitStruct.BaseSightType);
		return;
	}
	FogManager->RemoveMovingActorVision(WeakThis);
}

void AUnitActor::SyncNiagaraActor()
{
	if (!bNiagaraActorInit) return;
	if (!NiagaraActor.IsValid()) return;
	auto NewLocation = GetActorLocation();
	NewLocation.Z += UnitStruct.NiagaraHeightOffset;
	NiagaraActor->SetActorLocation(NewLocation);
	auto NewRotation = GetActorRotation();
	auto Offset = UnitStruct.NiagaraRotationOffset;
	NewRotation.Add(Offset.X, Offset.Y, Offset.Z);
	NiagaraActor->SetActorRotation(NewRotation);
	//NewRotation2 = NewRotation2.RotateVector(DummyVector);
	FRotator NewRotation2 = NewRotation;
	NiagaraActor->GetNiagaraComponent()->SetVariableQuat("MeshOrientation", FQuat(NewRotation2));
}

void AUnitActor::RefreshFluxPathOwnFlux()
{
	RefreshPath(false);
}

void AUnitActor::AskRefreshFluxPath(AFlux* FluxTarget)
{
	Flux = TWeakObjectPtr<AFlux>(FluxTarget);
	AddUnitForPathfindingRefresh();
}

void AUnitActor::OnFluxNodeAdded(AFlux* FluxUpdated, int Index)
{
	if (Index < CurrentSplineIndex) CurrentSplineIndex++;
}

void AUnitActor::OnFluxNodeRemoved(AFlux* FluxUpdated, int Index)
{
	if (Index < CurrentSplineIndex) CurrentSplineIndex--;
}

void AUnitActor::OnUnitActorDestroyed(AUnitActor* UnitActor)
{
	TWeakObjectPtr<AUnitActor> Destroyed = TWeakObjectPtr<AUnitActor>(UnitActor);
	if (!Destroyed.IsValid()) return;
	if (UnitActor != TargetActor) return;
	UnitStateAsk.Broadcast(this, EUnitStateAsk::UnitStateAskRange);
	CheckRangeTimer = BaseCheckRangeTimer;
	//SetTargetToReplicate(nullptr, EUnitTargetType::UTargetNone);
}

void AUnitActor::OnAttackersUnitActorDestroyed(AUnitActor* UnitActor)
{
	Attackers.Remove(UnitActor);
}

void AUnitActor::OnBuildingParentDestroyed(ABuildingParent* Building)
{
	TWeakObjectPtr<ABuildingParent> Removed = TWeakObjectPtr<ABuildingParent>(Building);
	if (!Removed.IsValid()) return;
	if (Building != TargetActor) return;
	UnitStateAsk.Broadcast(this, EUnitStateAsk::UnitStateAskRange);
	CheckRangeTimer = BaseCheckRangeTimer;
	//SetTargetToReplicate(nullptr, EUnitTargetType::UTargetNone);
}

void AUnitActor::OnNeutralCampRemoved(ALDElement* Element)
{
	TWeakObjectPtr<ALDElement> Removed = TWeakObjectPtr<ALDElement>(Element);
	if (!Removed.IsValid()) return;
	if (Element != TargetActor) return;
	UnitStateAsk.Broadcast(this, EUnitStateAsk::UnitStateAskRange);
	CheckRangeTimer = BaseCheckRangeTimer;
	//SetTargetToReplicate(nullptr, EUnitTargetType::UTargetNone);
}

void AUnitActor::OnDamaged(AActor* Actor, float NewHealth, float DamageAmount)
{
	OnDamagedMulticast(DamageableComponent->GetHealth(), DamageableComponent->GetMaxHealth(), DamageAmount);
}

void AUnitActor::OnHealthDepleted()
{
	DestroyThisUnit();
}

void AUnitActor::OnFluxDestroyed(AFlux* FluxDestroyed)
{
	Flux = nullptr;
	bFluxValid = false;
}

void AUnitActor::UpdateSightFogOfWarMulticast_Implementation(float NewSight)
{
	if (!bUseForgOfWar) return;
	auto LocalPlayer = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	auto LocalPlayerInfernale = Cast<APlayerControllerInfernale>(LocalPlayer);
	if (LocalPlayerInfernale->GetOwnerInfo().Team != OwnerInfo.Team) return;
	
	auto ActorArray = TArray<AActor*>();
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), FogOfWarManagerClass, ActorArray);
	if (ActorArray.Num() == 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AUnitActor Error : \n\t Unable to find FogOfWarManager"));
		return;
	}
	const auto FogManager = Cast<AFogOfWarManager>(ActorArray[0]);
	if (!FogManager)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AUnitActor Error : \n\t Unable to cast FogOfWarManager"));
		return;
	}
	FogManager->ModifyMovingActorVision(this, NewSight, UnitStruct.BaseSightType);
}

void AUnitActor::InitDoneMulticast_Implementation()
{
	if (HasAuthority()) return;
	bWasInit = true;
}

void AUnitActor::SetUnitStructMulticast_Implementation(FUnitStruct NewUnitStruct)
{
	if (HasAuthority()) return;
	UnitStruct = NewUnitStruct;
}

void AUnitActor::InitFogMulticast_Implementation(FOwner NewOwner)
{
	bUseForgOfWar = UFunctionLibraryInfernale::GetGameSettingsDataAsset()->UseFogOfWar;
	FogOfWarManagerClass = UFunctionLibraryInfernale::GetGameSettingsDataAsset()->FogOfWarManagerClass;
	
	if (!bUseForgOfWar) return;
	auto PlayerState = GetWorld()->GetFirstPlayerController()->GetPlayerState<APlayerStateInfernale>();
	auto ThisPlayerTeam = PlayerState->GetOwnerInfo().Team;
	// FogOfWarComponent->FogHoleMesh->SetWorldScale3D(UnitStruct.BaseSightRange * FVector(1, 1, 0)+FVector(0, 0, 17));

	auto ActorArray = TArray<AActor*>();
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), FogOfWarManagerClass, ActorArray);
	if (ActorArray.Num() == 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AUnitActor Error : \n\t Unable to find FogOfWarManager"));
		return;
	}
	const auto FogManager = Cast<AFogOfWarManager>(ActorArray[0]);
	if (!FogManager)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AUnitActor Error : \n\t Unable to cast FogOfWarManager"));
		return;
	}
	
	if (OwnerInfo.Team == ThisPlayerTeam && bUseForgOfWar)
	{
		FogOfWarComponent->SetVisibilityOfActorWithFog(true,true);
	}
	else if (OwnerInfo.Team != ThisPlayerTeam)
    {
		FogOfWarComponent->SetVisibilityOfActorWithFog(false,false);
		FogManager->EnemyActorsToShowOrHide.Add(this);
    }
}

void AUnitActor::SetTargetMulticast_Implementation(AActor* NewTarget, EUnitTargetType NewTargetType)
{
	SetTarget(TWeakObjectPtr<AActor>(NewTarget), NewTargetType);
}

// void AUnitActor::SetMovementLocallyMulticast_Implementation(bool bInMovementLocally)
// {
// 	bShouldDoMovementLocally = bInMovementLocally;
// }

void AUnitActor::ReplicatePathMulticast_Implementation(FVector NewLocation, const TArray<FVector> &NewPath)
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Unit %s RefreshPathMulticast"), *GetName()));
	if (HasAuthority()) return;
	SetActorLocation(NewLocation);
	Path = TArray<FVector>(NewPath);
}

void AUnitActor::ReplicateCalculatePathMulticast_Implementation(AFlux* FluxTarget, bool StartAtBeginning)
{
	if (this->HasAuthority()) return;
	//CreatePathArray(FluxTarget, StartAtBeginning); // Locally calculate pathfinding
	CopyPathFromFlux(TWeakObjectPtr<AFlux>(FluxTarget), StartAtBeginning); // Copy of the pathfinding from flux
}

void AUnitActor::SetReplicateMovementMulticast_Implementation(bool bInReplicateMovement)
{
	if (HasAuthority()) return;
	SetReplicatingMovement(bInReplicateMovement);
}

void AUnitActor::SetNiagaraActorMulticast_Implementation(TSubclassOf<ANiagaraUnitAsActor> NiagaraActorClass)
{
	if (bNiagaraActorInit) return;

	UClass* ClassType = NiagaraActorClass->GetDefaultObject()->GetClass();
	FActorSpawnParameters SpawnParams;
	const auto Transform = FTransform(FRotator::ZeroRotator, GetActorLocation(), FVector(1, 1, 1));
	TWeakObjectPtr<AActor> Actor = GetWorld()->SpawnActor(ClassType, &Transform, SpawnParams);
	NiagaraActor = Cast<ANiagaraUnitAsActor>(Actor);
	auto Color = UFunctionLibraryInfernale::GetOldTeamColorCpp(OwnerInfo.Team);
	NiagaraActor->GetNiagaraComponent()->SetColorParameter("Color", Color);
	
	FogOfWarComponent->NiagaraSystems.Add(NiagaraActor.Get());
	
	bNiagaraActorInit = true;
}

void AUnitActor::PreDestroyThisUnitMulticast_Implementation()
{
	UpdateFogOfWar(true);
	if (bNiagaraActorInit) NiagaraActor->Destroy();
	bIsGettingDestroyed = true;
}

void AUnitActor::OnDamagedMulticast_Implementation(float NewHealth, float MaxHealth, float DamageAmount)
{
	OnDamagedBP(NewHealth, MaxHealth, DamageAmount);
}

void AUnitActor::DestroyThisUnit()
{
	// Clean up everything, including the Niagara System
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Unit %s destroyed"), *GetName()));
	bIsGettingDestroyed = true;
	UnitActorDestroyed.Broadcast(this);
	PreDestroyThisUnitMulticast();
	Destroy();
}

void AUnitActor::MovementAI(float DeltaTime)
{
	//if (!bShouldDoMovementLocally && !HasAuthority()) return;
	
	if (bTargetIsValid) //&& HasAuthority())
	{
		AttackTarget(DeltaTime);
		return;
	}
	
	FollowPath(DeltaTime);
}

void AUnitActor::FollowPath(const float DeltaTime)
{
	if (bIsGettingDestroyed) return;
	const auto CurrentLocation = GetActorLocation();
	if (Path.Num() == 0)
	{
		if (HasAuthority() && !bTargetIsValid) DestroyThisUnit(); 
		return;
	}
	const auto TargetLocation = Path[0];
	const auto Direction = TargetLocation - CurrentLocation;
	const auto Distance = Direction.Size();

	if (Distance < UnitStruct.AcceptancePathfindingRadius)
	{
		CurrentSplineIndex++;
		Path.RemoveAt(0);
		return;
	}

	const auto DirectionNormalized = Direction.GetSafeNormal();
	auto Speed = UnitStruct.BaseSpeed;
	Speed = TransmutationComponent->GetEffectUnitSpeed(Speed);
	const auto NewLocation = CurrentLocation + DirectionNormalized * Speed * DeltaTime;
	const auto OldLocation = GetActorLocation();
	SetActorLocation(NewLocation);
	const auto NewRotation = DirectionNormalized.Rotation();
	SetActorRotation(NewRotation);
	UnitActorMoved.Broadcast(this, OldLocation);
}

void AUnitActor::AttackTarget(float DeltaTime)
{
	const auto CurrentLocation = GetActorLocation();
	//if (!HasAuthority())
	if (!TargetActor.IsValid()) return;
	const auto TargetLocation = TargetActor->GetActorLocation();
	const auto Direction = TargetLocation - CurrentLocation;
	const auto Distance = Direction.Size();
	const auto Targetable = Cast<IUnitTargetable>(TargetActor);
	if (!Targetable) return;
	const auto AttackAimRange = UnitStruct.AcceptanceRadiusAttack + Targetable->GetTargetableRange();

	if (Distance < AttackAimRange) return;

	const auto DirectionNormalized = Direction.GetSafeNormal();
	auto Speed = UnitStruct.BaseSpeed;
	Speed = TransmutationComponent->GetEffectUnitSpeed(Speed);
	const auto NewLocation = CurrentLocation + DirectionNormalized * Speed * DeltaTime;
	const auto OldLocation = GetActorLocation();
	SetActorLocation(NewLocation);
	const auto NewRotation = DirectionNormalized.Rotation();
	SetActorRotation(NewRotation);
	
	if (HasAuthority()) UnitActorMoved.Broadcast(this, OldLocation);
}


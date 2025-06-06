// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/ActorComponents/DamageableComponent.h"

#include "Interfaces/Ownable.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UDamageableComponent::UDamageableComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	CaptureByTeam = TArray<float>();
	CaptureByTeam.Init(0, 4);
	ResetCaptureByTeam();
	// ...
}

ETeam UDamageableComponent::GetTeamByIndex(int Index) const
{
	switch (Index)
	{
		case 0: return ETeam::Team1;
		case 1: return ETeam::Team2;
		case 2: return ETeam::Team3;
		case 3: return ETeam::Team4;
		default: return ETeam::NatureTeam;
	}
}

void UDamageableComponent::DamageTaken()
{
	IsHealing = false;
	CurrentDelaySinceLastAttack = 0.f;
	CurrentDelaySinceLastHeal = 0.f;
}

// Called when the game starts
void UDamageableComponent::BeginPlay()
{
	Super::BeginPlay();
	HealthDepeltedOwner.AddDynamic(this, &UDamageableComponent::OnHealthDepletedOwner);
	HealthDepeltedActor.AddDynamic(this, &UDamageableComponent::OnHealthDepletedActor);

	SetComponentTickEnabled(false);
}

void UDamageableComponent::OnHealthDepletedOwner(AActor* Actor, FOwner Depleter)
{
	HealthDepelted.Broadcast();
}

void UDamageableComponent::OnHealthDepletedActor(AActor* Actor, AActor* Depleter)
{
	HealthDepelted.Broadcast();
}


// Called every frame
void UDamageableComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!AllowedToHeal) return;

	if (IsHealing)
	{
		CurrentDelaySinceLastHeal += DeltaTime;
		if (CurrentDelaySinceLastHeal < HealingDelay) return;
		CurrentDelaySinceLastHeal = 0.f;

		if (bCaptureMode)
		{
			for (int i = 0; i < CaptureByTeam.Num(); i++)
			{
				if (CaptureByTeam[i] == 0) continue;
				CaptureByTeam[i] -= Healing;
				CaptureByTeam[i] = FMath::Clamp(CaptureByTeam[i], 0.f, MaxHealth);
				const auto Percent = CaptureByTeam[i] / MaxHealth;
				const auto Team = GetTeamByIndex(i);
				
				if (CaptureByTeam[i] <= 0) CaptureCancelled.Broadcast();
				else CaptureDamaged.Broadcast(Team, Percent);
			}
			return;
		}
		
		Health += Healing;
		Health = FMath::Clamp(Health, 0.f, MaxHealth);
		if (Health == MaxHealth) HealthFullyHealed.Broadcast();
		else Damaged.Broadcast(GetOwner(), Health, Healing);
		
		return;
	}

	CurrentDelaySinceLastAttack += DeltaTime;
	if (CurrentDelaySinceLastAttack < HealingDelaySinceLastAttack) return;
	CurrentDelaySinceLastAttack = 0.f;
	IsHealing = true;
	CurrentDelaySinceLastHeal = HealingDelay;
}

float UDamageableComponent::GetCaptureHealthByTeam(const ETeam Team) const
{
	const auto TeamIndex = GetIndexByTeam(Team);
	if (TeamIndex == -1) return 0;
	return CaptureByTeam[TeamIndex];
}

ETeam UDamageableComponent::GetCaptureTeam() const
{
	for (int i = 0; i < 4; i++)
	{
		if (CaptureByTeam[i] == 0) continue;
		return GetTeamByIndex(i);
	}
	return ETeam::NatureTeam;
}

bool UDamageableComponent::IsCaptureMode() const
{
	return bCaptureMode;
}

float UDamageableComponent::GetHealth() const
{
	return Health;
}

float UDamageableComponent::GetMaxHealth() const
{
	return MaxHealth;
}

void UDamageableComponent::SetMaxHealthKeepPercent(const float NewMaxHealth)
{
	const auto Percent = Health / MaxHealth;
	const auto OldMax = MaxHealth;
	MaxHealth = NewMaxHealth;
	Health = MaxHealth * Percent;

	if (!bCaptureMode) return;

	for (int i = 0; i < 4; i++)
	{
		if (CaptureByTeam[i] == 0) continue;
		CaptureByTeam[i] = (CaptureByTeam[i] / OldMax) * MaxHealth;
	}
}

void UDamageableComponent::SetMaxHealth(const float NewMaxHealth, const bool bHeal, const bool bHealToMax, const bool bHealPercent,
                                        const float NewHealth)
{
	float Mult = 1;
	if (MaxHealth != 0 ) Mult = NewHealth / MaxHealth;
	MaxHealth = NewMaxHealth;

	if (!bHeal) return;

	if (bHealToMax)
	{
		Health = MaxHealth;
		return;
	}

	if (bHealPercent)
	{
		Health *= Mult;
		return;
	}

	Health = NewHealth;
}

float UDamageableComponent::HealHealth(const float HealAmount, const bool bHealPercent)
{
	auto InitialHealth = Health;
	Health += HealAmount * bHealPercent ? MaxHealth : 1;
	Health = FMath::Clamp(Health, 0.f, MaxHealth);

	auto HealedAmount = Health - InitialHealth;
	return HealedAmount;
}

float UDamageableComponent::DamageHealth(const float DamageAmount, const bool bDamagePercent)
{
	if (bCaptureMode)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("Error: DamageHealth IN CAPTURE MODE"));
		return 0;
	}
	
	const auto DamagedAmount = LocalDamage(DamageAmount, bDamagePercent);

	Damaged.Broadcast(GetOwner(), Health, DamagedAmount);
	if (Health <= 0) HealthDepelted.Broadcast();
	
	return DamagedAmount;
}

float UDamageableComponent::DamageHealthActor(const float DamageAmount, const bool bDamagePercent,
	TWeakObjectPtr<AActor> DamageCauser)
{
	if (bCaptureMode)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("Error: DamageHealthActor IN CAPTURE MODE"));
		return 0;
	}
	const auto DamagedAmount = LocalDamage(DamageAmount, bDamagePercent);

	Damaged.Broadcast(GetOwner(), Health, DamagedAmount);
	if (Health <= 0) HealthDepeltedActor.Broadcast(GetOwner(), DamageCauser.Get());
	
	return DamagedAmount;
}

float UDamageableComponent::DamageHealthOwner(const float DamageAmount, const bool bDamagePercent, const FOwner DamageOwner)
{
	if (DamageAmount > 0) DamageTaken();
	if (bCaptureMode)
	{
		CaptureDamage(DamageAmount, DamageOwner);
		return DamageAmount;
	}
	
	const auto DamagedAmount = LocalDamage(DamageAmount, bDamagePercent);

	Damaged.Broadcast(GetOwner(), Health, DamagedAmount);
	if (Health <= 0) HealthDepeltedOwner.Broadcast(GetOwner(), DamageOwner);
	
	return DamagedAmount;
}

void UDamageableComponent::ResetCaptureByTeam()
{
	CaptureByTeam[0] = 0;
	CaptureByTeam[1] = 0;
	CaptureByTeam[2] = 0;
	CaptureByTeam[3] = 0;
}

void UDamageableComponent::CaptureDamage(const float DamageAmount, const FOwner DamageOwner)
{
	if (!bCaptureMode) return;
	const auto TeamIndex = GetIndexByTeam(DamageOwner.Team);
	if (TeamIndex == -1) return;

	float DamageRemaining = DamageAmount;
	float Percent;
	
	for (int i = 0; i < 4; i++)
	{
		if (CaptureByTeam[i] == 0) continue;
		if (i == TeamIndex) continue;

		if (CaptureByTeam[i] >= DamageRemaining)
		{
			CaptureByTeam[i] -= DamageRemaining;
			Percent = CaptureByTeam[i] / MaxHealth;
			Percent = FMath::Clamp(Percent, 0.f, 1.f);
			CaptureDamaged.Broadcast(GetTeamByIndex(i), Percent);
			return;
		}

		DamageRemaining -= CaptureByTeam[i];
		CaptureByTeam[i] = 0;
	}

	IOwnable* OwnerActor = Cast<IOwnable>(GetOwner());
	if (OwnerActor == nullptr) return;
	auto OwnerTeam = OwnerActor->GetOwner().Team;
	if (OwnerTeam == DamageOwner.Team)
	{
		CaptureByTeam[TeamIndex] = 0;
		CaptureDamaged.Broadcast(OwnerTeam, 0);
		return;
	}
	
	CaptureByTeam[TeamIndex] += DamageRemaining;
	Percent = CaptureByTeam[TeamIndex] / MaxHealth;
	Percent = FMath::Clamp(Percent, 0.f, 1.f);
	CaptureDamaged.Broadcast(DamageOwner.Team, Percent);

	if (CaptureByTeam[TeamIndex] >= MaxHealth)
	{
		CaptureCompleted.Broadcast(DamageOwner);
		ResetCaptureByTeam();
	}
}

void UDamageableComponent::SetCaptureMode(const bool bNewCaptureMode)
{
	bCaptureMode = bNewCaptureMode;
}

void UDamageableComponent::SetHealingAllowed(const bool bNewAllowedToHeal, const float NewHealing,
	const float NewHealingDelay, const float NewHealingDelaySinceLastAttack)
{
	AllowedToHeal = bNewAllowedToHeal;
	Healing = NewHealing;
	HealingDelay = NewHealingDelay;
	HealingDelaySinceLastAttack = NewHealingDelaySinceLastAttack;
	CurrentDelaySinceLastAttack = 0.f;

	SetComponentTickEnabled(AllowedToHeal);
}

float UDamageableComponent::LocalDamage(const float DamageAmount, const bool bDamagePercent)
{
	auto InitialHealth = Health;
	Health -= DamageAmount * (bDamagePercent ? MaxHealth : 1);
	Health = FMath::Clamp(Health, 0.f, MaxHealth);

	auto DamagedAmount = InitialHealth - Health;
	return DamageAmount;
}

int UDamageableComponent::GetIndexByTeam(const ETeam Team) const
{
	switch (Team)
	{
		case ETeam::NatureTeam: return -1;
		case ETeam::Team1: return 0;
		case ETeam::Team2: return 1;
		case ETeam::Team3: return 2;
		case ETeam::Team4: return 3;
		default: return -1;
	}
}




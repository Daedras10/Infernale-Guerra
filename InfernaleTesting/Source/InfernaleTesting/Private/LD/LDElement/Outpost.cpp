// Fill out your copyright notice in the Description page of Project Settings.


#include "LD/LDElement/Outpost.h"

#include "Component/ActorComponents/EffectAfterDelayComponent.h"
#include "DataAsset/OutpostDataAsset.h"

AOutpost::AOutpost()
{
	PrimaryActorTick.bCanEverTick = true;
	EffectAfterDelayComponent = CreateDefaultSubobject<UEffectAfterDelayComponent>(TEXT("EffectAfterDelayComponent"));
	LDElementType = ELDElementType::LDElementOutpostType;
}

void AOutpost::BeginPlay()
{
	Super::BeginPlay();
	SyncDataAsset();
	
	if (!HasAuthority()) return;
	
	//GameModeStart waiting then start charging
}

void AOutpost::SyncDataAsset()
{
	if (!bUseDataAsset) return;
	if (!OutpostDataAsset)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("Outpost Error : \n\t No Outpost Data Asset"));
		return;
	}
	
	ChargeTime = OutpostDataAsset->ChargeTime;
	DechargeTime = OutpostDataAsset->DechargeTime;
	Radius = OutpostDataAsset->Radius;
	//OutpostBuffs = OutpostDataAsset->OutpostBuffs;
	
}

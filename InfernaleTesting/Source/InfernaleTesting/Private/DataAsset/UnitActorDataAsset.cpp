// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAsset/UnitActorDataAsset.h"

FUnitStruct::FUnitStruct(): BaseHealth(0), BaseDamage(0), BaseAttackCD(0), BaseRange(0), BaseSpeed(0), BaseRushSpeed(0),
                            BaseDetectionRange(0),
                            BaseDetectionAngle(0),
                            BaseSightRange(0),
                            BaseSightAngle(0),
                            BaseSightType(),
                            NiagaraHeightOffset(0),
							NiagaraRotationOffset(FVector(0.f, 0.f, 0.f))
{
}

UUnitActorDataAsset::UUnitActorDataAsset()
{
}

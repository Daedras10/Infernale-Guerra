// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "Enums/Enums.h"
#include "MassClientFragments.generated.h"


struct FMassExecutionContext;

USTRUCT()
struct FClientStateFragment : public FMassFragment
{
	GENERATED_USTRUCT_BODY()
	
	/*
	 * { Context.Defer().AddTag<FAmalgamStateChangeTag>(Context.GetEntity(EntityIndexInContext)); }
	 * */

public:
	EAmalgamState GetState() const;
	void SetState(EAmalgamState InState);
	void SetStateAndNotify(EAmalgamState InState, FMassExecutionContext& Context, int32 EntityIndexInContext);

	EAmalgamAggro GetAggro() const;
	void SetAggro(EAmalgamAggro InAggro);

	void Kill(FMassExecutionContext& Context, int32 EntityIndexInContext);

protected:
	void NotifyContext(FMassExecutionContext& Context, int32 EntityIndexInContext);

protected:
	EAmalgamState State = EAmalgamState::Inactive;
	EAmalgamAggro AggroState = EAmalgamAggro::NoAggro;
};
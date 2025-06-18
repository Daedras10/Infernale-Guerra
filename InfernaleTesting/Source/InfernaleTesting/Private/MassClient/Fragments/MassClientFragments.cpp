// Fill out your copyright notice in the Description page of Project Settings.


#include "MassClient/Fragments/MassClientFragments.h"

EAmalgamState FClientStateFragment::GetState() const
{
	return State;
}

void FClientStateFragment::SetState(EAmalgamState InState)
{
	State = InState;
}

void FClientStateFragment::SetStateAndNotify(EAmalgamState InState, FMassExecutionContext& Context,
	int32 EntityIndexInContext)
{
	if (State == EAmalgamState::Killed) return;

	State = InState; 
	NotifyContext(Context, EntityIndexInContext); 
}

EAmalgamAggro FClientStateFragment::GetAggro() const
{
	return AggroState;
}

void FClientStateFragment::SetAggro(EAmalgamAggro InAggro)
{
	AggroState = InAggro;
}

void FClientStateFragment::Kill(FMassExecutionContext& Context, int32 EntityIndexInContext)
{
	//TODO ?
	//SetStateAndNotify
}

void FClientStateFragment::NotifyContext(FMassExecutionContext& Context, int32 EntityIndexInContext)
{
	//TODO: ?
}

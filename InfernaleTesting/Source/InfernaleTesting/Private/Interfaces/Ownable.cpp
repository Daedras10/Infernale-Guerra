// Fill out your copyright notice in the Description page of Project Settings.


#include "Interfaces/Ownable.h"
#include "Structs/SimpleStructs.h"

FOwner IOwnable::GetOwner()
{
	return FOwner();
}

void IOwnable::SetOwner(FOwner NewOwner)
{
}

void IOwnable::ChangeOwner(FOwner NewOwner)
{
	SetOwner(NewOwner);
}
// Fill out your copyright notice in the Description page of Project Settings.


#include "Interfaces/Damageable.h"

#include "Interfaces/Ownable.h"

// Add default functionality here for any IIDamageable functions that are not pure virtual.
float IDamageable::DamageHealthOwner(const float DamageAmount, const bool bDamagePercent, const FOwner DamageOwner)
{
	return 0;
}

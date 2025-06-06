// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Damageable.generated.h"

struct FOwner;
// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UDamageable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class INFERNALETESTING_API IDamageable
{
	GENERATED_BODY()

public:
	virtual float DamageHealthOwner(const float DamageAmount, const bool bDamagePercent, const FOwner DamageOwner);

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
};

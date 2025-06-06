// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Interactable.h"
#include "LDElement.generated.h"

class UGameSettingsDataAsset;
enum class EEntityType : uint8;
enum class ETeam : uint8;

UENUM(Blueprintable)
enum class ELDElementType : uint8
{
	LDElementNoneType,
	LDElementOutpostType,
	LDElementSoulBeaconType,
	LDElementNeutralCampType,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FElementDelegate, ALDElement*, Element);

UCLASS()
class INFERNALETESTING_API ALDElement : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALDElement();

	ELDElementType GetLDElementType() const;

	virtual void Tick(float DeltaTime) override;
	virtual bool IsAttackableBy(ETeam Team);
	virtual EEntityType GetEntityType() const;

	virtual void InteractStartHover(APlayerControllerInfernale* Interactor) override;
	virtual void InteractEndHover(APlayerControllerInfernale* Interactor) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void RegisterLDElement();

public:
	FElementDelegate LDElementRemoved;
	UPROPERTY(BlueprintAssignable) FPCBoolDelegate Hovered;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UGameSettingsDataAsset* GameSettings;

protected:
	ELDElementType LDElementType = ELDElementType::LDElementNoneType;

};

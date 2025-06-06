// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Component/PlayerController/PlayerControllerComponent.h"
#include "InteractionComponent.generated.h"

class IInteractable;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInteracting, UInteractionComponent*, InteractionComponent, bool,
                                             bIsClicked);

/**
 * 
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class INFERNALETESTING_API UInteractionComponent : public UPlayerControllerComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void AskEndMain();

	void EmptyMainInteractable();
	void ForceSetMainInteract(IInteractable* Interactable);
	void ForceStartMainInteract(IInteractable* Interactable, bool ForceFlux);
	void ForceEndMainInteractIf(IInteractable* Interactable);
	
	UFUNCTION(BlueprintCallable, BlueprintPure) bool CanCanCloseWithRightClick();
	UFUNCTION(BlueprintCallable) void SetCanCloseWithRightClick(bool bValue);
	UFUNCTION(BlueprintCallable) void SetAllowedToInteract(bool bValue);
	UFUNCTION(BlueprintCallable) AActor* GetMainInteractActor() const;


protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UFUNCTION() void TryInteractStartMain();
	UFUNCTION() void EndMain();
	UFUNCTION() void EndMainInteractBySecondary();
	UFUNCTION() void EndMainInteract();
	UFUNCTION() void TryInteractStartSecondary();
	UFUNCTION() void EndSecondary();
	UFUNCTION() void EndSecondaryInteract();

	void StartMainInteract(IInteractable* Interactable);
	void StartSecondaryInteract(IInteractable* Interactable);
	void StartHoverInteract(IInteractable* Interactable);
	void EndHoverInteract();

	bool GetHitResult(FHitResult& HitResult, const ECollisionChannel Channel);

	UFUNCTION() void OnPawnMoved(const FVector Location);
	UFUNCTION() void OnZoom(const float Value);
	UFUNCTION() void OnFluxModeChanged(const bool bIsFluxMode);
	UFUNCTION() void OnPlayerMovement();
	UFUNCTION() void OnPlayerMovementVector2D(const FVector2D MovementVector);
	UFUNCTION() void OnPlayerMovementFloat(const float Value);
	UFUNCTION() void OnTransmutationModeChanged(const bool bValue);

public:
	bool isNewCycle = false;
	
protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Debug Settings")
	bool bDebugInteractions = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Debug Settings")
	bool bDebugHover = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Debug Settings")
	bool bDebugRayCasts = false;
	
	bool bMainIsClicked = false;
	bool bSecondaryIsClicked = false;
	bool bAllowedToInteract = false;

	bool CanCloseWithRightClick = true;

	IInteractable* MainInteractable = nullptr;
	IInteractable* SecondaryInteractable = nullptr;
	IInteractable* HoverInteractable = nullptr;
	
	FInteracting InteractMain;
	FInteracting InteractSecondary;
	FInteracting InteractHover;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraActor.h"
#include "Components/ActorComponent.h"
#include "FogOfWarComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class INFERNALETESTING_API UFogOfWarComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UFogOfWarComponent();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FogOfWar")
	UStaticMeshComponent* FogHoleMesh;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "FogOfWar")
	void SetVisibilityOfActorWithFog(bool FogHoleMeshVisibility, bool ActorVisibility);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FogOfWar")
	TArray<UStaticMeshComponent*> ActorMeshes;

	UPROPERTY( BlueprintReadOnly, EditAnywhere, Category = "FogOfWar")
	TArray<ANiagaraActor*> NiagaraSystems;
};

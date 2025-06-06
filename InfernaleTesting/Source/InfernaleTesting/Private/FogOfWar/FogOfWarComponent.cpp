// Fill out your copyright notice in the Description page of Project Settings.


#include "FogOfWar/FogOfWarComponent.h"

#include "NiagaraComponent.h"


// Sets default values for this component's properties
UFogOfWarComponent::UFogOfWarComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UFogOfWarComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UFogOfWarComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UFogOfWarComponent::SetVisibilityOfActorWithFog(bool FogHoleMeshVisibility, bool ActorVisibility)
{
	if (FogHoleMesh)
	{
		FogHoleMesh->SetVisibility(!FogHoleMeshVisibility);
		// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("FogHoleMeshVisibility: %d"), FogHoleMeshVisibility));
	}
	for (auto ActorMesh : ActorMeshes)
	{
		if (ActorMesh)
		{
			ActorMesh->SetVisibility(ActorVisibility);
			
		}
	}
	for (auto NiagaraSystem : NiagaraSystems)
    {
        if (NiagaraSystem)
        {
            NiagaraSystem->GetNiagaraComponent()->SetVisibility(ActorVisibility);
        }
    }
	
}


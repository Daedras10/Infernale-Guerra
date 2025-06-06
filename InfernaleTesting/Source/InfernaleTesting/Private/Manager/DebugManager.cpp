// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/DebugManager.h"

#include "Kismet/GameplayStatics.h"
#include "LD/Breach.h"

// Sets default values
ADebugManager::ADebugManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ADebugManager::BeginPlay()
{
	Super::BeginPlay();
	
}

void ADebugManager::DebugBreach()
{
}

void ADebugManager::DebugLonelyBreach()
{
	TArray<AActor*> FoundActors = TArray<AActor*>();
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), BreachClass, FoundActors);
	bool bFound = false;

	for (auto Actor : FoundActors)
    {
        auto Breach = Cast<ABreach>(Actor);
        if (!Breach) continue;
		if (Breach->GetMainBuilding()) continue;
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("Lonely Breach: %s"), *Breach->GetName()));
		auto Start = Breach->GetActorLocation();
		auto End = Start + FVector(0, 0, 2000);
		DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 15.f, 0, 5.f);
		bFound = true;
    }
	if (!bFound)
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, TEXT("No lonely Breach found"));
        return;
    }
}

// Called every frame
void ADebugManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


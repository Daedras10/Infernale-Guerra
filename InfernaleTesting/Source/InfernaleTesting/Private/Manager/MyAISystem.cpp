// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/MyAISystem.h"

#include "EnvironmentQuery/EnvQueryManager.h"

void UMyAISystem::InitializeActorsForPlay(bool bTimeGotReset)
{
	Super::InitializeActorsForPlay(bTimeGotReset);

	UE_LOG(LogTemp, Warning, TEXT("[ok] MyAISystem initialized on world %s (NetMode: %d)"),
		*GetWorld()->GetName(),
		(int32)GetWorld()->GetNetMode());

	if (UEnvQueryManager::GetCurrent(GetWorld()))
	{
		UE_LOG(LogTemp, Warning, TEXT("✅ EQS Manager exists!"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ EQS Manager NOT created!"));
	}

}

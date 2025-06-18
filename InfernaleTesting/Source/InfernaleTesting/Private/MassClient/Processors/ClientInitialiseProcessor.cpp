// Fill out your copyright notice in the Description page of Project Settings.


#include "MassClient/Processors/ClientInitialiseProcessor.h"

#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "Manager/AmalgamVisualisationManager.h"
#include "Mass/Army/AmalgamFragments.h"
#include "MassClient/Fragments/MassClientFragments.h"
#include "MassClient/Tags/ClientTags.h"

UClientInitialiseProcessor::UClientInitialiseProcessor(): VisualisationManager(nullptr)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Client);
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Avoidance);

	bAutoRegisterWithProcessingPhases = true;
	bRequiresGameThreadExecution = true; /* Required for Niagara system spawn (if it's still done there) */
}

void UClientInitialiseProcessor::ConfigureQueries()
{
	/* Tags requiered */
	EntityQuery.AddTagRequirement<FAmalgamClientTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FAmalgamClientInitializeTag>(EMassFragmentPresence::All);

	
	/* Tags to remove */
	EntityQuery.AddTagRequirement<FAmalgamServerTag>(EMassFragmentPresence::None);


	/* Fragments */
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamMovementFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamFluxFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamOwnerFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamTransmutationFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FClientStateFragment>(EMassFragmentAccess::ReadWrite);

	
	// EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	// EntityQuery.AddRequirement<FClientStateFragment>(EMassFragmentAccess::ReadWrite);
	// EntityQuery.AddRequirement<FAmalgamGridFragment>(EMassFragmentAccess::ReadWrite);
	// EntityQuery.AddRequirement<FAmalgamFluxFragment>(EMassFragmentAccess::ReadWrite);
	// EntityQuery.AddRequirement<FAmalgamOwnerFragment>(EMassFragmentAccess::ReadWrite);
	//
	// EntityQuery.AddRequirement<FAmalgamMovementFragment>(EMassFragmentAccess::ReadWrite);
	// EntityQuery.AddRequirement<FAmalgamAggroFragment>(EMassFragmentAccess::ReadWrite);
	// EntityQuery.AddRequirement<FAmalgamFightFragment>(EMassFragmentAccess::ReadWrite);
	// EntityQuery.AddRequirement<FAmalgamNiagaraFragment>(EMassFragmentAccess::ReadWrite);
	//
	// EntityQuery.AddRequirement<FAmalgamTransmutationFragment>(EMassFragmentAccess::ReadWrite);
	// EntityQuery.AddRequirement<FAmalgamSightFragment>(EMassFragmentAccess::ReadWrite);
	//
	// EntityQuery.AddTagRequirement<FAmalgamInitializeTag>(EMassFragmentPresence::All);

	EntityQuery.RegisterWithProcessor(*this);
}

void UClientInitialiseProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("ClientInitializeProcessor : Executing0"));
	if (!VisualisationManager)
	{
		TArray<AActor*> OutActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAmalgamVisualisationManager::StaticClass(), OutActors);

		if (OutActors.Num() > 0) VisualisationManager = static_cast<AAmalgamVisualisationManager*>(OutActors[0]);
		else VisualisationManager = nullptr;

		if (!VisualisationManager)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("ClientInitializeProcessor : Unable to find visualisation manager, skipping execution."));
			return;
		}
	}
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("ClientInitializeProcessor : Executing"));
	if (VisualisationManager->IsGameIsEnding())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("ClientInitializeProcessor : Game is ending, skipping execution."));
		return;
	}
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("ClientInitializeProcessor : Executing2"));

	FColor DebugColor = VisualisationManager->HasAuthority() ? FColor::Purple : FColor::Emerald;

	EntityQuery.ForEachEntityChunk(EntityManager, Context, [&](FMassExecutionContext& LocalContext)
	{
		TArrayView<FTransformFragment> TransformView = LocalContext.GetMutableFragmentView<FTransformFragment>();
		TArrayView<FClientStateFragment> StateFragView = LocalContext.GetMutableFragmentView<FClientStateFragment>();
		TArrayView<FAmalgamFluxFragment> FluxFragView = LocalContext.GetMutableFragmentView<FAmalgamFluxFragment>();
		TArrayView<FAmalgamOwnerFragment> OwnerFragView = LocalContext.GetMutableFragmentView<FAmalgamOwnerFragment>();
		TArrayView<FAmalgamTransmutationFragment> TranstmFragView = LocalContext.GetMutableFragmentView<FAmalgamTransmutationFragment>();

		for (int32 Index = 0; Index < LocalContext.GetNumEntities(); ++Index)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 15.f, DebugColor, FString::Printf(TEXT("ClientInitializeProcessor : Processing Entity")));
			//UE_LOG(LogTemp, Warning, TEXT("ClientInitializeProcessor : Processing Entity %d on %s"), Index, VisualisationManager->HasAuthority() ? TEXT("Server") : TEXT("Client"));
			if (VisualisationManager->ClientInitializeInfosIsEmpty())
			{
				GEngine->AddOnScreenDebugMessage(-1, 15.f, DebugColor, TEXT("ClientInitializeProcessor : No ClientInitializeInfos available, skipping execution."));
				UE_LOG(LogTemp, Warning, TEXT("ClientInitializeProcessor : No ClientInitializeInfos available, skipping execution (on %s)."), VisualisationManager->HasAuthority() ? TEXT("Server") : TEXT("Client"));
				return;
			}

			const auto FirstInfo = VisualisationManager->GetFirstClientInitializeInfosAndRemoveit();
			const auto ElementInfo = VisualisationManager->CreateVisualUnitClient(FirstInfo);

			const auto Entity = LocalContext.GetEntity(Index);
			const auto Handle = ElementInfo.Handle;
			VisualisationManager->AddClientHandleTo(Handle, Entity.AsNumber());

			GEngine->AddOnScreenDebugMessage(-1, 2.5f, DebugColor, FString::Printf(TEXT("ClientInitializeProcessor : Initializing Amalgam")));
			
			FTransform& EntityTransform = TransformView[Index].GetMutableTransform();
			FClientStateFragment& StateFragment = StateFragView[Index];
			FAmalgamFluxFragment& FluxFragment = FluxFragView[Index];
			FAmalgamOwnerFragment& OwnerFragment = OwnerFragView[Index];
			FAmalgamTransmutationFragment& TransmutationFragment = TranstmFragView[Index];

			const auto Flux = FirstInfo.Flux;
			if (!Flux->IsValidLowLevel())
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("ClientInitializeProcessor : No Flux Found"));
				LocalContext.Defer().SwapTags<FAmalgamClientInitializeTag, FAmalgamKillTag>(Entity);
				continue;
			}

			TransmutationFragment.RefreshTransmutationComponent(GetWorld(), FirstInfo.EntityOwner.Player); //TODO: might not be possible
			OwnerFragment.SetOwner(FirstInfo.EntityOwner);
			EntityTransform.SetLocation(FirstInfo.Location);

			FluxFragment.SetFlux(Flux);
			FluxFragment.SetSplinePointIndex(0);
			
			StateFragment.SetState(EAmalgamState::FollowPath);
			StateFragment.SetAggro(EAmalgamAggro::NoAggro);

			
			/* Remove tags */
			LocalContext.Defer().RemoveTag<FAmalgamClientInitializeTag>(Entity);
		}
	});
}

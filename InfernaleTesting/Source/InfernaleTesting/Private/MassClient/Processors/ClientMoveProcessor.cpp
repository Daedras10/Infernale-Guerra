// Fill out your copyright notice in the Description page of Project Settings.


#include "MassClient/Processors/ClientMoveProcessor.h"

#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "Kismet/GameplayStatics.h"
#include "Manager/AmalgamVisualisationManager.h"
#include "Mass/Army/AmalgamFragments.h"
#include "Mass/Army/AmalgamTags.h"
#include "MassClient/Fragments/MassClientFragments.h"
#include "MassClient/Tags/ClientTags.h"


UClientMoveProcessor::UClientMoveProcessor(): VisualisationManager(nullptr)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Client);
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Avoidance);

	bAutoRegisterWithProcessingPhases = true;
	bRequiresGameThreadExecution = true; // Check if required, likely yes since we'll be doing movement
}

void UClientMoveProcessor::ConfigureQueries()
{
	/* Tags requiered */
	EntityQuery.AddTagRequirement<FAmalgamClientTag>(EMassFragmentPresence::All);
	
	/* Tags to remove */
	EntityQuery.AddTagRequirement<FAmalgamServerTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FAmalgamClientInitializeTag>(EMassFragmentPresence::None);

	/* Fragments */
	//TODO: Check if they are all required
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FClientStateFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamFluxFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamTransmutationFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamPathfindingFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamDirectionFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamMovementFragment>(EMassFragmentAccess::ReadWrite);

	/* Might not be used */
	EntityQuery.AddRequirement<FAmalgamOwnerFragment>(EMassFragmentAccess::ReadWrite);

	
}

void UClientMoveProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	if (!VisualisationManager)
	{
		TArray<AActor*> OutActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAmalgamVisualisationManager::StaticClass(), OutActors);

		if (OutActors.Num() > 0) VisualisationManager = static_cast<AAmalgamVisualisationManager*>(OutActors[0]);
		else VisualisationManager = nullptr;
		if (!VisualisationManager) return;
	}
	if (VisualisationManager->IsGameIsEnding()) return;
	
	//TODO:
	// Check Player still present for the TransmutationComponent refs
	// Will likely be a Map with EPlayerOwning and bool: isStillPresent
	

	EntityQuery.ForEachEntityChunk(EntityManager, Context, [&](FMassExecutionContext& LocalContext)
	{
		UE_LOG(LogTemp, Warning, TEXT("ClientMoveProcessor : Executing for entities %d on %s"), LocalContext.GetNumEntities(), VisualisationManager->HasAuthority() ? TEXT("Server") : TEXT("Client"));

		TArrayView<FTransformFragment> TransformView = LocalContext.GetMutableFragmentView<FTransformFragment>();
		TArrayView<FClientStateFragment> StateFragView = LocalContext.GetMutableFragmentView<FClientStateFragment>();
		TArrayView<FAmalgamFluxFragment> FluxFragView = LocalContext.GetMutableFragmentView<FAmalgamFluxFragment>();
		TArrayView<FAmalgamTransmutationFragment> TransmFragView = LocalContext.GetMutableFragmentView<FAmalgamTransmutationFragment>();
		TArrayView<FAmalgamPathfindingFragment> PathFragView = Context.GetMutableFragmentView<FAmalgamPathfindingFragment>();
		TArrayView<FAmalgamDirectionFragment> DirectionFragView = Context.GetMutableFragmentView<FAmalgamDirectionFragment>();
		TArrayView<FAmalgamTargetFragment> TargetFragView = Context.GetMutableFragmentView<FAmalgamTargetFragment>();
		TArrayView<FAmalgamMovementFragment> MovementFragView = LocalContext.GetMutableFragmentView<FAmalgamMovementFragment>();


		/* Variables to give to PCs */
		const float WorldDeltaTime = Context.GetDeltaTimeSeconds();
		TArray<FVector> UpdatedLocations;
		TArray<FVector> UpdatedRotations;
		TArray<FHandleBool> EntityHandlesStateUpdate = TArray<FHandleBool>();
		
		for (int32 Index = 0; Index < LocalContext.GetNumEntities(); ++Index)
		{
			FTransformFragment& TransformFragment = TransformView[Index];
			FTransform& EntityTransform = TransformFragment.GetMutableTransform();
			FClientStateFragment& StateFragment = StateFragView[Index];
			FAmalgamFluxFragment& FluxFragment = FluxFragView[Index];
			FAmalgamPathfindingFragment& PathFragment = PathFragView[Index];
			FAmalgamDirectionFragment& DirectionFragment = DirectionFragView[Index];
			FAmalgamTargetFragment& TargetFragment = TargetFragView[Index];
			FAmalgamTransmutationFragment& TransFragment = TransmFragView[Index];
			FAmalgamMovementFragment* MovementFragment = &MovementFragView[Index];

			const auto Location = EntityTransform.GetLocation();
			const auto State = StateFragment.GetState();
			const auto Flux = FluxFragment.GetFlux();
			const auto FluxValid = Flux.IsValid();
			bool bSucceeded	= true; //TODO : recheck if this is needed

			if (!FluxValid && !PathFragment.IsPathFinal()) PathFragment.MakePathFinal();

			if (FluxValid)
			{
				const auto LocalVersion = PathFragment.GetUpdateVersion();
				const bool FluxVersionIsOk = LocalVersion == Flux->GetUpdateVersion();

				if (LocalVersion != -1 && !FluxVersionIsOk) PathFragment.MakePathFinal();
				if (!PathFragment.IsPathFinal())
				{
					if (PathFragment.GetUpdateID() != Flux->GetUpdateID()) PathFragment.CopyPathFromFlux(Flux, Location, false);
					if (PathFragment.ShouldRecover()) PathFragment.RecoverPath(Location);
				}
			}
			
			FVector TargetLocation;
			
			switch (State)
			{
			case EAmalgamState::FollowPath:
				bSucceeded = FollowPath(TransformFragment, FluxFragment, PathFragment, DirectionFragment, TransFragment.GetSpeedModifier(MovementFragment->GetSpeed()), WorldDeltaTime);
				break;
			case EAmalgamState::Aggroed:
				// TargetLocation = GetTargetLocation(TargetFragment);
				// float TargetRangeOffset = TargetFragment.GetTargetRangeOffset(StateFragment.GetAggro());
				//float EntityRangeOffset = AggroFragment.GetFightRange();
				// float TotalRangeOffset = TargetRangeOffset + EntityRangeOffset;
				// TargetFragment.SetTotalRangeOffset(TotalRangeOffset);
				//
				// if (TargetLocation == FVector::ZeroVector)
				// {
				// 	StateFragment.SetAggro(EAmalgamAggro::NoAggro);
				// 	StateFragment.SetStateAndNotify(EAmalgamState::FollowPath, Context, Index);
				// 	continue;
				// }
				// if ((Location - TargetLocation).Length() - TotalRangeOffset < AggroFragment.GetFightRange())
				// {
				// 	StateFragment.SetStateAndNotify(EAmalgamState::Fighting, Context, Index);
				// 	EntityHandlesStateUpdate.Add(FHandleBool(Context.GetEntity(Index), true));
				// 	continue;
				// }
				// bSucceeded = FollowTarget(TransformFragment, TargetLocation, DirectionFragment, TransFragment.GetSpeedModifier(MovementFragment->GetRushSpeed()), PathFragment.GetAcceptanceAttackRadius(), WorldDeltaTime);
				break;
			case EAmalgamState::Fighting:
				break;
			case EAmalgamState::Killed:
				break;
			default:
				bSucceeded = false;
				break;
			}

			if (!bSucceeded)
			{
				//Kill locally and wait for server to update ?
				continue;
			}

			UpdatedLocations.Add(TransformFragment.GetTransform().GetLocation());
			UpdatedRotations.Add(DirectionFragment.Direction);
			
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("ClientMoveProcessor : Executing for entities %d"), LocalContext.GetNumEntities()));
			//DrawDebugSphere(GetWorld(), LocalContext.GetFragment<FTransformFragment>(Index).GetTransform().GetLocation(), 50.f, 12, FColor::Green, false, -1.f);
		}

		// States?
		
	});
}

FVector UClientMoveProcessor::GetDirectionFollow(const FVector Location, FVector& Destination,
	FAmalgamFluxFragment& FluxFragment)
{
	int32 SplinePointIndex = FluxFragment.GetSplinePointIndex();

	USplineComponent* Spline = FluxFragment.GetFlux()->GetSplineForAmalgamsComponent();
	Destination = Spline->GetSplinePointAt(SplinePointIndex, ESplineCoordinateSpace::World).Position;
	Destination.Z = 0.f;

	return (Destination - Location).GetSafeNormal();
}

FVector UClientMoveProcessor::GetDirectionAggroed(const FVector Location, FVector& Destination,
	FAmalgamTargetFragment& TargetFragment, FAmalgamAggroFragment& AggroFragment, FAmalgamStateFragment& StateFragment)
{
	//TODO
	return FVector::ZeroVector;
}

FVector UClientMoveProcessor::GetTargetLocation(const FAmalgamTargetFragment TargetFrag)
{
	//TODO
	return FVector::ZeroVector;
}

bool UClientMoveProcessor::CheckIfPathEnded(FAmalgamFluxFragment& FluxFrag, FVector Location, FVector Destination,
	EAmalgamState State)
{
	float Distance = (Location - Destination).Length();
	switch (State) {
	case Inactive:
		break;
	case EAmalgamState::FollowPath:
		if (!FluxFrag.CheckFluxIsValid()) return false;
		break;
	default:
		break;
	}

	return Distance <= DistanceThresholdPathEnded;
}

bool UClientMoveProcessor::FollowPath(FTransformFragment& TrsfFrag, FAmalgamFluxFragment& FlxFrag,
	FAmalgamPathfindingFragment& PathFragment, FAmalgamDirectionFragment& DirFragment, float Speed,
	const float DeltaTime)
{
	return false; //TODO: Implement this function
}

bool UClientMoveProcessor::FollowTarget(FTransformFragment& TrsfFrag, FVector TargetLocation,
	FAmalgamDirectionFragment& DirFragment, float Speed, float AcceptancePathfindingRadius, const float DeltaTime)
{
	return false; //TODO: Implement this function
}

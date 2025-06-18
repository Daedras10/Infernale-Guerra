// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Amalgam/Processors/AmalgamMoveProcessor.h"

//Tags
#include "Mass/Army/AmalgamTags.h"

//Fragments
#include "Mass/Army/AmalgamFragments.h"

#include "MassCommonFragments.h"

//Processor
#include "MassExecutionContext.h"
#include <MassEntityTemplateRegistry.h>

//Subsystem
#include <Mass/Collision/SpatialHashGrid.h>

#include "Components/SplineComponent.h"
#include "LD/Buildings/BuildingParent.h"

// Misc
#include "Kismet/GameplayStatics.h"
#include "MassClient/Tags/ClientTags.h"
#include "Structs/ReplicationStructs.h"

UAmalgamMoveProcessor::UAmalgamMoveProcessor() : EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::All;
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Avoidance);
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Movement);

	// Shouldn't run if the visualisation manager wasn't found
	bAutoRegisterWithProcessingPhases = true;

	bRequiresGameThreadExecution = true;
}

void UAmalgamMoveProcessor::ConfigureQueries()
{
	/* Tags requiered */
	EntityQuery.AddTagRequirement<FAmalgamServerTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FAmalgamMoveTag>(EMassFragmentPresence::All);
	
	/* Tags to remove */
	EntityQuery.AddTagRequirement<FAmalgamClientTag>(EMassFragmentPresence::None);

	/* Fragments */
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamMovementFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamPathfindingFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamAggroFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamFluxFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamStateFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamNiagaraFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamDirectionFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamTransmutationFragment>(EMassFragmentAccess::ReadWrite);
	
	EntityQuery.RegisterWithProcessor(*this);
}

void UAmalgamMoveProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	if (!VisualisationManager)
	{
		TArray<AActor*> OutActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAmalgamVisualisationManager::StaticClass(), OutActors);

		if (OutActors.Num() > 0)
			VisualisationManager = static_cast<AAmalgamVisualisationManager*>(OutActors[0]);
		else
			VisualisationManager = nullptr;

		check(VisualisationManager);
	}
	if (!GameModeInfernale)
	{
		const auto GameMode = Cast<AGameModeInfernale>(UGameplayStatics::GetGameMode(GetWorld()));
		if (GameMode)
			GameModeInfernale = GameMode;
		else
			GameModeInfernale = nullptr;

		check(GameModeInfernale);
	}
	if (GameModeInfernale->GameHasEnded()) return;
	EntityQuery.ForEachEntityChunk(EntityManager, Context, ([this](FMassExecutionContext& Context)
	{
		TArrayView<FTransformFragment> TransformView = Context.GetMutableFragmentView<FTransformFragment>();
		TArrayView<FAmalgamFluxFragment> FluxFragView = Context.GetMutableFragmentView<FAmalgamFluxFragment>();
		TArrayView<FAmalgamMovementFragment> MovementFragView = Context.GetMutableFragmentView<FAmalgamMovementFragment>();
		TArrayView<FAmalgamPathfindingFragment> PathFragView = Context.GetMutableFragmentView<FAmalgamPathfindingFragment>();
		TArrayView<FAmalgamAggroFragment> AggroFragView = Context.GetMutableFragmentView<FAmalgamAggroFragment>();
		TArrayView<FAmalgamTargetFragment> TargetFragView = Context.GetMutableFragmentView<FAmalgamTargetFragment>();
		TArrayView<FAmalgamStateFragment> StateFragView = Context.GetMutableFragmentView<FAmalgamStateFragment>();
		TArrayView<FAmalgamNiagaraFragment> NiagaraFragView = Context.GetMutableFragmentView<FAmalgamNiagaraFragment>();
		TArrayView<FAmalgamDirectionFragment> DirectionFragView = Context.GetMutableFragmentView<FAmalgamDirectionFragment>();
		TArrayView<FAmalgamTransmutationFragment> TransFragView = Context.GetMutableFragmentView<FAmalgamTransmutationFragment>();
		
		const float WorldDeltaTime = Context.GetDeltaTimeSeconds();

		TArray<FVector> UpdatedLocations;
		TArray<FVector> UpdatedRotations;
		TArray<FHandleBool> EntityHandlesStateUpdate = TArray<FHandleBool>();

		for (int32 Index = 0; Index < Context.GetNumEntities(); ++Index)
		{
			FAmalgamStateFragment& StateFragment = StateFragView[Index];
			FAmalgamNiagaraFragment& NiagaraFragment = NiagaraFragView[Index];
			FAmalgamDirectionFragment& DirectionFragment = DirectionFragView[Index];
			FAmalgamAggroFragment& AggroFragment = AggroFragView[Index];
			FAmalgamTransmutationFragment& TransFrag = TransFragView[Index];

			FAmalgamFluxFragment& FluxFragment = FluxFragView[Index];
			FAmalgamTargetFragment& TargetFragment = TargetFragView[Index];
			FAmalgamMovementFragment* MovementFragment = &MovementFragView[Index];
			FAmalgamPathfindingFragment& PathFragment = PathFragView[Index];
			FTransformFragment& TransformFragment = TransformView[Index];
			FTransform& Transform = TransformFragment.GetMutableTransform();
			FVector Location = Transform.GetLocation();

			const auto State = StateFragView[Index].GetState();
			bool bSucceeded = true;
			const auto Flux = FluxFragment.GetFlux();
			const auto FluxValid = Flux.IsValid();

			if (!FluxValid && !PathFragment.IsPathFinal())
			{
				PathFragment.MakePathFinal();
				/*StateFragment.SetStateAndNotify(EAmalgamState::Killed, Context, Index);
				continue;*/
			}

			if (FluxValid)
			{
				const auto Version = PathFragment.GetUpdateVersion();
				const bool FluxVersionIsOk = Version == Flux->GetUpdateVersion();
				if (Version != -1 && !FluxVersionIsOk)
	            {
					PathFragment.MakePathFinal();
	            }

				if (!PathFragment.IsPathFinal())
				{
					if (PathFragment.GetUpdateID() != Flux->GetUpdateID())
					{
						PathFragment.CopyPathFromFlux(Flux, Location, false);
						MovementFragment->SetSpeedMult(Flux->GetAmalgamsSpeedMult());
					}

					if (PathFragment.ShouldRecover())
						PathFragment.RecoverPath(Location);
				}
			}

			FVector TargetLocation;

			switch (State)
			{
			case EAmalgamState::FollowPath:
				bSucceeded = FollowPath(TransformFragment, FluxFragment, PathFragment, DirectionFragment, TransFrag.GetSpeedModifier(MovementFragment->GetSpeed()), WorldDeltaTime);
				break;

			case EAmalgamState::Aggroed:
			{
				TargetLocation = GetTargetLocation(TargetFragment);

				float TargetRangeOffset = TargetFragment.GetTargetRangeOffset(StateFragment.GetAggro());
				float EntityRangeOffset = AggroFragment.GetFightRange();
				float TotalRangeOffset = TargetRangeOffset + EntityRangeOffset;
				TargetFragment.SetTotalRangeOffset(TotalRangeOffset);

				if (TargetLocation == FVector::ZeroVector)
				{
					StateFragment.SetAggro(EAmalgamAggro::NoAggro);
					StateFragment.SetStateAndNotify(EAmalgamState::FollowPath, Context, Index);
					continue;
				}
				if ((Location - TargetLocation).Length() - TotalRangeOffset < AggroFragment.GetFightRange())
				{
					StateFragment.SetStateAndNotify(EAmalgamState::Fighting, Context, Index);
					EntityHandlesStateUpdate.Add(FHandleBool(Context.GetEntity(Index), true));
					continue;
				}
				bSucceeded = FollowTarget(TransformFragment, TargetLocation, DirectionFragment, TransFrag.GetSpeedModifier(MovementFragment->GetRushSpeed()), PathFragment.GetAcceptanceAttackRadius(), WorldDeltaTime);
			}
				break;

			case EAmalgamState::Fighting:
				break;

			case EAmalgamState::Killed:
				break;

			default:
				bSucceeded = false;
				if (bDebugMove) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Orange, FString::Printf(TEXT("AmalgamMoveProcessor : \n\t Amalgam @Index %d should not be handled")));
				break;
			}

			if(!bSucceeded)
			{
				StateFragment.SetDeathReason(EAmalgamDeathReason::EndOfPath);
				StateFragment.SetStateAndNotify(EAmalgamState::Killed, Context, Index);
				//StateFragment.Kill(EAmalgamDeathReason::EndOfPath, Context, Index);
				continue;
			}
			
			UpdatedLocations.Add(TransformFragment.GetTransform().GetLocation());
			UpdatedRotations.Add(DirectionFragment.Direction);

			//VisualisationManager->UpdatePositionP(Context.GetEntity(Index), TransformFragment.GetTransform().GetLocation(), DirectionFragment.Direction);
		}

		VisualisationManager->UpdateStatesP(EntityHandlesStateUpdate);
		
		TArray<FMassEntityHandle> Entities(Context.GetEntities());
		if (Entities.Num() != UpdatedLocations.Num() || Entities.Num() != UpdatedRotations.Num())
		{
			if (bDebugMove) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, FString::Printf(TEXT("AmalgamMoveProcessor : \n\t Entities and UpdatedLocations size mismatch.")));
			return;
		}
		
		const auto PCs = GameModeInfernale->GetPlayerControllers();
		auto FPlayerControllerInfos = TArray<FPlayerControllerInfo>();
		for (const auto PC : PCs)
		{
			FPlayerControllerInfo Info;
			Info.PlayerController = PC;
			Info.PlayerLocation = PC->GetCameraCenterPoint();
			//Info.DataForVisualisations = FVisualDataArray();
			//Info.DataForVisualisations.Items = TArray<FVisualDataItemEntry>();
			Info.DataForVisualisations = TArray<FDataForVisualisation>();
			
			Info.EntityHandlesToHide = TArray<FMassEntityHandle>();
			FPlayerControllerInfos.Add(Info);
		}
		const auto Radius = VisualisationManager->GetRadius();


		TArray<FMassEntityHandle> Handles = TArray<FMassEntityHandle>();
		for (int32 i = 0; i < Entities.Num(); ++i)
		{
			const auto UpdatedLocation = UpdatedLocations[i];
			const auto UpdatedRotation = UpdatedRotations[i];
			FAmalgamMovementFragment* MovementFragment = &MovementFragView[i];
			
			FDataForVisualisation Data = FDataForVisualisation();
			Data.EntityHandle = Entities[i];
			Data.LocationX = UpdatedLocation.X;
			Data.LocationY = UpdatedLocation.Y;
			Data.RotationX = UpdatedRotation.X;
			Data.RotationY = UpdatedRotation.Y;

			// FVisualDataItemEntry VisualDataItemEntry = FVisualDataItemEntry();
			// VisualDataItemEntry.EntityHandle = Entities[i];
			// VisualDataItemEntry.LocationX = UpdatedLocation.X;
			// VisualDataItemEntry.LocationY = UpdatedLocation.Y;
			// VisualDataItemEntry.RotationX = UpdatedRotation.X;
			// VisualDataItemEntry.RotationY = UpdatedRotation.Y;

			bool ShouldReplicate = false;
			const auto CurrentTime = GetWorld()->GetTimeSeconds();

			FColor DrawColor = FColor::Black;
			
			for (FPlayerControllerInfo& FPlayerControllerInfo : FPlayerControllerInfos)
			{
				const auto PCPlayer = FPlayerControllerInfo.PlayerController->GetPlayerOwning();
				const auto Distance = FVector::DistSquared(FPlayerControllerInfo.PlayerLocation, UpdatedLocation);
				if (Distance > FMath::Square(Radius))
				{
					if (!MovementFragment->IsVisibleBy(PCPlayer))
					{
						DrawColor = FColor::Purple;
						continue;
					}
					
					const auto ShouldReplicateHide = FPlayerControllerInfo.PlayerController->ShouldReplicateUnitToHideThisFrame();
					if (ShouldReplicateHide)
					{
						MovementFragment->SetVisibleBy(PCPlayer, false);
						FPlayerControllerInfo.EntityHandlesToHide.Add(Entities[i]);
						DrawColor = FColor::Red;
					}
					continue;
				}

				if (!MovementFragment->IsVisibleBy(PCPlayer))
				{
					FPlayerControllerInfo.DataForVisualisations.Add(Data);
					MovementFragment->SetLastReplicationTime(PCPlayer, CurrentTime);
					MovementFragment->SetVisibleBy(PCPlayer, true);
					DrawColor = FColor::Green;
					continue;
				}
				
				ShouldReplicate = MovementFragment->LastReplicationTimeCompared(PCPlayer, CurrentTime);
				if (!ShouldReplicate) continue;
				FPlayerControllerInfo.DataForVisualisations.Add(Data);
				DrawColor = FColor::Blue;
				//FPlayerControllerInfo.DataForVisualisations.Items.Add(VisualDataItemEntry);
			}

			//DrawDebugSphere(GetWorld(), UpdatedLocation, 200, 12, DrawColor, false, 0.f);
			//DataForVisualisation.Add(Data);
		}
		
		for (int32 i = 0; i < UpdatedLocations.Num(); ++i)
		{
			Handles.Add(Entities[i]);
		}

		if (Entities.Num() != UpdatedLocations.Num())
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, FString::Printf(TEXT("AmalgamMoveProcessor : \n\t Handles and UpdatedLocations size mismatch.")));
			return;
		}
		
		//VisualisationManager->BatchUpdatePosition(Handles, DataForVisualisation);

		for (FPlayerControllerInfo& FPlayerControllerInfo : FPlayerControllerInfos)
		{
			/* Fast array */
			// FPlayerControllerInfo.DataForVisualisations.MarkArrayDirty();
			// FPlayerControllerInfo.PlayerController->UpdateUnits(FPlayerControllerInfo.DataForVisualisations, FPlayerControllerInfo.EntityHandlesToHide);

			/* Normal array */
			FPlayerControllerInfo.PlayerController->UpdateUnits(FPlayerControllerInfo.DataForVisualisations, FPlayerControllerInfo.EntityHandlesToHide);
			//GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, FString::Printf(TEXT("Update Mass loc: %d"), FPlayerControllerInfo.DataForVisualisations.Num()));
		}
	}));
	

}

FVector UAmalgamMoveProcessor::GetDirectionFollow(const FVector Location, FVector& Destination, FAmalgamFluxFragment& FluxFragment)
{
	int32 SplinePointIndex = FluxFragment.GetSplinePointIndex();

	USplineComponent* Spline = FluxFragment.GetFlux()->GetSplineForAmalgamsComponent();
	Destination = Spline->GetSplinePointAt(SplinePointIndex, ESplineCoordinateSpace::World).Position;

	Destination.Z = 0.f;

	return (Destination - Location).GetSafeNormal();
}

FVector UAmalgamMoveProcessor::GetDirectionAggroed(const FVector Location, FVector& Destination, FAmalgamTargetFragment& TargetFragment, FAmalgamAggroFragment& AggroFragment, FAmalgamStateFragment& StateFragment)
{
	switch (StateFragment.GetAggro())
	{
	case EAmalgamAggro::Amalgam:
		Destination = ASpatialHashGrid::GetEntityData(TargetFragment.GetTargetEntityHandle()).Location;
		break;

	case EAmalgamAggro::Building:
		if (!TargetFragment.GetTargetBuilding().IsValid())
		{
			if(bDebugMove) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, FString::Printf(TEXT("MoveProcessor : \n\t Target building is null.")));
			return FVector::ZeroVector;
		}

		Destination = TargetFragment.GetTargetBuilding()->GetActorLocation();
		break;
	}

	Destination.Z = 0.f;

	return (Destination - Location).GetSafeNormal();
}

FVector UAmalgamMoveProcessor::GetTargetLocation(const FAmalgamTargetFragment TargetFrag)
{
	FVector Location = FVector::ZeroVector;
	if (TargetFrag.GetTargetBuilding().IsValid()) 
	{
		Location = TargetFrag.GetTargetBuilding()->GetActorLocation();
	}
	else if (TargetFrag.GetTargetLDElem().IsValid())
	{
		Location = TargetFrag.GetTargetLDElem()->GetActorLocation();
	}
	else
	{
		if (!ASpatialHashGrid::Contains(TargetFrag.GetTargetEntityHandle()))
			return FVector::ZeroVector;

		Location = ASpatialHashGrid::GetEntityData(TargetFrag.GetTargetEntityHandle()).Location;
	}

	return Location;
}

bool UAmalgamMoveProcessor::CheckIfPathEnded(FAmalgamFluxFragment& FluxFrag, FVector Location, FVector Destination, EAmalgamState State)
{
	float Distance = (Location - Destination).Length();

	switch (State)
	{
	case EAmalgamState::FollowPath:
		if (!FluxFrag.CheckFluxIsValid()) return false;
		break;

	case EAmalgamState::Aggroed:

		break;

	default:
		break;
	}

	return Distance <= DistanceThreshold;
}

bool UAmalgamMoveProcessor::FollowPath(FTransformFragment& TrsfFrag, FAmalgamFluxFragment& FlxFrag, FAmalgamPathfindingFragment& PathFragment, FAmalgamDirectionFragment& DirFragment, float Speed, const float DeltaTime)
{
	TArray<FVector>& Path = PathFragment.Path;

	FTransform& Transform = TrsfFrag.GetMutableTransform();
	const auto CurrentLocation = TrsfFrag.GetTransform().GetLocation();
	if (Path.Num() == 0)
	{
		return false;
	}

	const FVector TargetLocation = Path[0];
	const auto Direction = TargetLocation - CurrentLocation;
	const auto Distance = Direction.Length();

	if (Distance < PathFragment.GetAcceptancePathfindingRadius())
	{
		FlxFrag.NextSplinePoint();
		PathFragment.NextPoint();
		return true;
	}

	const auto DirectionNormalized = Direction.GetSafeNormal();
	//Speed = TransmutationComponent->GetEffectUnitSpeed(Speed);
	const auto NewLocation = CurrentLocation + DirectionNormalized * Speed * DeltaTime;
	const auto OldLocation = CurrentLocation;
	Transform.SetLocation(NewLocation);
	
	DirFragment.TargetDirection = DirectionNormalized;
	DirFragment.Direction = FMath::Lerp(DirFragment.Direction, DirectionNormalized, .5f);

	return true;
	
}

bool UAmalgamMoveProcessor::FollowTarget(FTransformFragment& TrsfFrag, FVector TargetLocation, FAmalgamDirectionFragment& DirFragment, float Speed, float AcceptanceRadiusAttack, const float DeltaTime)
{
	FTransform& Transform = TrsfFrag.GetMutableTransform();
	const auto CurrentLocation = Transform.GetLocation();
	
	const auto Direction = TargetLocation - CurrentLocation;
	const auto Distance = Direction.Length();

	if (Distance < AcceptanceRadiusAttack) return true;

	const auto DirectionNormalized = Direction.GetSafeNormal();
	//Speed = TransmutationComponent->GetEffectUnitSpeed(Speed);
	const auto NewLocation = CurrentLocation + DirectionNormalized * Speed * DeltaTime;
	const auto OldLocation = CurrentLocation;
	Transform.SetLocation(NewLocation);

	DirFragment.TargetDirection = DirectionNormalized;
	DirFragment.Direction = FMath::Lerp(DirFragment.Direction, DirectionNormalized, .5f);

	return true;
}

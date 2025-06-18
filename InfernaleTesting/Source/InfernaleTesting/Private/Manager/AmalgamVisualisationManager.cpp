// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/AmalgamVisualisationManager.h"

// Niagara includes
#include <NiagaraFunctionLibrary.h>

// Project function library, for getting emissive color from owner
#include <FunctionLibraries/FunctionLibraryInfernale.h>
#include <UnitAsActor/NiagaraUnitAsActor.h>

#include "DataAsset/GameSettingsDataAsset.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "Kismet/GameplayStatics.h"
#include "Mass/Army/AmalgamFragments.h"
#include "MassClient/Spawners/ClientMassSpawner.h"
#include "Structs/ReplicationStructs.h"

FDataForSpawnVisual::FDataForSpawnVisual(): World(nullptr), Location(FVector(0.f, 0.f, 0.f)), EntityType(EEntityType::EntityTypeNone)
{
}

FHandleBool::FHandleBool(): Value(false)
{
}

FHandleBool::FHandleBool(FMassEntityHandle InHandle, bool InValue): Handle(InHandle), Value(InValue)
{
}

// Sets default values
AAmalgamVisualisationManager::AAmalgamVisualisationManager(): ClientMassSpawnerGobborit(nullptr),
                                                              ClientMassSpawnerNeras(nullptr),
                                                              ClientMassSpawnerBehemot(nullptr), InfernalePawn(nullptr)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//bReplicates = true;
}

// Called when the game starts or when spawned
void AAmalgamVisualisationManager::BeginPlay()
{
	Super::BeginPlay();
	ElementArray = TArray<FNiagaraVisualElement>();

	if (!HasAuthority())
	{
		// const auto World = GetWorld();
		// const auto PlayerController = UGameplayStatics::GetPlayerController(World, 0);
		// UEnvQueryInstanceBlueprintWrapper* QueryInstance = UEnvQueryManager::RunEQSQuery(World, SimpleEQSQuery, PlayerController, EEnvQueryRunMode::SingleResult, nullptr);
		//
		// if (QueryInstance)
		// {
		// 	UE_LOG(LogTemp, Log, TEXT("Ran dummy EQS query — this forces EQS Manager creation."));
		// }
		// else
		// {
		// 	UE_LOG(LogTemp, Error, TEXT("Failed to run dummy EQS query."));
		// }
		const auto GameSettings = UFunctionLibraryInfernale::GetGameSettingsDataAsset();
		if (GameSettings->UseMassLocal) CreateClientSpawners();
		return;
	}
	//CreateClientSpawners(); //TODO: remove
	const auto GameMode = Cast<AGameModeInfernale>(UGameplayStatics::GetGameMode(GetWorld()));
	if (!GameMode)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("GameMode is not valid"));
		return;
	}
	GameMode->PreLaunchGame.AddDynamic(this, &AAmalgamVisualisationManager::OnPreLaunchGame);
}

// Called every frame
void AAmalgamVisualisationManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, FString::Printf(TEXT("Units in array: %d"), BPElementsArray.Num()));
}

void AAmalgamVisualisationManager::DebugAIandEQS()
{
	auto World = GetWorld();
	if (!World)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, FString::Printf(TEXT("World is not valid (%s)"), HasAuthority() ? TEXT("Server") : TEXT("Client")));
		return;
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Green, FString::Printf(TEXT("World is valid: %s (netcode %d) (%s)"), *World->GetName(), (int32)GetWorld()->WorldType, HasAuthority() ? TEXT("Server") : TEXT("Client")));
	}
	UEnvQueryManager* EQSManager = UEnvQueryManager::GetCurrent(World);
	
	if (EQSManager)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, FString::Printf(TEXT("EQS Manager is available on %s."), HasAuthority() ? TEXT("Server") : TEXT("Client")));
		//UE_LOG(LogTemp, Log, TEXT("EQS Manager is available on the %s."), HasAuthority() ? TEXT("Server") : TEXT("Client"));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, FString::Printf(TEXT("EQS Manager is NOT available on %s."), HasAuthority() ? TEXT("Server") : TEXT("Client")));
		//UE_LOG(LogTemp, Warning, TEXT("EQS Manager is NOT available on the %s."), HasAuthority() ? TEXT("Server") : TEXT("Client"));
	}
	
	UAISystem* AISystem = UAISystem::GetCurrent(*World);
	if (AISystem)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, FString::Printf(TEXT("AI System is present on %s world."), HasAuthority() ? TEXT("Server") : TEXT("Client")));
		//UE_LOG(LogTemp, Log, TEXT("AI System is present on client world."));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, FString::Printf(TEXT("NO AI SYSTEM in %s world!"), HasAuthority() ? TEXT("Server") : TEXT("Client")));
		//UE_LOG(LogTemp, Error, TEXT("NO AI SYSTEM in this client world!"));
	}
}

void AAmalgamVisualisationManager::CreateAndAddToMapPSimple(FMassEntityHandle EntityHandle, FOwner EntityOwner, const UWorld* World, UNiagaraSystem* NiagaraSystem, const FVector Location)
{
	if (!HasAuthority())
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("CreateAndAddToMapP: No autority"));
		return;
	}
	CreateAndAddToMapMulticast(EntityHandle, EntityOwner, World, NiagaraSystem, Location);
}

void AAmalgamVisualisationManager::CreateAndAddToMapPBP(FMassEntityHandle EntityHandle, FDataForSpawnVisual DataForSpawnVisual)
{
	if (!HasAuthority())
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("CreateAndAddToMapP: No autority"));
		return;
	}
	CreateAndAddToMapMulticastBP(EntityHandle, DataForSpawnVisual, DataForSpawnVisual.BPVisualisation);
}

void AAmalgamVisualisationManager::BatchUpdatePosition(const TArray<FMassEntityHandle>& EntityHandles, const TArray<FDataForVisualisation>& DataForVisualisations)
{
	if (!HasAuthority())
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("BatchUpdatePosition: No autority"));
		return;
	}

	// if (Entities.Num() != Locations.Num() && Entities.Num() != Rotations.Num())
	// {
	// 	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("BatchUpdatePosition: Update Count Mismatch"));
	// 	return;
	// }

	/*for (int Index = 0; Index < Entities.Num(); ++Index)
	{
		FVector CurrentLoc = Locations[Index];
		FVector CurrentRot = Rotations[Index];
		UpdatePositionMulticast(Entities[Index], CurrentLoc.X, CurrentLoc.Y, CurrentRot.Z);
	}*/

	BatchUpdatePositionMulticast(EntityHandles, DataForVisualisations);
}

void AAmalgamVisualisationManager::UpdatePositionP(FMassEntityHandle EntityHandle, const FDataForVisualisation DataForVisualisation)
{
	if (!HasAuthority())
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("UpdatePositionP: No autority"));
		return;
	}
	
	UpdatePositionMulticast(EntityHandle, DataForVisualisation);
	//UpdatePositionMulticast(EntityHandle, Location.X, Location.Y, Rotation.Z);
}

void AAmalgamVisualisationManager::UpdateStateP(FMassEntityHandle EntityHandle, bool IsFighting)
{
	if (!HasAuthority())
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("UpdateStateP: No autority"));
		return;
	}

	/* Fight and movement Meshes were never implemented */
	//UpdateStateMulticast(EntityHandle, IsFighting);
	//UpdatePositionMulticast(EntityHandle, Location.X, Location.Y, Rotation.Z);
}

void AAmalgamVisualisationManager::UpdateStatesP(const TArray<FHandleBool>& EntityHandlesStateUpdate)
{
	if (!HasAuthority())
	{
		GEngine->AddOnScreenDebugMessage(-1, 25.f, FColor::Red, TEXT("UpdateStatesP: No autority"));
		return;
	}
	/* Fight and movement Meshes were never implemented */
	//UpdateStatesMulticast(EntityHandlesStateUpdate);
}

void AAmalgamVisualisationManager::RemoveFromMapP(FMassEntityHandle EntityHandle)
{
	if (!HasAuthority())
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("RemoveFromMap: No autority"));
		return;
	}
	RemoveFromMapMulticast(EntityHandle);
}

void AAmalgamVisualisationManager::ChangeBatch(int Value)
{
	ChangeBatchMulticast(Value);
}

float AAmalgamVisualisationManager::GetRadius()
{
	return Radius;
}

void AAmalgamVisualisationManager::UpdatePositionOfSpecificUnits(
	const TArray<FDataForVisualisation>& DataForVisualisations,
	const TArray<FMassEntityHandle>& EntitiesToHide)
{
	//const auto Nums = DataForVisualisations.Items.Num();
	const auto Nums = DataForVisualisations.Num();
	if (bDebugReplicationNumbers) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, FString::Printf(TEXT("UpdatePositionOfSpecificUnits: %d"), Nums));
	
	if (!InfernalePawn)
	{
		const auto PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		const auto PlayerControllerInfernale = Cast<APlayerControllerInfernale>(PlayerController);
		if (!PlayerControllerInfernale) return;
		InfernalePawn = Cast<AInfernalePawn>(PlayerControllerInfernale->GetPawn());
	}
	if (!InfernalePawn) return;
	
	LocalPointLocation = InfernalePawn->GetLookAtLocationPoint();

	for (int Index = 0; Index < Nums; ++Index)
	{
		//const auto& DataForVisualisation = DataForVisualisations.Items[Index];
		const auto& DataForVisualisation = DataForVisualisations[Index];
		UpdateItemPosition(DataForVisualisation.EntityHandle, DataForVisualisation);
	}

	for (const auto EntityToHide : EntitiesToHide)
	{
		HideItem(EntityToHide);
	}
}

void AAmalgamVisualisationManager::UpdateUnitsInfo(const TArray<FDataForVisualisation>& DataForVisualisations)
{
	//const auto Nums = DataForVisualisations.Items.Num();
	const auto Nums = DataForVisualisations.Num();
	if (bDebugReplicationNumbers) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, FString::Printf(TEXT("UpdatePositionOfSpecificUnits: %d"), Nums));

	for (int Index = 0; Index < Nums; ++Index)
	{
		//const auto& DataForVisualisation = DataForVisualisations.Items[Index];
		const auto& DataForVisualisation = DataForVisualisations[Index];
		UpdateItemPosition(DataForVisualisation.EntityHandle, DataForVisualisation);
	}
}

void AAmalgamVisualisationManager::UpdateUnitsToHide(const TArray<FMassEntityHandle>& EntitiesToHide)
{
	for (const auto EntityToHide : EntitiesToHide)
	{
		HideItem(EntityToHide);
	}
}

void AAmalgamVisualisationManager::AddToMapMulticast_Implementation(FMassEntityHandle EntityHandle, UNiagaraComponent* NiagaraComponent)
{
	//GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("Added to map"));
	AddToMap(EntityHandle, NiagaraComponent);
}

void AAmalgamVisualisationManager::AddToMapMulticastBP_Implementation(FMassEntityHandle EntityHandle, AActor* BPVisualisation)
{
	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("Added to map"));

	AddToMap(EntityHandle, BPVisualisation);
}

void AAmalgamVisualisationManager::CreateAndAddToMapMulticast_Implementation(FMassEntityHandle EntityHandle, FOwner EntityOwner, const UWorld* World, UNiagaraSystem* NiagaraSystem, const FVector Location)
{
	CreateAndAddToMap(EntityHandle, EntityOwner, World, NiagaraSystem, Location);
}

void AAmalgamVisualisationManager::CreateAndAddToMapMulticastBP_Implementation(FMassEntityHandle EntityHandle, FDataForSpawnVisual DataForSpawnVisual, TSubclassOf<AActor> BPVisualisation)
{
	CreateAndAddToMap(EntityHandle, DataForSpawnVisual, BPVisualisation);
}

void AAmalgamVisualisationManager::UpdatePositionMulticast_Implementation(FMassEntityHandle EntityHandle, const FDataForVisualisation DataForVisualisation)
{
	//FVector Location, Rotation;

	//Location = FVector(xLoc, yLoc, 0.f); //@todo replace 0 w/ height offset
	//Rotation = FVector(0.f, 0.f, Rot);

	//UpdateItemPosition(EntityHandle, DataForVisualisation);
}

void AAmalgamVisualisationManager::BatchUpdatePositionMulticast_Implementation(const TArray<FMassEntityHandle>& EntityHandles, const TArray<FDataForVisualisation>& DataForVisualisations)
{
	if (EntityHandles.Num() != DataForVisualisations.Num())
	{
		//GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, TEXT("BatchUpdatePositionMulticast: EntityHandles and DataForVisualisations size mismatch."));
		return;
	}

	// const auto Mins = FMath::Min(EntityHandles.Num(), CurrentBatch + UnitsByBatch);
	//
	// for (int Index = CurrentBatch; Index < Mins; ++Index)
	// {
	// 	UpdateItemPosition(EntityHandles[Index], DataForVisualisations[Index]);
	// }
	// CurrentBatch += UnitsByBatch;
	// if (CurrentBatch >= EntityHandles.Num()) CurrentBatch = 0;

	if (!InfernalePawn)
	{
		const auto PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		const auto PlayerControllerInfernale = Cast<APlayerControllerInfernale>(PlayerController);
		if (!PlayerControllerInfernale) return;
		InfernalePawn = Cast<AInfernalePawn>(PlayerControllerInfernale->GetPawn());
	}
	if (!InfernalePawn) return;
	
	LocalPointLocation = InfernalePawn->GetLookAtLocationPoint();
	LocalPointLocation.Z = 0.f;
	
	for (int Index = 0; Index < EntityHandles.Num(); ++Index)
	{
		//UpdateItemPosition(EntityHandles[Index], DataForVisualisations[Index]);
	}
    
}

void AAmalgamVisualisationManager::RemoveFromMapMulticast_Implementation(FMassEntityHandle EntityHandle)
{
	RemoveFromMap(EntityHandle);
}

void AAmalgamVisualisationManager::AddToMap(FMassEntityHandle EntityHandle, UNiagaraComponent* NiagaraComponent)
{
	uint64 HandleAsNumber = EntityHandle.AsNumber();
	//checkf(!ContainsElement(HandleAsNumber), TEXT("Map already contains handle."));

	if (ContainsElement(HandleAsNumber)) return;

	FNiagaraVisualElement NewElement(HandleAsNumber, NiagaraComponent);
	
	ElementArray.Add(NewElement);
}

void AAmalgamVisualisationManager::AddToMap(FMassEntityHandle EntityHandle, AActor* BPActor)
{
	uint64 HandleAsNumber = EntityHandle.AsNumber();
	//checkf(!ContainsElement(HandleAsNumber), TEXT("Map already contains handle."));

	if (ContainsElement(HandleAsNumber)) return;

	FBPVisualElement NewElement(HandleAsNumber, -1, BPActor);

	BPElementsArray.Add(NewElement);
}

void AAmalgamVisualisationManager::CreateAndAddToMap(FMassEntityHandle EntityHandle, FOwner EntityOwner, const UWorld* World, UNiagaraSystem* NiagaraSystem, const FVector Location)
{
	uint64 HandleAsNumber = EntityHandle.AsNumber();
	//checkf(!ContainsElement(HandleAsNumber), TEXT("Map already contains handle."));

	if (ContainsElement(HandleAsNumber)) return;

	const auto TeamColor = UFunctionLibraryInfernale::GetTeamColorCpp(EntityOwner.Team, EEntityType::EntityTypeBehemot);
	const auto EmissiveColor = UFunctionLibraryInfernale::GetTeamColorEmissiveCpp(EntityOwner.Team, EEntityType::EntityTypeBehemot);
	

	UNiagaraComponent* SpawnedComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, NiagaraSystem, Location);
	SpawnedComponent->SetColorParameter("TeamColor", TeamColor);
	SpawnedComponent->SetColorParameter("EmissiveColor", EmissiveColor);
	FNiagaraVisualElement NewElement(HandleAsNumber, SpawnedComponent);

	ElementArray.Add(NewElement);
}

void AAmalgamVisualisationManager::CreateAndAddToMap(FMassEntityHandle EntityHandle, FDataForSpawnVisual DataForSpawnVisual, TSubclassOf<AActor> BPVisualisation)
{
	uint64 HandleAsNumber = EntityHandle.AsNumber();
	//checkf(!ContainsElement(HandleAsNumber), TEXT("Map already contains handle."));

	if (HasAuthority())
	{
		if (ContainsElement(HandleAsNumber))
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, TEXT("CreateAndAddToMap: Map already contains handle."));
			return;
		}
	}

	/* Client Spawn */
	//TODO: 
	// Let's try this
	FAmalgamClientInitializeInfo ClientInitializeInfo;
	ClientInitializeInfo.ServerEntityHandle = EntityHandle;
	ClientInitializeInfo.EntityOwner = DataForSpawnVisual.EntityOwner;
	ClientInitializeInfo.BPVisualisation = BPVisualisation;
	ClientInitializeInfo.Location = DataForSpawnVisual.Location;
	ClientInitializeInfo.Flux = DataForSpawnVisual.Flux;
	ClientInitializeInfo.EntityType = DataForSpawnVisual.EntityType;

	const auto GameSettings = UFunctionLibraryInfernale::GetGameSettingsDataAsset();
	if (HasAuthority() || !GameSettings->UseMassLocal)
	{
		const auto TeamColor = UFunctionLibraryInfernale::GetTeamColorCpp(DataForSpawnVisual.EntityOwner.Team, DataForSpawnVisual.EntityType);
		const auto EmissiveColor = UFunctionLibraryInfernale::GetTeamColorEmissiveCpp(DataForSpawnVisual.EntityOwner.Team, DataForSpawnVisual.EntityType);
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, TeamColor.ToFColorSRGB() , FString::Printf(TEXT("GetTeamColorCpp: Team: %s, EntityType: %s"), *UEnum::GetValueAsString(DataForSpawnVisual.EntityOwner.Team), *UEnum::GetValueAsString(DataForSpawnVisual.EntityType)));

		AActor* Spawned;
		Spawned = GetWorld()->SpawnActor<AActor>(BPVisualisation, DataForSpawnVisual.Location, FRotator(0.f, 0.f, 0.f));
		auto NiagaraUnitAsActor = Cast<ANiagaraUnitAsActor>(Spawned);
		if (!NiagaraUnitAsActor)
		{
			if (Spawned) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, FString::Printf(TEXT("CreateAndAddToMap: Failed to cast to ANiagaraUnitAsActor, actor: %s"), *Spawned->GetName()));
			else GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, TEXT("CreateAndAddToMap: Failed to cast to ANiagaraUnitAsActor, actor is null"));
			return;
		}
		NiagaraUnitAsActor->SetNumberOfSpawners(DataForSpawnVisual.NumberOfSpawners);
		NiagaraUnitAsActor->GetNiagaraComponent()->SetColorParameter("TeamColor", TeamColor);
		NiagaraUnitAsActor->GetNiagaraComponent()->SetColorParameter("EmissiveColor", EmissiveColor);
		// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("-------------------------------------------------------------")));
		// GEngine->AddOnScreenDebugMessage(-1, 5.f, TeamColor.ToFColorSRGB(), FString::Printf(TEXT("GetTeamColorCpp: Team: %s, EntityType: %s"), *UEnum::GetValueAsString(DataForSpawnVisual.EntityOwner.Team), *UEnum::GetValueAsString(DataForSpawnVisual.EntityType)));
		// GEngine->AddOnScreenDebugMessage(-1, 5.f, EmissiveColor.ToFColorSRGB(), FString::Printf(TEXT("GetTeamColorCpp: Team: %s, EntityType: %s"), *UEnum::GetValueAsString(DataForSpawnVisual.EntityOwner.Team), *UEnum::GetValueAsString(DataForSpawnVisual.EntityType)));
		// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("-------------------------------------------------------------")));
		NiagaraUnitAsActor->SpeedMultiplierUpdated(DataForSpawnVisual.SpeedMultiplier);
		NiagaraUnitAsActor->OwnerOnCreation(DataForSpawnVisual.EntityOwner);
		const FBPVisualElement NewElement(HandleAsNumber, -1, Spawned);
		BPElementsArray.Add(NewElement);
		return;
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Purple, TEXT("CreateAndAddToMap: Client side spawning"));
	ClientInitializeInfos.Add(ClientInitializeInfo);
	
	/* Add to UnitToSpawn */
	const auto Spawner = GetClientSpawner(DataForSpawnVisual.EntityType);
	if (!Spawner)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, FString::Printf(TEXT("CreateAndAddToMap: Spawner is null for EntityType: %s"), *UEnum::GetValueAsString(DataForSpawnVisual.EntityType)));
		return;
	}
	Spawner->DoClientSpawning();
}

FBPVisualElement AAmalgamVisualisationManager::CreateVisualUnitClient(FAmalgamClientInitializeInfo ClientInitializeInfo)
{
	const auto TeamColor = UFunctionLibraryInfernale::GetTeamColorCpp(ClientInitializeInfo.EntityOwner.Team, ClientInitializeInfo.EntityType);
	const auto EmissiveColor = UFunctionLibraryInfernale::GetTeamColorEmissiveCpp(ClientInitializeInfo.EntityOwner.Team, ClientInitializeInfo.EntityType);
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, TeamColor.ToFColorSRGB() , FString::Printf(TEXT("GetTeamColorCpp: Team: %s, EntityType: %s"), *UEnum::GetValueAsString(DataForSpawnVisual.EntityOwner.Team), *UEnum::GetValueAsString(DataForSpawnVisual.EntityType)));

	AActor* Spawned;
	Spawned = GetWorld()->SpawnActor<AActor>(ClientInitializeInfo.BPVisualisation, ClientInitializeInfo.Location, FRotator(0.f, 0.f, 0.f));
	auto NiagaraUnitAsActor = Cast<ANiagaraUnitAsActor>(Spawned);
	if (!NiagaraUnitAsActor)
	{
		if (Spawned) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, FString::Printf(TEXT("CreateAndAddToMap: Failed to cast to ANiagaraUnitAsActor, actor: %s"), *Spawned->GetName()));
		else GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, TEXT("CreateAndAddToMap: Failed to cast to ANiagaraUnitAsActor, actor is null"));
		return FBPVisualElement();
	}
	NiagaraUnitAsActor->SetNumberOfSpawners(1);
	NiagaraUnitAsActor->GetNiagaraComponent()->SetColorParameter("TeamColor", TeamColor);
	NiagaraUnitAsActor->GetNiagaraComponent()->SetColorParameter("EmissiveColor", EmissiveColor);
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("-------------------------------------------------------------")));
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, TeamColor.ToFColorSRGB(), FString::Printf(TEXT("GetTeamColorCpp: Team: %s, EntityType: %s"), *UEnum::GetValueAsString(DataForSpawnVisual.EntityOwner.Team), *UEnum::GetValueAsString(DataForSpawnVisual.EntityType)));
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, EmissiveColor.ToFColorSRGB(), FString::Printf(TEXT("GetTeamColorCpp: Team: %s, EntityType: %s"), *UEnum::GetValueAsString(DataForSpawnVisual.EntityOwner.Team), *UEnum::GetValueAsString(DataForSpawnVisual.EntityType)));
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("-------------------------------------------------------------")));


	NiagaraUnitAsActor->SpeedMultiplierUpdated(1);
	NiagaraUnitAsActor->OwnerOnCreation(ClientInitializeInfo.EntityOwner);

	// TODO HandleServer / HandleClient
	uint64 HandleAsNumber = ClientInitializeInfo.ServerEntityHandle.AsNumber();
	const FBPVisualElement NewElement(HandleAsNumber, -1, Spawned);
	BPElementsArray.Add(NewElement);
	return NewElement;
}

void AAmalgamVisualisationManager::UpdateItemPosition(FMassEntityHandle EntityHandle, FDataForVisualisation DataForVisualisation)
{
	if (!bHideAll) return;
	uint64 HandleAsNumber = EntityHandle.AsNumber();
	//checkf(ContainsElement(HandleAsNumber), TEXT("Handle is not in map."));
	
	if (!ContainsElement(HandleAsNumber))
	{
		//GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red, TEXT("UpdateItemPosition: Handle is not in map."));
		//auto Location = FVector(DataForVisualisation.LocationX, DataForVisualisation.LocationY, 0.f);
		//DrawDebugSphere(GetWorld(), Location + FVector(0, 0, 300), 200, 12, FColor::Yellow, false, 0.f);
		return;
	}
	auto Location = FVector(DataForVisualisation.LocationX, DataForVisualisation.LocationY, 0.f);
	auto Rotation = FVector(DataForVisualisation.RotationX, DataForVisualisation.RotationY,0.f);
	
	if (bUseBPVisualisation)
	{
		AActor* Visualisation = FindElementBP(HandleAsNumber)->Element.Get();
		ANiagaraUnitAsActor* NiagaraActor = Cast<ANiagaraUnitAsActor>(Visualisation);
		
		NiagaraActor->Activate(true);

		const auto Rot = Rotation.Rotation();
		auto Offset = NiagaraActor->GetOffset();
		Offset = Rot.RotateVector(Offset);
		
		Visualisation->SetActorLocation(Location + Offset); 
		//DrawDebugSphere(GetWorld(), Location + FVector(0, 0, 300), 200, 12, FColor::Red, false, 0.f);
		Visualisation->SetActorRotation(Rot);
		return;
	}

	TWeakObjectPtr<UNiagaraComponent> NC = GetNiagaraComponent(HandleAsNumber);
	NC->SetRelativeLocation(Location);
	NC->SetRelativeRotation(Rotation.Rotation());
}

void AAmalgamVisualisationManager::UpdateItemState(FMassEntityHandle EntityHandle, bool IsFighting)
{
	uint64 HandleAsNumber = EntityHandle.AsNumber();
	if (!ContainsElement(HandleAsNumber)) return;

	AActor* BPElem = FindElementBP(HandleAsNumber)->Element.Get();
	ANiagaraUnitAsActor* NiagaraActor = Cast<ANiagaraUnitAsActor>(BPElem);
	NiagaraActor->GetNiagaraComponent()->SetVariableStaticMesh("UnitMesh", NiagaraActor->GetMesh(IsFighting));
}

void AAmalgamVisualisationManager::HideItem(FMassEntityHandle EntityHandle)
{
	uint64 HandleAsNumber = EntityHandle.AsNumber();
	//checkf(ContainsElement(HandleAsNumber), TEXT("Handle is not in map."));
	
	if (!ContainsElement(HandleAsNumber)) return;
	
	if (bUseBPVisualisation)
	{
		AActor* Visualisation = FindElementBP(HandleAsNumber)->Element.Get();
		ANiagaraUnitAsActor* NiagaraActor = Cast<ANiagaraUnitAsActor>(Visualisation);
		NiagaraActor->Activate(false);
	}
}

void AAmalgamVisualisationManager::ShowHideAllItems(bool bShow)
{
	bHideAll = !bShow;
	if (!bShow) return;
	for (int Index = 0; Index < BPElementsArray.Num(); ++Index)
	{
		if (bUseBPVisualisation)
		{
			AActor* Visualisation = FindElementBP(BPElementsArray[Index].Handle)->Element.Get();
			ANiagaraUnitAsActor* NiagaraActor = Cast<ANiagaraUnitAsActor>(Visualisation);
			NiagaraActor->Activate(!bShow);
		}
	}
}



void AAmalgamVisualisationManager::RemoveFromMap(FMassEntityHandle EntityHandle)
{
	uint64 HandleAsNumber = EntityHandle.AsNumber();
	//checkf(ContainsElement(HandleAsNumber), TEXT("Handle is not in map."));

	if (!ContainsElement(HandleAsNumber)) return;

	if (bUseBPVisualisation)
	{
		int Idx = FindElementIndex(HandleAsNumber);
		const auto ElementBP = BPElementsArray[Idx];
		ElementBP.Element->Destroy();
		BPElementsArray.RemoveAt(Idx);
		return;
	}

	TWeakObjectPtr<UNiagaraComponent> NC = GetNiagaraComponent(HandleAsNumber);
	NC->DestroyComponent();

	int Idx = FindElementIndex(HandleAsNumber);
	ElementArray.RemoveAt(Idx);
}

void AAmalgamVisualisationManager::OnPreLaunchGame()
{
}

void AAmalgamVisualisationManager::ChangeBatchMulticast_Implementation(int Value)
{
	// UnitsByBatch += Value;
	// if (UnitsByBatch < 10) UnitsByBatch = 10;
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("NewVal: %d"), UnitsByBatch));


	Radius += Value;
	if (Radius < 1000) Radius = 1000;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Radius: %d"), Radius));
}

void AAmalgamVisualisationManager::SetGameIsEndingMulticast_Implementation(bool Value)
{
	bGameIsEnding = Value;
}

TWeakObjectPtr<UNiagaraComponent> AAmalgamVisualisationManager::GetNiagaraComponent(uint64 ElementHandle)
{
	TWeakObjectPtr<UNiagaraComponent> NC = FindElement(ElementHandle)->NiagaraComponent;
	return NC;
}

FNiagaraVisualElement* AAmalgamVisualisationManager::FindElement(uint64 ElementHandle)
{
	FNiagaraVisualElement* FoundElement = ElementArray.FindByPredicate([&](const FNiagaraVisualElement& Element)
		{
			return Element.Handle == ElementHandle;
		});

	return FoundElement;
}

FBPVisualElement* AAmalgamVisualisationManager::FindElementBP(uint64 ElementHandle)
{
	FBPVisualElement* FoundElement = BPElementsArray.FindByPredicate([&](const FBPVisualElement& Element)
		{
			return Element.Handle == ElementHandle;
		});

	return FoundElement;
}

bool AAmalgamVisualisationManager::IsGameIsEnding()
{
	return bGameIsEnding;
}

void AAmalgamVisualisationManager::SetGameIsEnding(bool Value)
{
	SetGameIsEndingMulticast(Value);
}

void AAmalgamVisualisationManager::AddClientHandleTo(uint64 ElementHandle, uint64 ClientHandle)
{
	
	FBPVisualElement* FoundElement = FindElementBP(ElementHandle);
	if (!FoundElement) return;
	FoundElement->ClientHandle = ClientHandle;
}

bool AAmalgamVisualisationManager::ClientInitializeInfosIsEmpty()
{
	return ClientInitializeInfos.IsEmpty();
}

FAmalgamClientInitializeInfo AAmalgamVisualisationManager::GetFirstClientInitializeInfosAndRemoveit()
{
	const auto FirstInfo = ClientInitializeInfos[0];
	ClientInitializeInfos.RemoveAt(0);
	return FirstInfo;
}

AClientMassSpawner* AAmalgamVisualisationManager::GetClientSpawner(EEntityType EntityType) const
{
	switch (EntityType) {
	case EEntityType::EntityTypeNone:
		break;
	case EEntityType::EntityTypeBehemot:
		return ClientMassSpawnerBehemot;
	case EEntityType::EntityTypeGobborit:
		return ClientMassSpawnerGobborit;
	case EEntityType::EntityTypeNerras:
		return ClientMassSpawnerNeras;
	case EEntityType::EntityTypeNeutralCamp:
		break;
	case EEntityType::EntityTypeBoss:
		break;
	case EEntityType::EntityTypeCity:
		break;
	case EEntityType::EntityTypeBuilding:
		break;
	}
	return nullptr;
}

void AAmalgamVisualisationManager::CreateClientSpawners()
{
	ClientMassSpawnerGobborit = GetWorld()->SpawnActor<AClientMassSpawner>(AClientMassSpawner::StaticClass(), FVector(0.f, 0.f, 0.f), FRotator(0.f, 0.f, 0.f));
	ClientMassSpawnerGobborit->Initialize(EntityTypesGobborit, SpawnDataGeneratorsGobborit);

	ClientMassSpawnerBehemot = GetWorld()->SpawnActor<AClientMassSpawner>(AClientMassSpawner::StaticClass(), FVector(0.f, 0.f, 0.f), FRotator(0.f, 0.f, 0.f));
	ClientMassSpawnerBehemot->Initialize(EntityTypesBehemot, SpawnDataGeneratorsBehemot);

	ClientMassSpawnerNeras = GetWorld()->SpawnActor<AClientMassSpawner>(AClientMassSpawner::StaticClass(), FVector(0.f, 0.f, 0.f), FRotator(0.f, 0.f, 0.f));
	ClientMassSpawnerNeras->Initialize(EntityTypesNeras, SpawnDataGeneratorsNeras);

	ClientMassSpawnerGobborit->PostRegister();
	ClientMassSpawnerBehemot->PostRegister();
	ClientMassSpawnerNeras->PostRegister();
}

int32 AAmalgamVisualisationManager::FindElementIndex(uint64 ElementHandle)
{
	if (bUseBPVisualisation)
	{
		int32 FoundElement = BPElementsArray.IndexOfByPredicate([&](const FBPVisualElement& Element)
			{
				return Element.Handle == ElementHandle;
			});
		return FoundElement;
	}

	int32 FoundElement = ElementArray.IndexOfByPredicate([&](const FNiagaraVisualElement& Element)
		{
			return Element.Handle == ElementHandle;
		});

	return FoundElement;
}

bool AAmalgamVisualisationManager::ContainsElement(uint64 ElementHandle)
{
	if (bUseBPVisualisation)
	{
		FBPVisualElement* FoundElement = BPElementsArray.FindByPredicate([&](const FBPVisualElement& Element)
			{
				return Element.Handle == ElementHandle;
			});
		return FoundElement != nullptr;
	}

	FNiagaraVisualElement* FoundElement = ElementArray.FindByPredicate([&](const FNiagaraVisualElement& Element)
		{
			return Element.Handle == ElementHandle;
		});

	return FoundElement != nullptr;
}

void AAmalgamVisualisationManager::UpdateStateMulticast_Implementation(FMassEntityHandle EntityHandle, bool IsFighting)
{
	UpdateItemState(EntityHandle, IsFighting);
}

void AAmalgamVisualisationManager::UpdateStatesMulticast_Implementation(
	const TArray<FHandleBool>& EntityHandlesStateUpdate)
{
	for (const auto& EntityHandleState : EntityHandlesStateUpdate)
	{
		UpdateItemState(EntityHandleState.Handle, EntityHandleState.Value);
	}
}

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

// Mass includes
#include <MassEntityTypes.h>

// Niagara includes
#include <NiagaraComponent.h>

// Network includes
#include <Net/UnrealNetwork.h>

// IG includes
#include <Interfaces/Ownable.h>

#include "LD/LDElement/Boss.h"
#include "Structs/SimpleStructs.h"
#include "AmalgamVisualisationManager.generated.h"


struct FMassSpawnDataGenerator;
struct FMassSpawnedEntityType;
class AClientMassSpawner;
class AAmalgamSpawnerParent;
struct FDataForVisualisation;
enum class EEntityType : uint8;

USTRUCT()
struct FNiagaraVisualElement
{
	GENERATED_USTRUCT_BODY()

	FNiagaraVisualElement() = default;
	FNiagaraVisualElement(uint64 InHandle, UNiagaraComponent* InNiagaraComponent) : Handle(InHandle), NiagaraComponent(InNiagaraComponent)
	{ }

	inline bool operator==(const FNiagaraVisualElement Other) { return Handle == Other.Handle; }

	uint64 Handle;
	UNiagaraComponent* NiagaraComponent;
};

USTRUCT()
struct FBPVisualElement
{
	GENERATED_USTRUCT_BODY()

	FBPVisualElement() = default;
	FBPVisualElement(uint64 InHandle, uint64 InClientHandle, AActor* InElement) : Handle(InHandle), ClientHandle(InClientHandle), 
		Element(InElement)
	{
	}

	inline bool operator==(const FNiagaraVisualElement Other) const { return Handle == Other.Handle; }

	uint64 Handle;
	uint64 ClientHandle;
	TWeakObjectPtr<AActor> Element;
};


USTRUCT()
struct FDataForSpawnVisual
{
	GENERATED_USTRUCT_BODY()
public:
	FDataForSpawnVisual();

public:
	UPROPERTY() FOwner EntityOwner;
	UPROPERTY() UWorld* World;
	UPROPERTY() TSubclassOf<AActor> BPVisualisation;
	UPROPERTY() FVector Location;
	UPROPERTY() AFlux* Flux = nullptr;
	UPROPERTY() float SpeedMultiplier = -1;
	UPROPERTY() EEntityType EntityType;
	UPROPERTY() int NumberOfSpawners = 1;
};

USTRUCT()
struct FHandleBool
{
	GENERATED_USTRUCT_BODY()
public:
	FHandleBool();
	FHandleBool(FMassEntityHandle InHandle, bool InValue);

public:
	UPROPERTY() FMassEntityHandle Handle;
	UPROPERTY() bool Value;
};

USTRUCT()
struct FAmalgamClientInitializeInfo
{
	GENERATED_BODY()

public:
	UPROPERTY() FMassEntityHandle ServerEntityHandle;
	UPROPERTY() FOwner EntityOwner;
	UPROPERTY() TSubclassOf<AActor> BPVisualisation;
	UPROPERTY() FVector Location;
	UPROPERTY() AFlux* Flux = nullptr;
	UPROPERTY() EEntityType EntityType;
};




UCLASS()
class INFERNALETESTING_API AAmalgamVisualisationManager : public AActor
{
	GENERATED_BODY()
	
	/* Default AActor Methods */
public:	
	// Sets default values for this actor's properties
	AAmalgamVisualisationManager();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(CallInEditor) void DebugAIandEQS();
	
	/* End of Default AActor Methods */

public: /* Public AVM Methods */
	void CreateAndAddToMapPSimple(FMassEntityHandle EntityHandle, FOwner EntityOwner, const UWorld* World, UNiagaraSystem* NiagaraSystem, const FVector Location);
	void CreateAndAddToMapPBP(FMassEntityHandle EntityHandle, FDataForSpawnVisual DataForSpawnVisual);
	void BatchUpdatePosition(const TArray<FMassEntityHandle>& EntityHandles, const TArray<FDataForVisualisation>& DataForVisualisations);
	void UpdatePositionP(FMassEntityHandle EntityHandle, const FDataForVisualisation DataForVisualisation);
	void UpdateStateP(FMassEntityHandle EntityHandle, bool IsFighting);
	void UpdateStatesP(const TArray<FHandleBool>& EntityHandlesStateUpdate);
	void RemoveFromMapP(FMassEntityHandle EntityHandle);
	void ChangeBatch(int Value);

	float GetRadius();
	void UpdatePositionOfSpecificUnits(const TArray<FDataForVisualisation>& DataForVisualisations, const TArray<FMassEntityHandle>& EntitiesToHide);
	void UpdateUnitsInfo(const TArray<FDataForVisualisation>& DataForVisualisations);
	void UpdateUnitsToHide(const TArray<FMassEntityHandle>& EntitiesToHide);

	void ShowHideAllItems(bool bShow);
	FBPVisualElement* FindElementBP(uint64 ElementHandle);
	bool IsGameIsEnding();
	void SetGameIsEnding(bool Value);

	void AddClientHandleTo(uint64 ElementHandle, uint64 ClientHandle);

	bool ClientInitializeInfosIsEmpty();
	FAmalgamClientInitializeInfo GetFirstClientInitializeInfosAndRemoveit();

	FBPVisualElement CreateVisualUnitClient(FAmalgamClientInitializeInfo ClientInitializeInfo);
	AClientMassSpawner* GetClientSpawner(EEntityType EntityType) const;

protected: /* Protected AVM Methods */
		   /* These should only be called through replicated methods */
		   /* so that managers are never out of sync */

	void CreateClientSpawners();
	
	

	void AddToMap(FMassEntityHandle EntityHandle, UNiagaraComponent* NiagaraComponent);
	void AddToMap(FMassEntityHandle EntityHandle, AActor* BPActor);
	void CreateAndAddToMap(FMassEntityHandle EntityHandle, FOwner EntityOwner, const UWorld* World, UNiagaraSystem* NiagaraSystem, const FVector Location);
	void CreateAndAddToMap(FMassEntityHandle EntityHandle, FDataForSpawnVisual DataForSpawnVisual, TSubclassOf<AActor> BPVisualisation);

	void UpdateItemPosition(FMassEntityHandle EntityHandle, FDataForVisualisation DataForVisualisation);
	void UpdateItemState(FMassEntityHandle EntityHandle, bool IsFighting);
	void HideItem(FMassEntityHandle EntityHandle);

	void RemoveFromMap(FMassEntityHandle EntityHandle);

	UFUNCTION() void OnPreLaunchGame();

	UFUNCTION(NetMulticast, Reliable) void AddToMapMulticast(FMassEntityHandle EntityHandle, UNiagaraComponent* NiagaraComponent);
	UFUNCTION(NetMulticast, Reliable) void AddToMapMulticastBP(FMassEntityHandle EntityHandle, AActor* BPVisualisation);
	UFUNCTION(NetMulticast, Reliable) void CreateAndAddToMapMulticast(FMassEntityHandle EntityHandle, FOwner EntityOwner, const UWorld* World, UNiagaraSystem* NiagaraSystem, const FVector Location);
	UFUNCTION(NetMulticast, Reliable) void CreateAndAddToMapMulticastBP(FMassEntityHandle EntityHandle, FDataForSpawnVisual DataForSpawnVisual, TSubclassOf<AActor> BPVisualisation);
	UFUNCTION(NetMulticast, Unreliable) void UpdatePositionMulticast(FMassEntityHandle EntityHandle, const FDataForVisualisation DataForVisualisation);
	UFUNCTION(NetMulticast, Unreliable) void UpdateStateMulticast(FMassEntityHandle EntityHandle, bool IsFighting);
	UFUNCTION(NetMulticast, Unreliable) void UpdateStatesMulticast(const TArray<FHandleBool>& EntityHandlesStateUpdate);
	UFUNCTION(NetMulticast, Reliable) void BatchUpdatePositionMulticast(const TArray<FMassEntityHandle>& EntityHandles, const TArray<FDataForVisualisation>& DataForVisualisations);
	UFUNCTION(NetMulticast, Reliable) void RemoveFromMapMulticast(FMassEntityHandle EntityHandle);
	UFUNCTION(NetMulticast, Reliable) void ChangeBatchMulticast(int Value);
	UFUNCTION(NetMulticast, Reliable) void SetGameIsEndingMulticast(bool Value);

private: /* Private Methods */

	TWeakObjectPtr<UNiagaraComponent> GetNiagaraComponent(uint64 ElementHandle);
	FNiagaraVisualElement* FindElement(uint64 ElementHandle);
	int32 FindElementIndex(uint64 ElementHandle);
	bool ContainsElement(uint64 ElementHandle);

public: /* Public Member variables */

protected: /* Private Member variables */
	UPROPERTY() AClientMassSpawner* ClientMassSpawnerGobborit;
	UPROPERTY() AClientMassSpawner* ClientMassSpawnerNeras;
	UPROPERTY() AClientMassSpawner* ClientMassSpawnerBehemot;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Mass") TArray<FMassSpawnedEntityType> EntityTypesGobborit;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Mass") TArray<FMassSpawnedEntityType> EntityTypesNeras;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Mass") TArray<FMassSpawnedEntityType> EntityTypesBehemot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Mass") TArray<FMassSpawnDataGenerator> SpawnDataGeneratorsGobborit;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Mass") TArray<FMassSpawnDataGenerator> SpawnDataGeneratorsNeras;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Mass") TArray<FMassSpawnDataGenerator> SpawnDataGeneratorsBehemot;

	TArray<FAmalgamClientInitializeInfo> ClientInitializeInfos = TArray<FAmalgamClientInitializeInfo>();
	TArray<FNiagaraVisualElement> ElementArray;
	TArray<FBPVisualElement> BPElementsArray;

	UPROPERTY(EditAnywhere) bool bUseBPVisualisation = false;

	bool bDebugReplicationNumbers = false;
	bool bHideAll = true;
	int CurrentBatch = 0;
	int UnitsByBatch = 50;

	UPROPERTY() AInfernalePawn* InfernalePawn;
	
	FVector LocalPointLocation;
	
	int Radius = 11000;
	bool bGameIsEnding = false;
	
	/*UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> BPVisualisation;*/
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FogOfWarComponent.h"
#include "GameFramework/Actor.h"
#include "RHICommandList.h"
#include <MassEntityTypes.h>

#include "Enums/Enums.h"
#include "FogOfWarManager.generated.h"

UENUM()
enum class VisionType : uint8
{
	Circle,
	Square,
	Cone,
	Ellipse,
	Trapezoid
};

USTRUCT(Blueprintable)
struct FMovingActorVision
{
	GENERATED_BODY()
public:
	FMovingActorVision();
	FMovingActorVision(TWeakObjectPtr<AActor> InActor, float InSightRadius, VisionType InVisionType);

public:
	UPROPERTY() TWeakObjectPtr<AActor> Actor;
	UPROPERTY() float SightRadius;
	UPROPERTY() VisionType VisionType;

};

USTRUCT(Blueprintable)
struct FMassEntityVision
{
	GENERATED_BODY()
public:
	FMassEntityVision();
	FMassEntityVision(float InSightRadius, VisionType InVisionType);

public:
	UPROPERTY() float SightRadius;
	UPROPERTY() VisionType VisionType;

};


USTRUCT()
struct FPositionWithVision
{
	GENERATED_BODY()
public:
	FPositionWithVision();
	FPositionWithVision(int ID, FVector2D InPosition, float InSightRadius, VisionType InVisionType);

public:
	UPROPERTY() int StaticBuildID;
	UPROPERTY() FVector2D Position;
	UPROPERTY() float SightRadius;
	UPROPERTY() VisionType VisionType;
};

UCLASS()
class INFERNALETESTING_API AFogOfWarManager : public AActor
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, Category = "Fog of War")
	ETeam PlayerTeam = ETeam::NatureTeam;
	
	void UpdateTextureRegion(uint8* Data, uint32 InDestX,uint32 InDestY,int32 InSrcX,int32 InSrcY,uint32 InWidth,uint32 InHeight, bool bIsReset);
	UFUNCTION(CallInEditor, Category = "Fog of War")
	void InitFogTexture();

	UPROPERTY(EditAnywhere, Category = "Fog of War")
	float iuhledsrglohiusergiolhjsedgr;

	UFUNCTION(Blueprintable, Category = "Fog of War")
	void FindAllBaseOfTeam(ETeam Team);
	void SetFogVisibilityOfActor(UFogOfWarComponent* Actor, bool bVisibleFog, bool bVisibleActor);

	UFUNCTION(CallInEditor, Category = "Fog of War")
	void FindAllBase();
	// Sets default values for this actor's properties
	AFogOfWarManager();

	UFUNCTION(Category = "Fog of War")
	void AddMovingActorVision(TWeakObjectPtr<AActor> Actor, float SightRadius, VisionType VisionType);
	UFUNCTION(Category = "Fog of War")
	void AddMassEntityVision(FMassEntityHandle Handle, float SightRadius, VisionType VisionType);

	UFUNCTION(Category = "Fog of War")
	bool Contains(FMassEntityHandle Handle);

	UFUNCTION(Category = "Fog of War")
	void RemoveMovingActorVision(TWeakObjectPtr<AActor> Actor);
	UFUNCTION(Category = "Fog of War")
	void RemoveMassEntityVision(FMassEntityHandle Handle);

	UFUNCTION(CallInEditor, Category = "Fog of War")
	void ApplyTextureToMaterial();

	UFUNCTION(BlueprintCallable, Category = "Fog of War")
	int GetAlphaAtPosition0To255(FVector2D Position);

	UFUNCTION(BlueprintCallable, Category = "Fog of War")
	int GetAlphaAtPosition0To1(FVector2D Position);

	UFUNCTION(Category = "Fog of War")
	void ModifyMovingActorVision(TWeakObjectPtr<AActor> Actor, float SightRadius, VisionType VisionType);
	
	UFUNCTION(Category = "Fog of War")
	void ModifyMassEntityVision(FMassEntityHandle Handle, float SightRadius, VisionType VisionType);
	void ModifyStaticActorVision(int ID, float SightRadius, VisionType VisionType);

	void SetMassEntityVision(FMassEntityHandle Handle, const FVector& Location);

	UPROPERTY(EditAnywhere, Category = "Fog of War")
	ETeam debugTeam;

	UPROPERTY(EditAnywhere, Category = "Fog of War")
	int FogResolution = 100;

	UPROPERTY(EditAnywhere, Category = "Fog of War")
	TArray<TWeakObjectPtr<AActor>> EnemyActorsToShowOrHide;

	UPROPERTY(EditAnywhere, Category = "Fog of War")
	TArray<TWeakObjectPtr<AActor>> UnknownBaseActors;

protected:
	// Called when the game starts or when spawned
	UFUNCTION(CallInEditor, Category = "Fog of War")
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void DisableFogOfWar();

	UPROPERTY(EditAnywhere, Category = "Fog of War")
	float UpdateInterval;

	float UpdateTimer;

	UPROPERTY(EditAnywhere, Category = "Fog of War")
	FVector2D MapSize = FVector2D(1000, 1000);

	UPROPERTY(EditAnywhere, Category = "Fog of War")
	UTexture2D* FogTexture;

	UPROPERTY(EditAnywhere, Category = "Fog of War")
	TArray<FMovingActorVision> MovingActors;
	
	UPROPERTY(EditAnywhere, Category = "Fog of War")
	TArray<FMassEntityHandle> MassEntities;
	TMap<uint64, FMassEntityVision> MassVisionData;

	UPROPERTY(EditAnywhere, Category = "Fog of War")
	bool bUseMass = false;

	UPROPERTY(EditAnywhere, Category = "Fog of War")
	TArray<FPositionWithVision> StaticActors;

	UPROPERTY(EditAnywhere, Category = "Fog of War")
	UMaterialInterface* Material;

	UPROPERTY(EditAnywhere, Category = "Fog of War")
	UCurveFloat* FogOpacityCurve;

	UPROPERTY(EditAnywhere, Category = "Fog of War")
	bool UseFogOfWar = false;

	UPROPERTY(EditAnywhere, Category = "Fog of War")
	bool bLocalName = true;
	
	UPROPERTY(EditAnywhere, Category = "Fog of War")
	bool bDebugPlayerTeams = false;

	UPROPERTY(EditAnywhere, Category = "Fog of War")
	UStaticMeshComponent* FogPlane;
	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial;

	UPROPERTY(EditAnywhere, Category = "Fog of War")
	UStaticMesh* InFogMesh;

	FColor Black = FColor(0, 0, 0, 255);
	FColor White = FColor(255, 255, 255, 255);

	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> AllBPMainBuildings;

	static void SmootherTransition(TArray<int32> PixelIndexes);
	UFUNCTION(CallInEditor, Category = "Fog of War")
	void SetStaticActorsVision();
	void SetMovingActorsVision();
	
	UFUNCTION(CallInEditor, Category = "Fog of War")
	void ResetAllTexture();

	UPROPERTY(EditAnywhere, Category = "Fog of War")
	int NumberOfRedPixels;

	uint8* RawImageData;

	TArray<uint8> RawImageDataArray;

	FVector lastCheckPosition = FVector(-9999999, -9999999, -9999999);

	bool isFirstTime = true;
	

public:
	//void SetNumberOfRedPixels();
	void SetEnemyVision();
	void SetVision();
	virtual void Tick(float DeltaTime) override;

};
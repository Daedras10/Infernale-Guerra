// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmbianceManager.generated.h"

class AInfernalePawn;
class ASkyLight;
class USphereComponent;

USTRUCT(Blueprintable)
struct FAmbianceSphere
{
	GENERATED_BODY()
public:
	FAmbianceSphere();
	FAmbianceSphere(USphereComponent* InSphereComponent, FVector InCenter, float InRadius);

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) USphereComponent* SphereComponent;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FVector Center = FVector(0.f, 0.f, 0.f);
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float Radius = 10000.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FLinearColor AmbianceColor = FColor::White;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float AmbianceIntensity = 1.f;
	
};

UCLASS()
class INFERNALETESTING_API AAmbianceManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAmbianceManager();
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void RefreshSphere(const FAmbianceSphere& AmbianceSphere);
	void SpheresVisibility(const bool bVisible);
	void DestroyAmbianceSphere(const FAmbianceSphere& AmbianceSphere);
	void CreateSphereIndex(const int i);
	void GetSkyLight();
	void ChangeTargets(FVector NewPosition);

	UFUNCTION() void SubscribeToPCEvents();
	UFUNCTION() void OnPCMoveVector2(FVector2D _);
	UFUNCTION() void OnPCMoveFloat(float _);
	UFUNCTION() void OnPCMove();
	UFUNCTION(BlueprintCallable) void OnPCMoveBP();

	UFUNCTION(CallInEditor) void RefreshSpheres();
	UFUNCTION(CallInEditor) void MakeSpheresVisible();
	UFUNCTION(CallInEditor) void MakeSpheresInvisible();
	UFUNCTION(CallInEditor) void CreateMissingSpheres();
	UFUNCTION(CallInEditor, Category = "Spawn tool") void CreateSpheres();
	UFUNCTION(CallInEditor, Category = "Spawn tool") void ClearSpheres();
	UFUNCTION(CallInEditor, Category = "Spawn tool") void DestroyLast();
	UFUNCTION(CallInEditor, Category = "Spawn tool") void CreateOneMore();

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) ASkyLight* SkyLight;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) TArray<FAmbianceSphere> AmbianceSpheres = TArray<FAmbianceSphere>();
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float LerpSpeed = 1.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bDebug = false;

	AInfernalePawn* InfernalePawn;
	
	//FAmbianceSphere* CurrentAmbianceSphere = nullptr;
	FLinearColor TargetColor = FLinearColor::White;
	FLinearColor CurrentColor = FLinearColor::White;
	float TargetIntensity = 1;
	float CurrentIntensity = 1;
	

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spawn tool") int SphereToCreate;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spawn tool") float Radius = 10000.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spawn tool") FVector DefaultCenter = FVector(0.f, 0.f, 0.f);

	
};

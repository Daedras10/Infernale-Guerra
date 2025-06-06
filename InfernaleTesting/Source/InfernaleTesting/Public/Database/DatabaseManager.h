// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VaRestSubsystem.h"
#include "VaRestRequestJSON.h"
#include "VaRestTypes.h"
#include "VaRestJsonObject.h"
#include "VaRestJsonValue.h"
#include "DatabaseManager.generated.h"

UCLASS()
class INFERNALETESTING_API ADatabaseManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADatabaseManager();
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	FVaRestCallDelegate VarestDelegate;

	UFUNCTION(BlueprintCallable, Category = "Database")
	void VaRestTest(UVaRestRequestJSON* Request);

	UFUNCTION(BlueprintCallable, Category = "Database")
	void VaRestInit(FString URL);

	UFUNCTION(BlueprintCallable, Category = "Database")
	UVaRestSubsystem* GetVARestSub();

};

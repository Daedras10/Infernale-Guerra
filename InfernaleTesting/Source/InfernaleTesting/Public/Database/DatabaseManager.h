// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VaRestSubsystem.h"
#include "VaRestRequestJSON.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
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
	
	// FVaRestCallDelegate VarestDelegate;
	//
	// UFUNCTION(BlueprintCallable, Category = "Database")
	// void VaRestTest(UVaRestRequestJSON* Request);
	//
	// UFUNCTION(BlueprintCallable, Category = "Database")
	// void VaRestInit(FString URL);
	//
	// UFUNCTION(BlueprintCallable, Category = "Database")
	// UVaRestSubsystem* GetVARestSub();

	UFUNCTION(BlueprintCallable, Category = "Database")
	void FetchDataFromAPI(FString TableName);

	void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	UFUNCTION(BlueprintCallable, Category = "Database")
	FString GetDataFromJson(FString JsonName, FString PrimaryKey, FString Column = "", FString PrimaryKeyValue = "");

private:

	FString CurrentTableName;

};

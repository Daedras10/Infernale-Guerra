// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataGathererActor.h"
#include "Blueprint/UserWidget.h"
#include "Components/Slider.h"
#include "PlayerStatsChartWidget.generated.h"

class UComboBoxString;
struct FPlayerSnapshot;
class ADataGathererActor;


/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UPlayerStatsChartWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart")
	TMap<FString, FPlayerChartData> PlayerDataSeries;

	// Key for time in MM:SS format and value is an array of FBaseCaptureInfo structs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart")
	TMap<FString, FBaseCaptureArray> BaseCaptureDataSeries;

	// Key for time in MM:SS format and value is an array of FAmalgamePresence
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart")
	TMap<FString, FArrayOfAmalgamPresence> AmalgamPresenceDataSeries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart")
	TMap<FString, FLinearColor> PlayerColors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart")
	int32 NumXSubdivisions = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart")
	int32 NumYSubdivisions = 5;

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	UFUNCTION(BlueprintCallable, Category = "Chart")
	void SetupPlayerUIElements();

	virtual void NativeConstruct() override;

	//on destroy empty the memory
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "Chart")
	void OnPlayerCheckboxClicked(bool bIsChecked);
	
	UFUNCTION(BlueprintCallable, Category = "Chart")
	void OnTypeCheckboxClicked(bool bIsChecked);

	FStringPlayerChartData GetDataSeriesAtIndexOfType(int32 Index, EChartGetInfo ChartGetInfo);
	UFUNCTION(BlueprintCallable, Category = "Chart")
	void ActivatePlayerDataSeries(TArray<bool> ActivePlayer);

	FVector2D HoverPosition;
	bool bIsHovering = false;

	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart")
	TArray<FArrayOfUObjects> PlayerObjects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart")
	TArray<FArrayOfUObjects> TypeObjects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart")
	UComboBoxString* ComboBoxString;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChartData")
	TMap<FString, FPlayerChartData> PlayerSoulsInReserveDataSeries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChartData")
	TMap<FString, FPlayerChartData> PlayerSoulsGainDataSeries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChartData")
	TMap<FString, FPlayerChartData> PlayerAmalgamsOnMapPerTeamDataSeries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChartData")
	TMap<FString, FPlayerChartData> PlayerBuildingCountDataSeries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChartData")
	TMap<FString, FPlayerChartData> PlayerDominationPointsDataSeries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart")
	FString JsonRelativePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart")
	bool bUseJsonInSaveFolder = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart")
	EChartGetInfo CurrentActiveInfo = EChartGetInfo::DominationPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart")
	UTexture2D* TopDownMapTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart")
	USlider* TimelineSlider;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart")
	int SliderValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart")
	bool bGraphMode = true;

	void AsyncLoadBaseCaptureDataSeries();
	UFUNCTION(BlueprintCallable, Category = "Chart")
	void SetPlayerSnapshots(EChartGetInfo DefaultInfo = EChartGetInfo::DominationPoints);

	UFUNCTION(BlueprintCallable, Category = "Chart")
	void RefreshGraphAfterComboBoxChange();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart")
	bool isArmyVolume = false;
private:
	
	TArray<FPlayerSnapshot> PlayerSnapshots;

	FString GetPlayerNameFromOwnership(const FDataGathererPlayerInfo& PlayerInfo);
	FString GetPlayerNameFromOwnership(const ETeam& Team);
	void AsyncLoadPlayerDataSeries(TArray<FPlayerSnapshot> Array, EChartGetInfo ChartGetInfo);
	TMap<ETeam, int32> AsyncCountNumberOfAmalgamsOnMapPerTeam(FPlayerSnapshot LocalPlayerSnapshot);

	TArray<bool> ActiveType = { true, true, true }; // Gobborit, Nerras, Behemot

	FLinearColor GetColorFromIndex(int32 Index) const;
};

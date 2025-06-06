// Fill out your copyright notice in the Description page of Project Settings.


#include "LeData/PlayerStatsChartWidget.h"
#include "Kismet/GameplayStatics.h"
#include "SlateOptMacros.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/RichTextBlock.h"
#include "FunctionLibraries/FunctionLibraryInfernale.h"
#include "GameMode/GameInstanceInfernale.h"
#include "Rendering/DrawElements.h"


int32 UPlayerStatsChartWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                                           const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                                           int32 LayerId,
                                           const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// if (bGraphMode)
	// {
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const float Width = Size.X;
	const float Height = Size.Y;

	FSlateDrawElement::MakeLines(
		OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
		{FVector2D(0, Height), FVector2D(Width, Height)}, // X-axis
		ESlateDrawEffect::None, FLinearColor::Gray, true, 1.5f);

	FSlateDrawElement::MakeLines(
		OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
		{FVector2D(0, 0), FVector2D(0, Height)}, // Y-axis
		ESlateDrawEffect::None, FLinearColor::Gray, true, 1.5f);

	for (int32 i = 0; i <= NumXSubdivisions; ++i)
	{
		float Fraction = float(i) / NumXSubdivisions;
		float X = Fraction * Width;

		// Tick mark
		FSlateDrawElement::MakeLines(
			OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform()),
			{FVector2D(X, Height), FVector2D(X, Height + 5)},
			ESlateDrawEffect::None, FLinearColor::Gray, true, 1.0f
		);

		// Label (show as integer index)
		FString Label = FString::Printf(TEXT("%.0f"), (1.0f - Fraction) * 100); // fallback, inverted

		if (PlayerDataSeries.Num() > 0)
		{
			const FPlayerChartData* FirstNonEmptySeries = nullptr;
			for (const auto& Pair : PlayerDataSeries)
			{
				if (Pair.Value.XAxisLabels.Num() > 0)
				{
					FirstNonEmptySeries = &Pair.Value;
					break;
				}
			}
			if (FirstNonEmptySeries)
			{
				const int32 NumLabels = FirstNonEmptySeries->XAxisLabels.Num();
				if (NumLabels > 0)
				{
					int32 LabelIndex = FMath::RoundToInt((1.0f - Fraction) * (NumLabels - 1));
					if (FirstNonEmptySeries->XAxisLabels.IsValidIndex(LabelIndex))
					{
						Label = FirstNonEmptySeries->XAxisLabels[LabelIndex];
					}
				}
			}
		}
		FVector2D LabelSize(30, 15);
		FVector2D LabelPos(X - LabelSize.X / 2.0f, Height + 6);
		FSlateLayoutTransform LabelTransform(LabelPos);
		FSlateDrawElement::MakeText(
			OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2f(LabelSize), LabelTransform),
			FText::FromString(Label), FCoreStyle::GetDefaultFontStyle("SmallFont", 10), ESlateDrawEffect::None,
			FLinearColor::White
		);
	}


	float MaxValue = 0.0f;
	for (const auto& Pair : PlayerDataSeries)
	{
		const TArray<float>& Values = Pair.Value.Values;
		if (Values.Num() < 2) continue;

		for (const float Value : Values)
		{
			MaxValue = FMath::Max(MaxValue, Value);
		}
	}
	MaxValue *= 1.1f;

	// Draw Y-axis subdivisions
	for (int32 i = 0; i <= NumYSubdivisions; ++i)
	{
		float Fraction = float(i) / NumYSubdivisions;
		float Y = Fraction * Height;

		// Tick mark
		FSlateDrawElement::MakeLines(
			OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform()),
			{FVector2D(-5, Y), FVector2D(0, Y)},
			ESlateDrawEffect::None, FLinearColor::Gray, true, 1.0f
		);

		// Label (scale value)
		float Value = MaxValue * (1.0f - Fraction);
		FString Label = FString::Printf(TEXT("%.0f"), Value);
		FVector2D LabelSize(40, 15);
		FVector2D LabelPos(-LabelSize.X - 4, Y - LabelSize.Y / 2.0f);
		FSlateLayoutTransform LabelTransform(LabelPos);
		FSlateDrawElement::MakeText(
			OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2f(LabelSize), LabelTransform),
			FText::FromString(Label), FCoreStyle::GetDefaultFontStyle("SmallFont", 10), ESlateDrawEffect::None,
			FLinearColor::White
		);
	}

	for (const auto& Pair : PlayerDataSeries)
	{
		const TArray<float>& Values = Pair.Value.Values;
		if (Values.Num() < 2) continue;

		TArray<FVector2D> Points;
		for (int32 i = 0; i < Values.Num(); ++i)
		{
			float NormalizedX = float(i) / (Values.Num() - 1);
			float NormalizedY = 1.0f - (Values[i] / MaxValue);

			FVector2D Point(NormalizedX * Width, NormalizedY * Height);
			Points.Add(Point);
		}

		// GEngine->AddOnScreenDebugMessage(-1, 0.2f, FColor::Red,FString::Printf(TEXT(" Number of points: %d"), Points.Num()));

		FLinearColor LineColor = FLinearColor::White;

		if (const FLinearColor* FoundColor = PlayerColors.Find(Pair.Key))
		{
			LineColor = *FoundColor;
		}
		else
		{
			LineColor = FLinearColor::MakeRandomColor();
		}

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			Points,
			ESlateDrawEffect::None,
			LineColor,
			true,
			2.0f
		);

		if (bIsHovering)
		{
			float HoverXNormalized = HoverPosition.X / Width;
			int32 HoverIndex = FMath::Clamp(FMath::RoundToInt(HoverXNormalized * (Values.Num() - 1)), 0,
			                                Values.Num() - 1);

			float ValueAtHover = Values[HoverIndex];
			float NormY = 1.0f - (ValueAtHover / MaxValue);
			// FVector2D HoverPoint(HoverIndex * (Width / (Values.Num() - 1)), NormY * Height); // remonter le point de la moitié de la taille du point
			int size = 8;
			FVector2D HoverPoint(HoverIndex * ((Width - size) / (Values.Num() - 1)), NormY * Height - size / 2);
			FSlateLayoutTransform HoverTransform(HoverPoint);
			// Draw circle at point
			FSlateDrawElement::MakeBox(
				OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(
					FVector2D(size, size), HoverTransform),
				FCoreStyle::Get().GetBrush("WhiteBrush"),
				ESlateDrawEffect::None, LineColor
			);
			// Draw text
			FString HoverText = FString::Printf(TEXT("%.1f"), ValueAtHover);
			FFontOutlineSettings OutlineSettings;
			OutlineSettings.OutlineSize = 1;
			OutlineSettings.OutlineColor = FLinearColor::Red;
			OutlineSettings.OutlineMaterial = nullptr;
			FSlateDrawElement::MakeText(
				OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(
					FVector2D(50, 30), HoverTransform),
				FText::FromString(HoverText), FCoreStyle::GetDefaultFontStyle("SmallFont", 12, OutlineSettings),
				ESlateDrawEffect::None, FLinearColor::White);
		}
	}
	// }
	// else
	// {
	// if (TopDownMapTexture == nullptr)
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("TopDownMapTexture is null in UPlayerStatsChartWidget::NativePaint"));
	// 	return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + 1,
	// 	                          InWidgetStyle, bParentEnabled);
	// }
	// FSlateDrawElement::MakeBox(
	// 	OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
	// 	FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None,
	// 	FLinearColor::Transparent
	// );
	// FVector2D ImageSize = FVector2D(TopDownMapTexture->GetSurfaceWidth(), TopDownMapTexture->GetSurfaceHeight());
	// FVector2D ImagePosition = FVector2D((AllottedGeometry.GetLocalSize().X - ImageSize.X) / 2.0f,
	//                                     (AllottedGeometry.GetLocalSize().Y - ImageSize.Y) / 2.0f);
	// FSlateLayoutTransform ImageTransform(ImagePosition);
	// FSlateBrush* TopDownMapBrush = new FSlateBrush();
	// TopDownMapBrush->SetResourceObject(TopDownMapTexture);
	// FSlateDrawElement::MakeBox(
	// 	OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(ImageSize, ImageTransform),
	// 	TopDownMapBrush, ESlateDrawEffect::None, FLinearColor::White
	// );
	// FVector2D CenterPosition = FVector2D(AllottedGeometry.GetLocalSize().X / 2.0f,
	//                                     AllottedGeometry.GetLocalSize().Y / 2.0f);

	// put circles on the map for each base
	// TArray<FBaseCaptureArray> BaseCaptureArrayTemp;
	// BaseCaptureDataSeries.GenerateValueArray(BaseCaptureArrayTemp);
	// for (const auto& BaseInfos : BaseCaptureArrayTemp[0].BasesInfosArray)
	// {
	// 	FVector2D CirclePosition = FVector2D(BaseInfos.Position.X, BaseInfos.Position.Y) + CenterPosition;
	// 	FSlateLayoutTransform CircleTransform(CirclePosition);
	// 	FLinearColor CircleColor;
	// 	switch (BaseInfos.Team)
	// 	{
	// 	case ETeam::NatureTeam:
	// 		CircleColor = FLinearColor::White;
	// 		break;
	// 	case ETeam::Team1:
	// 		CircleColor = GetColorFromIndex(0);
	// 		break;
	// 	case ETeam::Team2:
	// 		CircleColor = GetColorFromIndex(1);
	// 		break;
	// 	case ETeam::Team3:
	// 		CircleColor = GetColorFromIndex(2);
	// 		break;
	// 	case ETeam::Team4:
	// 		CircleColor = GetColorFromIndex(3);
	// 		break;
	// 	}
	// 	FSlateDrawElement::MakeBox(
	// 		OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(FVector2D(10, 10), CircleTransform),
	// 		FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, CircleColor
	// 	);
	// 	FSlateDrawElement::MakeText(
	// 		OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(FVector2D(10, 10), CircleTransform),
	// 		FText::FromString(BaseInfos.Name), FCoreStyle::GetDefaultFontStyle("SmallFont", 10),
	// 		ESlateDrawEffect::None, FLinearColor::White
	// 	);
	// }

	// }

	return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + 1, InWidgetStyle,
	                          bParentEnabled);
}

void UPlayerStatsChartWidget::SetupPlayerUIElements()
{
	for (const auto& PlayerObject : PlayerObjects)
	{
		PlayerObject.CheckBox->OnCheckStateChanged.AddDynamic(this, &UPlayerStatsChartWidget::OnPlayerCheckboxClicked);
	}
	for (const auto& TypeObject : TypeObjects)
	{
		TypeObject.CheckBox->OnCheckStateChanged.AddDynamic(this, &UPlayerStatsChartWidget::OnTypeCheckboxClicked);
	}
	int NumPlayerObjects = PlayerObjects.Num();
	if (NumPlayerObjects == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No player objects found in UPlayerStatsChartWidget::SetupPlayerUIElements"));
		return;
	}
	TArray<FLinearColor> PlayerColorsTemp;
	PlayerColors.GenerateValueArray(PlayerColorsTemp);
	if (PlayerColorsTemp.Num() > NumPlayerObjects)
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("Not enough player colors found in UPlayerStatsChartWidget::SetupPlayerUIElements"));
		GEngine->AddOnScreenDebugMessage(
			-1, 509.f, FColor::Red,
			FString::Printf(TEXT("Not enough player colors found in UPlayerStatsChartWidget::SetupPlayerUIElements expected: %d, found: %d"),
			                NumPlayerObjects, PlayerColorsTemp.Num()));
		return;
	}
	TArray<FString> PlayerNamesTemp;
	PlayerColors.GenerateKeyArray(PlayerNamesTemp);
	if (PlayerNamesTemp.Num() > NumPlayerObjects)
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("Not enough player names found in UPlayerStatsChartWidget::SetupPlayerUIElements"));
		GEngine->AddOnScreenDebugMessage(
			-1, 509.f, FColor::Red,
			FString::Printf(TEXT("Not enough player colors found in UPlayerStatsChartWidget::SetupPlayerUIElements expected: %d, found: %d"),
			                NumPlayerObjects, PlayerColorsTemp.Num()));
		return;
	}
	for (int32 i = 0; i < PlayerColorsTemp.Num(); ++i)
	{
		const auto& PlayerObject = PlayerObjects[i];
		PlayerObject.CheckBox->SetVisibility(ESlateVisibility::Visible);
		PlayerObject.CheckBox->SetIsChecked(true);

		PlayerObject.HorizontalBox->SetVisibility(ESlateVisibility::Visible);

		PlayerObject.Image->SetVisibility(ESlateVisibility::Visible);
		PlayerObject.Image->SetColorAndOpacity(PlayerColorsTemp[i]);

		PlayerObject.RichTextBlock->SetVisibility(ESlateVisibility::Visible);
		PlayerObject.RichTextBlock->SetText(FText::FromString(PlayerNamesTemp[i]));
	}
	PlayerColorsTemp.Empty();
	PlayerNamesTemp.Empty();

	// TimelineSlider->SetMaxValue(BaseCaptureDataSeries.Num());
}

void UPlayerStatsChartWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetupPlayerUIElements();
}

void UPlayerStatsChartWidget::NativeDestruct()
{
	Super::NativeDestruct();
	PlayerDataSeries.Empty();
	PlayerSoulsInReserveDataSeries.Empty();
	PlayerSoulsGainDataSeries.Empty();
	PlayerAmalgamsOnMapPerTeamDataSeries.Empty();
	PlayerBuildingCountDataSeries.Empty();
	PlayerDominationPointsDataSeries.Empty();
	PlayerObjects.Empty();
	PlayerColors.Empty();
	PlayerSnapshots.Empty();
}

void UPlayerStatsChartWidget::OnPlayerCheckboxClicked(bool bIsChecked)
{
	TArray<bool> temp;
	for (const auto& PlayerObject : PlayerObjects)
	{
		if (PlayerObject.CheckBox->IsChecked())
		{
			PlayerObject.CheckBox->SetCheckedState(ECheckBoxState::Checked);
			temp.Add(true);
		}
		else
		{
			PlayerObject.CheckBox->SetCheckedState(ECheckBoxState::Unchecked);
			temp.Add(false);
		}
	}
	ActivatePlayerDataSeries(temp);
	temp.Empty();
}

void UPlayerStatsChartWidget::OnTypeCheckboxClicked(bool bIsChecked)
{
	size_t count = 0;
	for (const auto& TypeObject : TypeObjects)
	{
		if (TypeObject.CheckBox->IsChecked())
		{
			TypeObject.CheckBox->SetCheckedState(ECheckBoxState::Checked);
			ActiveType[count] = true;
		}
		else
		{
			TypeObject.CheckBox->SetCheckedState(ECheckBoxState::Unchecked);
			ActiveType[count] = false;
		}
		UE_LOG(LogTemp, Warning, TEXT("ActiveType[%llu] = %d"), count, ActiveType[count]);
		count++;
	}
	RefreshGraphAfterComboBoxChange();
}

FStringPlayerChartData UPlayerStatsChartWidget::GetDataSeriesAtIndexOfType(int32 Index, EChartGetInfo ChartGetInfo)
{
	TArray<FString> PlayerNamesTemp;
	FStringPlayerChartData Data = FStringPlayerChartData();
	PlayerDataSeries.GenerateKeyArray(PlayerNamesTemp);
	switch (ChartGetInfo)
	{
	case EChartGetInfo::SoulsInReserve:
		// PlayerSoulsInReserveDataSeries in this find the Key using the PlayerNameTemp[Index]
		if (PlayerSoulsInReserveDataSeries.Contains(PlayerNamesTemp[Index]))
		{
			Data.PlayerChartData = PlayerSoulsInReserveDataSeries[PlayerNamesTemp[Index]];
			Data.String = PlayerNamesTemp[Index];
		}
		break;
	case EChartGetInfo::SoulsGain:
		if (PlayerSoulsGainDataSeries.Contains(PlayerNamesTemp[Index]))
		{
			Data.PlayerChartData = PlayerSoulsGainDataSeries[PlayerNamesTemp[Index]];
			Data.String = PlayerNamesTemp[Index];
		}
		break;
	case EChartGetInfo::BuildingCount:
		if (PlayerBuildingCountDataSeries.Contains(PlayerNamesTemp[Index]))
		{
			Data.PlayerChartData = PlayerBuildingCountDataSeries[PlayerNamesTemp[Index]];
			Data.String = PlayerNamesTemp[Index];
		}
		break;
	case EChartGetInfo::AmalgamsOnMapPerTeam:
		if (PlayerAmalgamsOnMapPerTeamDataSeries.Contains(PlayerNamesTemp[Index]))
		{
			Data.PlayerChartData = PlayerAmalgamsOnMapPerTeamDataSeries[PlayerNamesTemp[Index]];
			Data.String = PlayerNamesTemp[Index];
		}
		break;
	case EChartGetInfo::DominationPoints:
		if (PlayerDominationPointsDataSeries.Contains(PlayerNamesTemp[Index]))
		{
			Data.PlayerChartData = PlayerDominationPointsDataSeries[PlayerNamesTemp[Index]];
			Data.String = PlayerNamesTemp[Index];
		}
		break;
	case EChartGetInfo::ArmyVolumePerTeam:
		if (PlayerAmalgamsOnMapPerTeamDataSeries.Contains(PlayerNamesTemp[Index]))
		{
			Data.PlayerChartData = PlayerAmalgamsOnMapPerTeamDataSeries[PlayerNamesTemp[Index]];
			Data.String = PlayerNamesTemp[Index];
		}
		break;
	}

	return Data;
}

void UPlayerStatsChartWidget::ActivatePlayerDataSeries(TArray<bool> ActivePlayer)
{
	for (int32 i = 0; i < ActivePlayer.Num(); ++i)
	{
		if (i >= PlayerDataSeries.Num()) break;
		if (ActivePlayer[i])
		{
			FStringPlayerChartData Data = GetDataSeriesAtIndexOfType(i, CurrentActiveInfo);
			PlayerDataSeries.Add(Data.String, Data.PlayerChartData);
		}
		else
		{
			FStringPlayerChartData Data = GetDataSeriesAtIndexOfType(i, CurrentActiveInfo);
			// PlayerDataSeries.Remove(Data.String);
			if (PlayerDataSeries.Contains(Data.String))
			{
				PlayerDataSeries[Data.String] = FPlayerChartData();
			}
		}
	}
	Invalidate(EInvalidateWidgetReason::Paint);
}

FReply UPlayerStatsChartWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	HoverPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	bIsHovering = true;
	Invalidate(EInvalidateWidgetReason::Paint);
	return FReply::Handled();
}

void UPlayerStatsChartWidget::AsyncLoadBaseCaptureDataSeries()
{
	FCriticalSection Mutex;
	TMap<FString, FBaseCaptureArray> TempBaseCaptureDataSeries;
	ParallelFor(PlayerSnapshots.Num(), [&](int32 Index)
	{
		FPlayerSnapshot LocalPlayerSnapshot = PlayerSnapshots[Index];
		FString XAxisLabel = LocalPlayerSnapshot.TimeMMSS;
		FBaseCaptureArray BaseCaptureArray;
		for (const auto& BaseInfos : LocalPlayerSnapshot.BasesInfos)
		{
			BaseCaptureArray.BasesInfosArray.Add(BaseInfos);
			//top left is -29000 -29000 bottom right is 29000 29000 normalize these to the size of the TopDownMapTexture
			BaseCaptureArray.BasesInfosArray.Last().Position = FVector(
				(BaseInfos.Position.X + 29000) / 58000 * TopDownMapTexture->GetSurfaceWidth(),
				(BaseInfos.Position.Y + 29000) / 58000 * TopDownMapTexture->GetSurfaceHeight(),
				0.0f
			);
		}
		FScopeLock Lock(&Mutex);
		TempBaseCaptureDataSeries.Add(XAxisLabel, BaseCaptureArray);
	});

	// Sort TempBaseCaptureDataSeries by XAxisLabel (descending)
	TArray<FString> SortedKeys;
	TempBaseCaptureDataSeries.GetKeys(SortedKeys);
	SortedKeys.Sort([](const FString& A, const FString& B)
	{
		return A > B;
	});

	TMap<FString, FBaseCaptureArray> SortedBaseCaptureDataSeries;
	for (const FString& Key : SortedKeys)
	{
		SortedBaseCaptureDataSeries.Add(Key, TempBaseCaptureDataSeries[Key]);
	}
	BaseCaptureDataSeries = SortedBaseCaptureDataSeries;
}

void UPlayerStatsChartWidget::SetPlayerSnapshots(EChartGetInfo DefaultInfo)
{
	if (bUseJsonInSaveFolder) JsonRelativePath = UKismetSystemLibrary::GetProjectSavedDirectory() + JsonRelativePath;
	PlayerSnapshots = UFunctionLibraryInfernale::GetPlayerSnapshotsFromJsonFromPath(JsonRelativePath);
	if (!PlayerSnapshots.IsValidIndex(0))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error : \n\t Unable to find PlayerSnapshots"));
		return;
	}
	AsyncLoadPlayerDataSeries(PlayerSnapshots, EChartGetInfo::SoulsInReserve);
	AsyncLoadPlayerDataSeries(PlayerSnapshots, EChartGetInfo::SoulsGain);
	AsyncLoadPlayerDataSeries(PlayerSnapshots, EChartGetInfo::BuildingCount);
	AsyncLoadPlayerDataSeries(PlayerSnapshots, EChartGetInfo::AmalgamsOnMapPerTeam);
	AsyncLoadPlayerDataSeries(PlayerSnapshots, EChartGetInfo::DominationPoints);

	switch (DefaultInfo)
	{
	case EChartGetInfo::SoulsInReserve:
		PlayerDataSeries = PlayerSoulsInReserveDataSeries;
		break;
	case EChartGetInfo::SoulsGain:
		PlayerDataSeries = PlayerSoulsGainDataSeries;
		break;
	case EChartGetInfo::BuildingCount:
		PlayerDataSeries = PlayerBuildingCountDataSeries;
		break;
	case EChartGetInfo::AmalgamsOnMapPerTeam:
		PlayerDataSeries = PlayerAmalgamsOnMapPerTeamDataSeries;
		break;
	case EChartGetInfo::DominationPoints:
		PlayerDataSeries = PlayerDominationPointsDataSeries;
		break;
	case EChartGetInfo::ArmyVolumePerTeam:
		PlayerDataSeries = PlayerAmalgamsOnMapPerTeamDataSeries;
		break;
	}

	AsyncLoadBaseCaptureDataSeries();
}

void UPlayerStatsChartWidget::RefreshGraphAfterComboBoxChange()
{
	switch (CurrentActiveInfo)
	{
	case EChartGetInfo::SoulsInReserve:
		PlayerDataSeries = PlayerSoulsInReserveDataSeries;
		break;
	case EChartGetInfo::SoulsGain:
		PlayerDataSeries = PlayerSoulsGainDataSeries;
		break;
	case EChartGetInfo::BuildingCount:
		PlayerDataSeries = PlayerBuildingCountDataSeries;
		break;
	case EChartGetInfo::AmalgamsOnMapPerTeam:
		AsyncLoadPlayerDataSeries(PlayerSnapshots, EChartGetInfo::AmalgamsOnMapPerTeam);
		PlayerDataSeries = PlayerAmalgamsOnMapPerTeamDataSeries;
		break;
	case EChartGetInfo::DominationPoints:
		PlayerDataSeries = PlayerDominationPointsDataSeries;
		break;
	case EChartGetInfo::ArmyVolumePerTeam:
		AsyncLoadPlayerDataSeries(PlayerSnapshots, EChartGetInfo::AmalgamsOnMapPerTeam);
		PlayerDataSeries = PlayerAmalgamsOnMapPerTeamDataSeries;
		break;
	}
	for (const auto& PlayerObject : PlayerObjects)
	{
		PlayerObject.CheckBox->OnCheckStateChanged.Broadcast(PlayerObject.CheckBox->IsChecked());
	}
	Invalidate(EInvalidateWidgetReason::Paint);
}

FString UPlayerStatsChartWidget::GetPlayerNameFromOwnership(const FDataGathererPlayerInfo& PlayerInfo)
{
	FString PlayerName = "";
	const auto GameInstanceInfernale = Cast<UGameInstanceInfernale>(UGameplayStatics::GetGameInstance(GetWorld()));
	const auto OpponentInfos = GameInstanceInfernale->OpponentPlayerInfo;
	for (const auto& OpponentInfo : OpponentInfos)
	{
		if (OpponentInfo.PlayerOwnerInfo.Player != PlayerInfo.Owner.Player) continue;
		return OpponentInfo.PlayerName;
	}
	return "Unknown Player";
}

FString UPlayerStatsChartWidget::GetPlayerNameFromOwnership(const ETeam& Team)
{
	FString PlayerName = "";
	const auto GameInstanceInfernale = Cast<UGameInstanceInfernale>(UGameplayStatics::GetGameInstance(GetWorld()));
	const auto OpponentInfos = GameInstanceInfernale->OpponentPlayerInfo;
	for (const auto& OpponentInfo : OpponentInfos)
	{
		if (OpponentInfo.PlayerOwnerInfo.Team != Team) continue;
		return OpponentInfo.PlayerName;
	}
	return "Unknown Player";
}

TMap<ETeam, int32> UPlayerStatsChartWidget::AsyncCountNumberOfAmalgamsOnMapPerTeam(FPlayerSnapshot LocalPlayerSnapshot)
{
	TMap<ETeam, int32> AmalgamsCount;
	for (const auto& AmalgamsPositionsTeamAndType : LocalPlayerSnapshot.AmalgamPositionsAndType)
	{
		int Multiplier = 1;
		if (!isArmyVolume)
		{
			if (AmalgamsPositionsTeamAndType.Type == "Gobborit") Multiplier = 121;
			else if (AmalgamsPositionsTeamAndType.Type == "Nerras") Multiplier = 7;
			else if (AmalgamsPositionsTeamAndType.Type == "Behemot") Multiplier = 11;
		}
		if (AmalgamsPositionsTeamAndType.Type == "Gobborit" && ActiveType[0])
			AmalgamsCount.FindOrAdd(AmalgamsPositionsTeamAndType.Team) += AmalgamsPositionsTeamAndType.Positions.Num() *
				Multiplier;
		else if (AmalgamsPositionsTeamAndType.Type == "Nerras" && ActiveType[1])
			AmalgamsCount.FindOrAdd(AmalgamsPositionsTeamAndType.Team) += AmalgamsPositionsTeamAndType.Positions.Num() *
				Multiplier;
		else if (AmalgamsPositionsTeamAndType.Type == "Behemot" && ActiveType[2])
			AmalgamsCount.FindOrAdd(AmalgamsPositionsTeamAndType.Team) += AmalgamsPositionsTeamAndType.Positions.Num() *
				Multiplier;
	}
	return AmalgamsCount;
}

FLinearColor UPlayerStatsChartWidget::GetColorFromIndex(int32 Index) const
{
	if (PlayerColors.Num() == 0) return FLinearColor::White;

	TArray<FString> PlayerNamesTemp;
	PlayerColors.GenerateKeyArray(PlayerNamesTemp);
	if (Index < 0 || Index >= PlayerNamesTemp.Num()) return FLinearColor::White;

	const FString& PlayerName = PlayerNamesTemp[Index];
	if (const FLinearColor* FoundColor = PlayerColors.Find(PlayerName))
	{
		return *FoundColor;
	}
	return FLinearColor::MakeRandomColor();
}


void UPlayerStatsChartWidget::AsyncLoadPlayerDataSeries(TArray<FPlayerSnapshot> Array, EChartGetInfo ChartGetInfo)
{
	// Prepare a thread-safe array to collect results
	TMap<FString, FPlayerChartData> TempDataSeries;
	FCriticalSection Mutex;
	ParallelFor(Array.Num(), [&](int32 Index)
	{
		const auto& PlayerSnapshot = Array[Index];
		if (ChartGetInfo == EChartGetInfo::AmalgamsOnMapPerTeam)
		{
			TMap<ETeam, int32> AmalgamsOnMapOfTeam = AsyncCountNumberOfAmalgamsOnMapPerTeam(PlayerSnapshot);
			for (const auto& AmalgamCount : AmalgamsOnMapOfTeam)
			{
				FPlayerChartData ChartData;
				ChartData.XAxisLabels.Add(PlayerSnapshot.TimeMMSS);
				ChartData.Values.Add(AmalgamCount.Value);
				const FString PlayerName = GetPlayerNameFromOwnership(AmalgamCount.Key);
				FScopeLock Lock(&Mutex);
				if (TempDataSeries.Contains(PlayerName))
				{
					TempDataSeries[PlayerName].XAxisLabels.Add(PlayerSnapshot.TimeMMSS);
					TempDataSeries[PlayerName].Values.Add(AmalgamCount.Value);
				}
				else
				{
					TempDataSeries.Add(PlayerName, ChartData);
				}
			}
		}
		else
		{
			for (const auto& PlayerInfo : PlayerSnapshot.PlayerInfos)
			{
				if (PlayerInfo.Owner.Team == ETeam::NatureTeam) continue;
				FPlayerChartData ChartData;
				ChartData.XAxisLabels.Add(PlayerSnapshot.TimeMMSS);
				switch (ChartGetInfo)
				{
				case EChartGetInfo::AmalgamsOnMapPerTeam:
					break;
				case EChartGetInfo::SoulsInReserve:
					ChartData.Values.Add(PlayerInfo.SoulsInReserve);
					break;
				case EChartGetInfo::SoulsGain:
					ChartData.Values.Add(
						(PlayerInfo.SoulsFromIncome + PlayerInfo.SoulsFromMonsters + PlayerInfo.SoulsFromBeacons) /
						5.f);
					break;
				case EChartGetInfo::BuildingCount:
					ChartData.Values.Add(PlayerInfo.NumberOfBases);
					break;
				case EChartGetInfo::DominationPoints:
					ChartData.Values.Add(PlayerInfo.DominationPoints);
					break;
				case EChartGetInfo::ArmyVolumePerTeam:
					break;
				}

				const FString PlayerName = GetPlayerNameFromOwnership(PlayerInfo);

				FScopeLock Lock(&Mutex);
				if (TempDataSeries.Contains(PlayerName))
				{
					TempDataSeries[PlayerName].XAxisLabels.Add(PlayerSnapshot.TimeMMSS);
					switch (ChartGetInfo)
					{
					case EChartGetInfo::SoulsInReserve:
						TempDataSeries[PlayerName].Values.Add(PlayerInfo.SoulsInReserve);
						break;
					case EChartGetInfo::SoulsGain:
						TempDataSeries[PlayerName].Values.Add(
							(PlayerInfo.SoulsFromIncome + PlayerInfo.SoulsFromMonsters + PlayerInfo.SoulsFromBeacons) /
							5.f);
						break;
					case EChartGetInfo::BuildingCount:
						TempDataSeries[PlayerName].Values.Add(PlayerInfo.NumberOfBases);
						break;
					case EChartGetInfo::DominationPoints:
						TempDataSeries[PlayerName].Values.Add(PlayerInfo.DominationPoints);
						break;
					default:
						return;
					}
				}
				else
				{
					TempDataSeries.Add(PlayerName, ChartData);
				}
			}
		}
	});


	// Now safely assign to the member variable on the main thread
	// PlayerDominationPointsDataSeries = TempDataSeries;
	TMap<FString, FPlayerChartData> SortedDataSeries;
	switch (ChartGetInfo)
	{
	case EChartGetInfo::SoulsInReserve:
		SortedDataSeries = TempDataSeries;
		break;
	case EChartGetInfo::SoulsGain:
		SortedDataSeries = TempDataSeries;
		break;
	case EChartGetInfo::BuildingCount:
		SortedDataSeries = TempDataSeries;
		break;
	case EChartGetInfo::AmalgamsOnMapPerTeam:
		SortedDataSeries = TempDataSeries;
		break;
	case EChartGetInfo::DominationPoints:
		SortedDataSeries = TempDataSeries;
		break;
	case EChartGetInfo::ArmyVolumePerTeam:
		break;
	}

	// If you need to sort, do it here, but do NOT remove/add while iterating
	for (auto& Pair : SortedDataSeries)
	{
		// Sort XAxisLabels and Values together if needed
		TArray<int32> Indices;
		Indices.SetNum(Pair.Value.XAxisLabels.Num());
		for (int32 i = 0; i < Indices.Num(); ++i) Indices[i] = i;
		Indices.Sort([&](int32 A, int32 B)
		{
			return Pair.Value.XAxisLabels[A] > Pair.Value.XAxisLabels[B];
		});

		TArray<FString> SortedLabels;
		TArray<float> SortedValues;
		for (int32 i : Indices)
		{
			SortedLabels.Add(Pair.Value.XAxisLabels[i]);
			SortedValues.Add(Pair.Value.Values[i]);
		}
		Pair.Value.XAxisLabels = SortedLabels;
		Pair.Value.Values = SortedValues;
	}

	switch (ChartGetInfo)
	{
	case EChartGetInfo::SoulsInReserve:
		PlayerSoulsInReserveDataSeries = SortedDataSeries;
		break;
	case EChartGetInfo::SoulsGain:
		PlayerSoulsGainDataSeries = SortedDataSeries;
		break;
	case EChartGetInfo::BuildingCount:
		PlayerBuildingCountDataSeries = SortedDataSeries;
		break;
	case EChartGetInfo::AmalgamsOnMapPerTeam:
		PlayerAmalgamsOnMapPerTeamDataSeries = SortedDataSeries;
		break;
	case EChartGetInfo::DominationPoints:
		PlayerDominationPointsDataSeries = SortedDataSeries;
		break;
	case EChartGetInfo::ArmyVolumePerTeam:
		PlayerAmalgamsOnMapPerTeamDataSeries = SortedDataSeries;
		break;
	}

	TempDataSeries.Empty();
	SortedDataSeries.Empty();
}

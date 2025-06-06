// Fill out your copyright notice in the Description page of Project Settings.


#include "FunctionLibraries/FunctionLibraryInfernale.h"

#include "Component/GameModeComponent/VictoryManagerComponent.h"
#include "DataAsset/GameSettingsDataAsset.h"
#include "GameMode/GameInstanceInfernale.h"
#include "GameMode/Infernale/PlayerControllerInfernale.h"
#include "Kismet/GameplayStatics.h"
#include "Manager/UnitActorManager.h"
#include "Mass/Army/AmalgamFragments.h"

ETeam UFunctionLibraryInfernale::GetNextTeam(ETeam CurrentTeam)
{
	switch (CurrentTeam)
	{
		case ETeam::NatureTeam:
			return ETeam::Team1;
		case ETeam::Team1:
			return ETeam::Team2;
		case ETeam::Team2:
			return ETeam::Team3;
		case ETeam::Team3:
			return ETeam::Team4;
		case ETeam::Team4:
			return ETeam::NatureTeam;
		default:
			return ETeam::NatureTeam;
	}
}

ETeam UFunctionLibraryInfernale::GetPreviousTeam(ETeam CurrentTeam)
{
	switch (CurrentTeam)
	{
		case ETeam::NatureTeam:
			return ETeam::Team4;
		case ETeam::Team1:
			return ETeam::NatureTeam;
		case ETeam::Team2:
			return ETeam::Team1;
		case ETeam::Team3:
			return ETeam::Team2;
		case ETeam::Team4:
			return ETeam::Team3;
		default:
			return ETeam::NatureTeam;
	}
}

EPlayerOwning UFunctionLibraryInfernale::GetNextPlayerOwning(EPlayerOwning CurrentPlayerOwning)
{
	switch (CurrentPlayerOwning)
	{
		case EPlayerOwning::Nature:
			return EPlayerOwning::Player1;
		case EPlayerOwning::Player1:
			return EPlayerOwning::Player2;
		case EPlayerOwning::Player2:
			return EPlayerOwning::Player3;
		case EPlayerOwning::Player3:
			return EPlayerOwning::Player4;
		case EPlayerOwning::Player4:
			return EPlayerOwning::Nature;
		default:
			return EPlayerOwning::Nature;
	}
}

EPlayerOwning UFunctionLibraryInfernale::GetPreviousPlayerOwning(EPlayerOwning CurrentPlayerOwning)
{
	switch (CurrentPlayerOwning)
	{
		case EPlayerOwning::Nature:
			return EPlayerOwning::Player4;
		case EPlayerOwning::Player1:
			return EPlayerOwning::Nature;
		case EPlayerOwning::Player2:
			return EPlayerOwning::Player1;
		case EPlayerOwning::Player3:
			return EPlayerOwning::Player2;
		case EPlayerOwning::Player4:
			return EPlayerOwning::Player3;
		default:
			return EPlayerOwning::Nature;
	}
}

FString UFunctionLibraryInfernale::GetRomanStringFromNumber(int Number)
{
	switch (Number)
	{
	case 1:
		return "I";
	case 2:
		return "II";
	case 3:
		return "II";
	case 4:
		return "IV";
	case 5:
    		return "V";
	case 6:
		return "VI";
	default: return "?";
	}
}

bool UFunctionLibraryInfernale::TryGetUnitActorManager(AUnitActorManager*& OutSpawnerManager, TSubclassOf<AUnitActorManager> UnitActorManagerClass)
{
	const UWorld* World = GEngine->GameViewport->GetWorld(); 
	auto UnitActorManagerArray = TArray<AActor*>();
	UGameplayStatics::GetAllActorsOfClass(World, UnitActorManagerClass, UnitActorManagerArray);
	if (UnitActorManagerArray.Num() == 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error : \n\t Unable to find UnitActorManager"));
		return false;
	}
	OutSpawnerManager = Cast<AUnitActorManager>(UnitActorManagerArray[0]);
	if (!OutSpawnerManager)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error : \n\t Unable to cast UnitActorManager"));
		return false;
	}
	return true;
}

FLinearColor UFunctionLibraryInfernale::GetOldTeamColorCpp(ETeam Team)
{
	switch (Team) {
	case ETeam::NatureTeam:
		return FLinearColor(0.67f, 0.67f, 0.67f, 1.00f);
	case ETeam::Team1:
		return FLinearColor(0.64f, 1.00f, 0.38f, 1.00f);
	case ETeam::Team2:
		return FLinearColor(0.00f, 0.73f, 1.00f, 1.00f);
	case ETeam::Team3:
		return FLinearColor(1.00f, 0.47f, 0.00f, 1.00f);
	case ETeam::Team4:
		return FLinearColor(0.84f, 0.40f, 1.00f, 1.00f);
	}
	return FLinearColor(0, 0, 0, 1);
}

FLinearColor UFunctionLibraryInfernale::GetTeamColorCpp(ETeam Team, EEntityType EntityType)
{
	switch (Team) {
		case ETeam::NatureTeam:
			{
				switch (EntityType)
				{
					case EEntityType::EntityTypeGobborit:
						return FLinearColor(1, 1, 1, 1);
					case EEntityType::EntityTypeNerras:
						return FLinearColor(1, 1, 1, 1);
					case EEntityType::EntityTypeBehemot:
						return FLinearColor(1, 1, 1, 1);
					default: return FLinearColor(0, 0, 0, 1);
				}
			}
		case ETeam::Team1:
			switch (EntityType)
			{
				case EEntityType::EntityTypeGobborit:
					return FLinearColor(0.076185, 0.23074, 0.114435, 1);
				case EEntityType::EntityTypeNerras:
					return FLinearColor(0.116971, 0.254152, 0.109462, 1);
				case EEntityType::EntityTypeBehemot:
					return FLinearColor(0.019382, 0.141263, 0.066626, 1);
				default: return FLinearColor(0, 0, 0, 1);
			}
		case ETeam::Team2:
			switch (EntityType)
			{
			case EEntityType::EntityTypeGobborit:
				return FLinearColor(0.165132, 0.584078, 0.571125, 1);
			case EEntityType::EntityTypeNerras:
				return FLinearColor(0.337164, 0.730461, 1, 1);
			case EEntityType::EntityTypeBehemot:
				return FLinearColor(0, 0.07036, 0.088656, 1);
			default: return FLinearColor(0, 0, 0, 1);
			}
		case ETeam::Team3:
			switch (EntityType)
			{
			case EEntityType::EntityTypeGobborit:
				return FLinearColor(1, 0.47932, 0, 1);
			case EEntityType::EntityTypeNerras:
				return FLinearColor(0.300544, 0.14996, 0.051269, 1);
			case EEntityType::EntityTypeBehemot:
				return FLinearColor(0.191202, 0.090842, 0, 1);
			default: return FLinearColor(0, 0, 0, 1);
			}
		case ETeam::Team4:
			switch (EntityType)
			{
			case EEntityType::EntityTypeGobborit:
				return FLinearColor(0.168269, 0.040915, 0.323143, 1);
			case EEntityType::EntityTypeNerras:
				return FLinearColor(0.351533, 0.064803, 0.3564, 1);
			case EEntityType::EntityTypeBehemot:
				return FLinearColor(0.097587, 0.046665, 0.0181164, 1);
			default: return FLinearColor(0, 0, 0, 1);
			}
	}
	return FLinearColor(0, 0, 0, 1);
}

FLinearColor UFunctionLibraryInfernale::GetTeamColorEmissiveCpp(ETeam Team, EEntityType EntityType)
{
	switch (Team) {
	case ETeam::NatureTeam:
		{
			switch (EntityType)
			{
			case EEntityType::EntityTypeGobborit:
				return FLinearColor(1, 1, 1, 1);
			case EEntityType::EntityTypeNerras:
				return FLinearColor(1, 1, 1, 1);
			case EEntityType::EntityTypeBehemot:
				return FLinearColor(1, 1, 1, 1);
			default: return FLinearColor(0, 0, 0, 1);
			}
		}
	case ETeam::Team1:
		switch (EntityType)
		{
	case EEntityType::EntityTypeGobborit:
		return FLinearColor(0.07036, 1, 0, 1);
	case EEntityType::EntityTypeNerras:
		return FLinearColor(0.396755, 1, 0.401978, 1);
	case EEntityType::EntityTypeBehemot:
		return FLinearColor(0.14996, 1, 0.313989, 1);
	default: return FLinearColor(0, 0, 0, 1);
		}
	case ETeam::Team2:
		switch (EntityType)
		{
	case EEntityType::EntityTypeGobborit:
		return FLinearColor(0, 0.799103, 1, 1);
	case EEntityType::EntityTypeNerras:
		return FLinearColor(0.246201, 0.672443, 1, 1);
	case EEntityType::EntityTypeBehemot:
		return FLinearColor(0, 0.181164, 1, 1);
	default: return FLinearColor(0, 0, 0, 1);
		}
	case ETeam::Team3:
		switch (EntityType)
		{
	case EEntityType::EntityTypeGobborit:
		return FLinearColor(1, 0.571125, 0, 1);
	case EEntityType::EntityTypeNerras:
		return FLinearColor(1, 0.520996, 0.124772, 1);
	case EEntityType::EntityTypeBehemot:
		return FLinearColor(1, 0.274677, 0, 1);
	default: return FLinearColor(0, 0, 0, 1);
		}
	case ETeam::Team4:
		switch (EntityType)
		{
	case EEntityType::EntityTypeGobborit:
		return FLinearColor(0.291771, 0.0865, 1, 1);
	case EEntityType::EntityTypeNerras:
		return FLinearColor(0.637597, 0.291771, 1, 1);
	case EEntityType::EntityTypeBehemot:
		return FLinearColor(0.177888, 0, 1, 1);
	default: return FLinearColor(0, 0, 0, 1);
		}
	}
	return FLinearColor(0, 0, 0, 1);
}

UStaticMesh* UFunctionLibraryInfernale::GetMeshFromId(const FString Id)
{
	const auto GameSettingsDataAsset = GetGameSettingsDataAsset();
	return GetMeshFromId(Id, GameSettingsDataAsset);
	
}

UStaticMesh* UFunctionLibraryInfernale::GetMeshFromId(const FString Id, UGameSettingsDataAsset* GameSettingsDataAsset)
{
	if (!GameSettingsDataAsset) return nullptr;
	const auto MeshesById = GameSettingsDataAsset->MeshesById;
	for (const auto MeshById : MeshesById)
	{
		if (Id.Equals(MeshById.MeshId)) return MeshById.Mesh;
	}
	return nullptr;
}

bool UFunctionLibraryInfernale::InEditor()
{
#if WITH_EDITOR
	return true;
#else
	return false;
#endif
}

UGameSettingsDataAsset* UFunctionLibraryInfernale::GetGameSettingsDataAsset()
{
	const UWorld* World = GEngine->GameViewport->GetWorld(); 
	const auto GameInstance = UGameplayStatics::GetGameInstance(World);
	const auto GameInstanceInfernale = Cast<UGameInstanceInfernale>(GameInstance);

	return GameInstanceInfernale->GetGameSettingsDataAsset();
}

FString UFunctionLibraryInfernale::GetTimeRemaining(const FTimeRemaining TimeRemaining)
{
	float Minutes = FMath::FloorToInt(TimeRemaining.TimeRemaining / 60);
	float Seconds = FMath::FloorToInt(FMath::Fmod(TimeRemaining.TimeRemaining, 60));
	FString TimeRemainingString = FString::Printf(TEXT("%02d:%02d"), static_cast<int>(Minutes), static_cast<int>(Seconds));
	return TimeRemainingString;
}

ECollisionChannel UFunctionLibraryInfernale::GetCustomTraceChannel(ECustomTraceChannel CustomTraceChannel)
{
	switch (CustomTraceChannel)
	{
		case ECustomTraceChannel::FluxStartEnd:
			return ECollisionChannel::ECC_GameTraceChannel1;
		case ECustomTraceChannel::HoverInteractor:
			return ECollisionChannel::ECC_GameTraceChannel2;
		case ECustomTraceChannel::PathfindingCollision:
			return ECollisionChannel::ECC_GameTraceChannel3;
		case ECustomTraceChannel::CameraCollision:
			return ECollisionChannel::ECC_GameTraceChannel4;
		
		default: return ECollisionChannel::ECC_Visibility;
	}
}

FText UFunctionLibraryInfernale::GetNodeEffectAsText(FNodeEffect NodeEffect, const FString& Style)
{
	FString NodeEffectString = "";
	FString NodeEffectValue = "";
	switch (NodeEffect.NodeEffect)
	{
	case ENodeEffect::NodeEffectNone:
		NodeEffectString = "MissingEffect";
		break;
	case ENodeEffect::NodeEffectDamageToBuilding:
		NodeEffectString = NodeEffect.IsPercentage ?
			"<BODY_sm16_Grey>All your units:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value*100) + "% damage against buildings" :
				"<BODY_sm16_Grey>All your units:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value) + " damage against buildings";
		break;
	case ENodeEffect::NodeEffectDamageToMonster:
		NodeEffectString = NodeEffect.IsPercentage ?
			"<BODY_sm16_Grey>All your units:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value*100) + "% damage against monsters and bosses" :
				"<BODY_sm16_Grey>All your units:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value) + " damage against monsters and bosses";
		//GEngine->AddOnScreenDebugMessage(-1,5.f, FColor::Red, NodeEffectString);
		break;
	case ENodeEffect::NodeEffectDamageToUnit:
		NodeEffectString = NodeEffect.IsPercentage ?
			"<BODY_sm16_Grey>All your units:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value*100) + "% damage against units" :
				"<BODY_sm16_Grey>All your units:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value) + " damage against units";
		break;
	case ENodeEffect::NodeEffectHealthUnit:
		NodeEffectString = NodeEffect.IsPercentage ?
			"<BODY_sm16_Grey>All your units:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value*100) + "% health" :
				"<BODY_sm16_Grey>All your units:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value) + " health";
		break;
	case ENodeEffect::NodeEffectHealthBuilding:
		NodeEffectString = NodeEffect.IsPercentage ?
			"<BODY_sm16_Grey>All your buildings:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value*100) + "% health" :
				"<BODY_sm16_Grey>All your buildings:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value) + " health";
		break;
	case ENodeEffect::NodeEffectFluxRange:
		NodeEffectString = NodeEffect.IsPercentage ?
			"<BODY_sm16_Grey>Your flux range:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value*100) + "%" :
				"<BODY_sm16_Grey>Your flux range:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value) + "m";
		break;
	case ENodeEffect::NodeEffectUnitSight:
		NodeEffectString = NodeEffect.IsPercentage ?
			"<BODY_sm16_Grey>All your units:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value*100) + "% sight" :
				"<BODY_sm16_Grey>All your units:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value) + "m sight";
		break;
	case ENodeEffect::NodeEffectUnitSpeed:
		NodeEffectString = NodeEffect.IsPercentage ?
			"<BODY_sm16_Grey>All your units:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value*100) + "% movement speed" :
				"<BODY_sm16_Grey>All your units:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value) + "m/s movement speed";
		break;
	case ENodeEffect::NodeEffectBuildingSight:
		NodeEffectString = NodeEffect.IsPercentage ?
			"<BODY_sm16_Grey>All your buildings:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value*100) + "% sight" :
				"<BODY_sm16_Grey>All your buildings:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value) + "m sight";
		break;
	case ENodeEffect::NodeEffectBuildingRecycleSouls:
		NodeEffectString = NodeEffect.IsPercentage ?
			//"<BODY_sm16_Grey>All your buildings:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value*100) + "% more souls when recycled" :
			"<BODY_sm16_White>Full soul value from recycling buildings</> <BODY_sm16_White>" :
				"<BODY_sm16_Grey>All your buildings:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value) + " more souls when recycled";
		break;
	case ENodeEffect::NodeEffectBuildingOverclockDuration:
		NodeEffectString = NodeEffect.IsPercentage ?
			"<BODY_sm16_Grey>All your buildings:</> <BODY_sm16_White>" + FString(NodeEffect.Value <= 1 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value*100) + "% overclock duration" :
				"<BODY_sm16_Grey>All your buildings:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value) + "s overclock duration";
		break;
	case ENodeEffect::NodeEffectBuildingConstructionTime:
		NodeEffectString = NodeEffect.IsPercentage ?
			"<BODY_sm16_Grey>All your buildings:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 1 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value*100) + "% build speed" :
				"<BODY_sm16_Grey>All your buildings:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value) + "s build duration";
		break;
	case ENodeEffect::NodeEffectBuildingConstructionCost:
		NodeEffectString = NodeEffect.IsPercentage ?
			"<BODY_sm16_Grey>All your buildings:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value*100) + "% building cost" :
				"<BODY_sm16_Grey>All your buildings:</> <BODY_sm16_White>" + FString(NodeEffect.Value >= 0 ? "+" : "") + FString::SanitizeFloat(NodeEffect.Value) + "souls building cost";
		break;
	}

	NodeEffectString = Style + NodeEffectString + "</>";
	FText Text = FText::FromString(NodeEffectString);
	return Text;
}

void UFunctionLibraryInfernale::ForceDestroyComponent(const UObject* WorldContextObject, UActorComponent* ActorComponent)
{
	if (!ActorComponent) return;

	GEngine->AddOnScreenDebugMessage(1, 2.5f, FColor::Orange, TEXT("Force Destroy Component :") + ActorComponent->GetName());

	ActorComponent->DestroyComponent();
}

TArray<FDataGathererPlayerInfo> UFunctionLibraryInfernale::GetDataGathererPlayerInfoFromJsonFromPath(FString Path)
{
	if (Path == "")
	{
		Path = FPaths::ProjectSavedDir() / TEXT("PlayerSnapshots.json");
	}
	TArray<FDataGathererPlayerInfo> LocalDataGathererPlayerInfo;
	FString JsonString;
	if (FFileHelper::LoadFileToString(JsonString, *Path))
	{
		TArray<TSharedPtr<FJsonValue>> JsonArray;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
		if (FJsonSerializer::Deserialize(Reader, JsonArray))
		{
			for (const auto& JsonValue : JsonArray)
			{
				FPlayerSnapshot PlayerSnapshot;
				bool bSuccess = FJsonObjectConverter::JsonObjectToUStruct(
					JsonValue->AsObject().ToSharedRef(), FPlayerSnapshot::StaticStruct(), &PlayerSnapshot, 0, 0);
				if (bSuccess)
				{
					for (const auto& PlayerInfo : PlayerSnapshot.PlayerInfos)
					{
						LocalDataGathererPlayerInfo.Add(PlayerInfo);
					}
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("Failed to convert JSON object to PlayerInfo struct."));
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to deserialize JSON string."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load JSON file from path: %s"), *Path);
	}

	return LocalDataGathererPlayerInfo;
}

TArray<FString> UFunctionLibraryInfernale::GetTimeMMSSFromJsonFromPath(FString Path)
{
	TArray<FString> LocalDataGathererPlayerInfo;

	// Validate path
	if (Path.IsEmpty())
	{
		Path = FPaths::ProjectSavedDir() / TEXT("PlayerSnapshots.json");
	}

	// Check if file exists
	if (!FPaths::FileExists(Path))
	{
		UE_LOG(LogTemp, Error, TEXT("JSON file does not exist at path: %s"), *Path);
		return LocalDataGathererPlayerInfo;
	}

	FString JsonString;
	if (FFileHelper::LoadFileToString(JsonString, *Path))
	{
		// Validate JSON string isn't empty
		if (JsonString.IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("JSON file is empty: %s"), *Path);
			return LocalDataGathererPlayerInfo;
		}

		// Try to parse with error handling
		TArray<TSharedPtr<FJsonValue>> JsonArray;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

		bool bParseSuccess = false;

		// Wrap the deserialization in a try-catch equivalent
		{
			FJsonSerializer::Deserialize(Reader, JsonArray);

			if (JsonArray.Num() > 0) 
			{
				bParseSuccess = true;
			}

			if (!bParseSuccess || JsonArray.Num() == 0)
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON or array is empty"));
				return LocalDataGathererPlayerInfo;
			}

			for (const auto& JsonValue : JsonArray)
			{
				// Validate that we have a valid JSON object
				if (!JsonValue.IsValid() || !(JsonValue->Type == EJson::Object))
				{
					continue;
				}

				FPlayerSnapshot PlayerSnapshot;
				bool bSuccess = FJsonObjectConverter::JsonObjectToUStruct(JsonValue->AsObject().ToSharedRef(),
				                                                          FPlayerSnapshot::StaticStruct(),
				                                                          &PlayerSnapshot, 0, 0);
				if (bSuccess)
				{
					LocalDataGathererPlayerInfo.Add(PlayerSnapshot.TimeMMSS);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to convert JSON object to PlayerInfo struct."));
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load JSON file from path: %s"), *Path);
	}

	return LocalDataGathererPlayerInfo;
}

TArray<FPlayerSnapshot> UFunctionLibraryInfernale::GetPlayerSnapshotsFromJsonFromPath(FString Path)
{
	if (Path == "")
	{
		Path = FPaths::ProjectSavedDir() / TEXT("PlayerSnapshots.json");
	}
	TArray<FPlayerSnapshot> LocalPlayerSnapshots;
	FString JsonString;
	if (FFileHelper::LoadFileToString(JsonString, *Path))
	{
		TArray<TSharedPtr<FJsonValue>> JsonArray;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
		if (FJsonSerializer::Deserialize(Reader, JsonArray))
		{
			for (const auto& JsonValue : JsonArray)
			{
				FPlayerSnapshot PlayerSnapshot;
				bool bSuccess = FJsonObjectConverter::JsonObjectToUStruct(
					JsonValue->AsObject().ToSharedRef(), FPlayerSnapshot::StaticStruct(), &PlayerSnapshot, 0, 0);
				if (bSuccess)
				{
					LocalPlayerSnapshots.Add(PlayerSnapshot);
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("Failed to convert JSON object to PlayerInfo struct."));
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to deserialize JSON string."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load JSON file from path: %s"), *Path);
	}

	return LocalPlayerSnapshots;
}
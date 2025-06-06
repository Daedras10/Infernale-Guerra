// Fill out your copyright notice in the Description page of Project Settings.

#include "FogOfWar/FogOfWarManager.h"

#include "DataAsset/GameSettingsDataAsset.h"
#include "DataAsset/MainBuildingDataAsset.h"
#include "FunctionLibraries/FunctionLibraryInfernale.h"
#include "GameMode/Infernale/PlayerStateInfernale.h"
#include "Kismet/GameplayStatics.h"
#include "LD/Breach.h"
#include "LD/Buildings/MainBuilding.h"
#include "UnitAsActor/UnitActor.h"

FMovingActorVision::FMovingActorVision(): SightRadius(0), VisionType()
{
}

FMovingActorVision::FMovingActorVision(TWeakObjectPtr<AActor> InActor, float InSightRadius, ::VisionType InVisionType)
	: Actor(InActor), SightRadius(InSightRadius), VisionType(InVisionType)
{
}

FMassEntityVision::FMassEntityVision(): SightRadius(0), VisionType()
{
}

FMassEntityVision::FMassEntityVision(float InSightRadius, ::VisionType InVisionType) 
	: SightRadius(InSightRadius), VisionType(InVisionType)
{
}

FPositionWithVision::FPositionWithVision(): StaticBuildID(0), Position(FVector2D(0, 0)), SightRadius(0), VisionType()
{
}

FPositionWithVision::FPositionWithVision(int ID, FVector2D InPosition, float InSightRadius, ::VisionType InVisionType) 
	: StaticBuildID(ID), Position(InPosition), SightRadius(InSightRadius), VisionType(InVisionType)
{
}

void AFogOfWarManager::UpdateTextureRegion(uint8* Data, uint32 InDestX, uint32 InDestY, int32 InSrcX, int32 InSrcY, uint32 InWidth, uint32 InHeight, bool bIsReset = false)
{
	FUpdateTextureRegion2D* Region = new FUpdateTextureRegion2D( InDestX, InDestY, InSrcX, InSrcY, InWidth, InHeight);

	// Ensure the update region does not exceed the texture's height
	int32 MaxHeight = FMath::Min(InHeight, FogTexture->GetSizeY() - InDestY);
	int32 MaxWidth = FMath::Min(InWidth, FogTexture->GetSizeX() - InDestX);

	if (MaxHeight <= 0 || MaxWidth <= 0  ||	(InDestX < 0 || InDestY < 0 || InSrcX < 0 || InSrcY < 0) ||
		(InDestX + InWidth > static_cast<uint32>(FogTexture->GetSizeX()) || InDestY + InHeight >static_cast<uint32>(FogTexture->GetSizeY())))
	{
		delete[] Data;
		delete[] Region;
		return;
	}
	
	
	TFunction<void(uint8* SrcData, const FUpdateTextureRegion2D* Regions)> DataCleanupFunc =
		[](const uint8* SrcData, const FUpdateTextureRegion2D* Regions) {
			delete[] SrcData;
			delete[] Regions;
	};

	for (int i = 0; i < static_cast<int>(InWidth * InHeight); i++)
	{
		int Index = InDestX + InDestY * MapSize.X + (i % InWidth) + (i / InWidth) * MapSize.X;
		if (Index >= 0 && Index < MapSize.X * MapSize.Y)
		{
			if (bIsReset) Data[i] = 0;
			else
			{
				if (Index >= 0 && Index < MapSize.X * MapSize.Y)
				{
					if (Data[i] + RawImageData[Index] > 255) Data[i] = 255;
					else Data[i] += RawImageData[Index];
					RawImageData[Index] = Data[i];
				}
			}
		}
	}
	FogTexture->UpdateTextureRegions(0, 1, Region, InWidth, 1, Data, DataCleanupFunc);
	Region = nullptr;
	Data = nullptr;
	DataCleanupFunc = nullptr;
	ApplyTextureToMaterial();
}

void AFogOfWarManager::InitFogTexture()
{
	RawImageDataArray.Init(0, MapSize.X * MapSize.Y);
	RawImageData = new uint8[MapSize.X * MapSize.Y];
	RawImageData = RawImageDataArray.GetData();
	FName FogOfWarName = bLocalName ? FName(*FString::FromInt(FMath::RandRange(0, 100000))) : "FogOfWartTexture";
	FogTexture = UTexture2D::CreateTransient(MapSize.X, MapSize.Y, EPixelFormat::PF_R8, FogOfWarName);
	FogTexture->CompressionSettings = TextureCompressionSettings::TC_Grayscale;
	FogTexture->SRGB = 0;
	FogTexture->Filter = TextureFilter::TF_Nearest;
	FogTexture->AddToRoot();
	FogTexture->UpdateResource();
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("FogTexture Created"));
	if (!FogTexture)
	{
		return;
	}
	uint8* Data = new uint8[MapSize.X * MapSize.Y];
	for (int i = 0; i < MapSize.X * MapSize.Y; i++)
	{
		Data[i] = 0;
	}
}

void AFogOfWarManager::FindAllBaseOfTeam(ETeam Team)
{
	//debug the team name
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Team: %s"), *UEnum::GetValueAsString(Team)));
	StaticActors.Empty();
	TArray<AActor*> AllActors;
	for (auto& WeakActor : AllBPMainBuildings)
	{
		if (WeakActor.IsValid())
		{
			AllActors.Add(WeakActor.Get());
		}
	}
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMainBuilding::StaticClass(), AllActors);
	AllBPMainBuildings.Empty();
	for (auto AllActor : AllActors)
	{
		AllBPMainBuildings.Add(TWeakObjectPtr<AMainBuilding>(Cast<AMainBuilding>(AllActor)));
	}
	UnknownBaseActors.Empty();
	for (TWeakObjectPtr<AActor> Actor : AllBPMainBuildings)
	{
		ABuildingParent* MainBuilding = Cast<ABuildingParent>(Actor);
		if (MainBuilding != nullptr && MainBuilding->GetOwner().Team == Team)
		{
			FVector2D MainBuildingPosition = FVector2D(Actor->GetActorLocation().X,
			                                           Actor->GetActorLocation().Y);
			AMainBuilding* Src = Cast<AMainBuilding>(Actor);
			if (Src == nullptr) continue;
			FPositionWithVision newMainBuilding = FPositionWithVision(1,MainBuildingPosition, Src->MainBuildingDataAsset->SightRange, Src->MainBuildingDataAsset->VisionTypeShape);
			StaticActors.Add(newMainBuilding);
			SetFogVisibilityOfActor(Src->GetFogOfWarComponent(), true, true);

			for (ABreach* Breach : Cast<AMainBuilding>(Src)->GetBreaches())
			{
				if (Breach == nullptr) continue;
				FVector2D BreachPosition = FVector2D(Breach->GetActorLocation().X,
				                                     Breach->GetActorLocation().Y);
				FPositionWithVision newBreach = FPositionWithVision(1,BreachPosition, Src->MainBuildingDataAsset->SightRange/2, Src->MainBuildingDataAsset->VisionTypeShape);
				StaticActors.Add(newBreach);
				SetFogVisibilityOfActor(Breach->FogOfWarComponent, true, true);
			}
		}
		// else if (MainBuilding != nullptr && MainBuilding->GetOwner().Team != Team)
		// {
		// 	AMainBuilding* Src = Cast<AMainBuilding>(Actor);
		// 	FVector Color{UFunctionLibraryInfernale::GetTeamColorCpp(ETeam::NatureTeam)};
		// 	Src->MainBaseStaticMesh->SetVectorParameterValueOnMaterials("Color", Color);
		// 	UnknownBaseActors.Add(Src);
		// }
		else if (MainBuilding != nullptr)
		{
			SetFogVisibilityOfActor(MainBuilding->GetFogOfWarComponent(), false, true);
			for (ABreach* Breach : Cast<AMainBuilding>(MainBuilding)->GetBreaches())
			{
				SetFogVisibilityOfActor(Breach->FogOfWarComponent, false, true);
			}
		}
	}
}

void AFogOfWarManager::SetFogVisibilityOfActor(UFogOfWarComponent* Actor, bool bVisibleFog, bool bVisibleActor)
{
    if (Actor == nullptr) return;
    Actor->SetVisibilityOfActorWithFog(bVisibleFog,bVisibleActor);
}

void AFogOfWarManager::FindAllBase()
{
	FindAllBaseOfTeam(debugTeam);
}

// Sets default values
AFogOfWarManager::AFogOfWarManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	FogPlane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FogPlane"));
	FogPlane->SetVisibility(true);
	FogPlane->SetHiddenInGame(false);
	FogPlane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FogPlane->SetCastShadow(false);
	FogPlane->SetRelativeScale3D(FVector(MapSize.X, MapSize.Y, 1));
	FogPlane->SetCollisionProfileName(TEXT("Custom"));
	FogPlane->SetCollisionResponseToAllChannels(ECR_Ignore);
	
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Plane.Plane'"));
	if (PlaneMeshAsset.Succeeded())
	{
		FogPlane->SetStaticMesh(PlaneMeshAsset.Object);
	}

	RawImageData = new uint8[MapSize.X * MapSize.Y];
	RawImageDataArray.Init(0, MapSize.X * MapSize.Y);

	InitFogTexture();

	ApplyTextureToMaterial();
	
}

void AFogOfWarManager::AddMovingActorVision(TWeakObjectPtr<AActor> Actor, float SightRadius, VisionType VisionType)
{
	MovingActors.Add(FMovingActorVision(Actor, SightRadius, VisionType));
	// SetMovingActorsVision();

}

void AFogOfWarManager::AddMassEntityVision(FMassEntityHandle Handle, float SightRadius, VisionType VisionType)
{
	MassEntities.Add(Handle);
	MassVisionData.Add(Handle.AsNumber(), FMassEntityVision(SightRadius, VisionType));
}

bool AFogOfWarManager::Contains(FMassEntityHandle Handle)
{
	return MassEntities.Contains(Handle);
}

void AFogOfWarManager::RemoveMovingActorVision(TWeakObjectPtr<AActor> Actor)
{
	MovingActors.RemoveAll([Actor](FMovingActorVision MovingActor)
    {
        return MovingActor.Actor == Actor;
    });
}

void AFogOfWarManager::RemoveMassEntityVision(FMassEntityHandle Handle)
{
	MassEntities.Remove(Handle);
	MassVisionData.Remove(Handle.AsNumber());
}

void AFogOfWarManager::ApplyTextureToMaterial()
{
	if (FogPlane != nullptr)
	{

		if (Material != nullptr)
		{
			DynamicMaterial = UMaterialInstanceDynamic::Create(Material, FogPlane);

			if (DynamicMaterial != nullptr)
			{
				FogPlane->SetMaterial(0, DynamicMaterial);
			}
		}
	}

	if (DynamicMaterial != nullptr)
	{
		DynamicMaterial->SetTextureParameterValue("FogTexture", FogTexture);
	}
}

int AFogOfWarManager::GetAlphaAtPosition0To255(FVector2D Position)
{
	return RawImageData[static_cast<int>(static_cast<int>(Position.X) + static_cast<int>(Position.Y) * MapSize.X)];
}

int AFogOfWarManager::GetAlphaAtPosition0To1(FVector2D Position)
{
	return GetAlphaAtPosition0To255(Position) / 255;
}

void AFogOfWarManager::ModifyMovingActorVision(TWeakObjectPtr<AActor> Actor, float SightRadius, VisionType VisionType)
{
	for (FMovingActorVision& MovingActor : MovingActors)
	{
		if (MovingActor.Actor == Actor)
		{
			MovingActor.SightRadius = SightRadius;
			MovingActor.VisionType = VisionType;
		}
	}
}

void AFogOfWarManager::ModifyMassEntityVision(FMassEntityHandle Handle, float SightRadius, VisionType VisionType)
{
	uint64 HandleAsNumber = Handle.AsNumber();
	MassVisionData[HandleAsNumber].SightRadius = SightRadius;
	MassVisionData[HandleAsNumber].VisionType = VisionType;
}

void AFogOfWarManager::ModifyStaticActorVision(int ID, float SightRadius, VisionType VisionType)
{
	for (FPositionWithVision StaticActor : StaticActors)
    {
        if (StaticActor.StaticBuildID == ID)
        {
            StaticActor.SightRadius = SightRadius;
            StaticActor.VisionType = VisionType;
        }
    }
}

void AFogOfWarManager::SmootherTransition(TArray<int32> PixelIndexes)
{
	
}

void AFogOfWarManager::SetStaticActorsVision()
{
	FindAllBaseOfTeam(PlayerTeam);
	if (StaticActors.Num() >= 1)
	{		
		for (FPositionWithVision PositionWithVision : StaticActors)
		{
			FVector2D Position = PositionWithVision.Position;
			int SightRadius = PositionWithVision.SightRadius;
			int32 X = Position.X/100 + MapSize.X/2;
			int32 Y = Position.Y/100 + MapSize.Y/2;
			uint8* Data = new uint8[SightRadius * SightRadius];
			for (int i = 0; i < SightRadius * SightRadius; i++)
			{
				Data[i] = 0;
			}
			int ArraySize = SightRadius * SightRadius;
			if (X < 0 || Y < 0)
			{
				continue;
			}
			if (X > MapSize.X || Y > MapSize.Y)
			{
				continue;
			}
			switch (PositionWithVision.VisionType)
			{
				case VisionType::Circle:
					
					for (int i = 0; i < ArraySize; i++)
					{
						float XPos = static_cast<float>(i % SightRadius);
						float YPos = static_cast<float>(i / SightRadius);
						if (FMath::Sqrt(FMath::Pow(XPos - SightRadius/2, 2) + FMath::Pow(YPos - SightRadius/2, 2)) < SightRadius/2)
						{
							Data[i] = FogOpacityCurve->GetFloatValue(FMath::Sqrt(FMath::Pow(XPos - SightRadius/2, 2) + FMath::Pow(YPos - SightRadius/2, 2)) / SightRadius);
						}
					}
					break;
				case VisionType::Square:
					
					break;
				case VisionType::Cone:
					// calculate the zone of the cone
					break;
				case VisionType::Ellipse:
					// calculate the zone of the ellipse
					break;
				case VisionType::Trapezoid:
					// calculate the zone of the trapezoid
					break;
			}
			UpdateTextureRegion(Data, X - SightRadius/2, Y - SightRadius/2, 0, 0, SightRadius, SightRadius);
		}
	}
}

void AFogOfWarManager::SetMovingActorsVision()
{
	if (MovingActors.Num() >= 1)
	{
		int count = 0;
		for (FMovingActorVision MovingActorVision : MovingActors)
		{
			count++;
			if (count % 2 == 0)
			{
				continue;
			}
			FVector2D Position = FVector2D(MovingActorVision.Actor->GetActorLocation().X, MovingActorVision.Actor->GetActorLocation().Y);
			int SightRadius = MovingActorVision.SightRadius;
			int32 X = Position.X/100 + MapSize.X/2;
			int32 Y = Position.Y/100 + MapSize.Y/2;
			if (X - SightRadius/2 < 0 || Y - SightRadius/2 < 0)
			{
				continue;
			}
			if (X - SightRadius/2 + SightRadius > MapSize.X || Y - SightRadius/2 + SightRadius > MapSize.Y)
			{
				continue;
			}
			uint8* Data = new uint8[SightRadius * SightRadius];
			for (int i = 0; i < SightRadius * SightRadius; i++)
			{
				Data[i] = 0;
			}
			int ArraySize = SightRadius * SightRadius;
			
			for (int i = 0; i < ArraySize; i++)
			{
				float XPos = static_cast<float>(i % SightRadius);
				float YPos = static_cast<float>(i / SightRadius);
				if (FMath::Sqrt(FMath::Pow(XPos - SightRadius/2, 2) + FMath::Pow(YPos - SightRadius/2, 2)) < SightRadius/2)
				{
					Data[i] = FogOpacityCurve->GetFloatValue(FMath::Sqrt(FMath::Pow(XPos - SightRadius/2, 2) + FMath::Pow(YPos - SightRadius/2, 2)) / SightRadius);
				}
			}
			
			UpdateTextureRegion(Data, X - SightRadius/2, Y - SightRadius/2, 0, 0, SightRadius, SightRadius);
		}
	}
}

void AFogOfWarManager::SetMassEntityVision(FMassEntityHandle Handle, const FVector& Location)
{
	FVector2D Position = FVector2D(Location.X, Location.Y);
	int SightRadius = MassVisionData[Handle.AsNumber()].SightRadius;
	int32 X = Position.X / 100 + MapSize.X / 2;
	int32 Y = Position.Y / 100 + MapSize.Y / 2;
	if (X - SightRadius / 2 < 0 || Y - SightRadius / 2 < 0)
	{
		return;
	}
	if (X - SightRadius / 2 + SightRadius > MapSize.X || Y - SightRadius / 2 + SightRadius > MapSize.Y)
	{
		return;
	}
	uint8* Data = new uint8[SightRadius * SightRadius];
	for (int i = 0; i < SightRadius * SightRadius; i++)
	{
		Data[i] = 0;
	}
	int ArraySize = SightRadius * SightRadius;

	for (int i = 0; i < ArraySize; i++)
	{
		float XPos = static_cast<float>(i % SightRadius);
		float YPos = static_cast<float>(i / SightRadius);
		if (FMath::Sqrt(FMath::Pow(XPos - SightRadius / 2, 2) + FMath::Pow(YPos - SightRadius / 2, 2)) < SightRadius / 2)
		{
			Data[i] = FogOpacityCurve->GetFloatValue(FMath::Sqrt(FMath::Pow(XPos - SightRadius / 2, 2) + FMath::Pow(YPos - SightRadius / 2, 2)) / SightRadius);
		}
	}

	UpdateTextureRegion(Data, X - SightRadius / 2, Y - SightRadius / 2, 0, 0, SightRadius, SightRadius);
}

void AFogOfWarManager::ResetAllTexture()
{
	uint8* Data = new uint8[MapSize.X * MapSize.Y];
	for (int i = 0; i < MapSize.X * MapSize.Y; i++)
	{
		Data[i] = 0;
	}
	UpdateTextureRegion(Data, 0, 0, 0, 0, MapSize.X, MapSize.Y, true);
}

// Called when the game starts or when spawned
void AFogOfWarManager::BeginPlay()
{
	Super::BeginPlay();
	UseFogOfWar = UFunctionLibraryInfernale::GetGameSettingsDataAsset()->UseFogOfWar;
	if (!UseFogOfWar) {
		DisableFogOfWar();
		return;
	}
	
	auto PlayerControllerInfernale = Cast<APlayerControllerInfernale>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (PlayerControllerInfernale != nullptr)
    {
		PlayerTeam = PlayerControllerInfernale->GetTeam();
		if (PlayerTeam != ETeam::NatureTeam)
		{
			FindAllBaseOfTeam(PlayerTeam);
		}
    }
	else if (bDebugPlayerTeams) GEngine->AddOnScreenDebugMessage(-1, 20.5f, FColor::Red, TEXT("AFogOfWarManager::PlayerState is null"));
	
	
	InitFogTexture();

	if (FogTexture == nullptr)
	{
		// InitFogTexture();
		GEngine->AddOnScreenDebugMessage(-1, 20.5f, FColor::Red, TEXT("FogTexture is null"));
		return;
	}

	if (StaticActors.Num() == 0)
	{
		FindAllBaseOfTeam(PlayerTeam);
		GEngine->AddOnScreenDebugMessage(-1, 20.5f, FColor::Red, TEXT("AFogOfWarManager::StaticActors is empty"));
		return;
	}
	// FogPlane->SetRelativeScale3D(FVector(MapSize.X, MapSize.Y, 1));
	SetStaticActorsVision();
}

void AFogOfWarManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// if (FogTexture)
	// {
	// 	FTexturePlatformData* PlatformData = FogTexture->GetPlatformData();
	// 	if (PlatformData)
	// 	{
	// 		FTexture2DMipMap& FirstMip = PlatformData->Mips[0];
	// 		FByteBulkData& ImageData = FirstMip.BulkData;
	// 		if (ImageData.IsLocked())
	// 		{
	// 			ImageData.Unlock();
	// 		}
	// 		RawImageData = nullptr;
	// 	}
	// 	FogTexture = nullptr;
	// }

	Super::EndPlay(EndPlayReason);
}

void AFogOfWarManager::DisableFogOfWar()
{
	FogPlane->SetVisibility(false);
}


void AFogOfWarManager::SetEnemyVision()
{
	TArray<TWeakObjectPtr<AActor>> ToDelete;
	for (TWeakObjectPtr<AActor> Actor : EnemyActorsToShowOrHide)
	{
		if (Actor == nullptr)
		{
			ToDelete.Add(Actor);
		}
	}
	for (TWeakObjectPtr<AActor> Actor : ToDelete)
	{
		EnemyActorsToShowOrHide.Remove(Actor);
	}
	for (TWeakObjectPtr<AActor> Actor : EnemyActorsToShowOrHide)
	{
		//check if the Actor is in the vision of the player if it is then show it
		//if it is not then hide it
		if (Actor == nullptr)
		{
			continue;
		}
		// check if the Actor is a UnitActor
		if (Actor->IsA(AUnitActor::StaticClass()))
		{
			auto UnitActor = Cast<AUnitActor>(Actor);
			auto FogComponent = UnitActor->GetFogOfWarComponent();
			if (FogComponent == nullptr)
			{
				continue;
			}
			auto Location = Actor->GetActorLocation();
			auto LocationInsideTheTexture = FVector2D(Location.X/100 + MapSize.X/2, Location.Y/100 + MapSize.Y/2);
			if (GetAlphaAtPosition0To1(LocationInsideTheTexture) > 0)
			{
				// GEngine->AddOnScreenDebugMessage(-1, 20.5f, FColor::Red, TEXT("Show the Actor"));
				FogComponent->SetVisibilityOfActorWithFog(false, true);
			}
			else
			{
				// GEngine->AddOnScreenDebugMessage(-1, 20.5f, FColor::Red, TEXT("Hide the Actor"));
				FogComponent->SetVisibilityOfActorWithFog(false, false);
			}
		}
	}

	for (TWeakObjectPtr<AActor> Actor : UnknownBaseActors)
	{
		if (Actor == nullptr)
		{
			continue;
		}
		if (Actor->IsA(AMainBuilding::StaticClass()))
		{
			auto Location = Actor->GetActorLocation();
			auto LocationInsideTheTexture = FVector2D(Location.X/100 + MapSize.X/2, Location.Y/100 + MapSize.Y/2);
			AMainBuilding* Src = Cast<AMainBuilding>(Actor);
			if (GetAlphaAtPosition0To1(LocationInsideTheTexture) > 0)
			{
				FVector Color{UFunctionLibraryInfernale::GetOldTeamColorCpp(Src->GetOwner().Team)};
				Src->MainBaseStaticMesh->SetVectorParameterValueOnMaterials("Color", Color);
			}
			// else
			// {
			// 	FVector Color{UFunctionLibraryInfernale::GetTeamColorCpp(ETeam::NatureTeam)};
			// 	Src->MainBaseStaticMesh->SetVectorParameterValueOnMaterials("Color", Color);
			// }
		}
	}
}

void AFogOfWarManager::SetVision()
{
	if (PlayerTeam != ETeam::NatureTeam)
	{
		FindAllBaseOfTeam(PlayerTeam);
		ResetAllTexture();
		if(!bUseMass)
			SetMovingActorsVision();
		SetStaticActorsVision();
		SetEnemyVision();
	}
}

// Called every frame
void AFogOfWarManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!UseFogOfWar) return;
	// if (FogTexture != nullptr)
	// {
	// 	FogPlane->SetVisibility(bDebugActive);
	// }

	UpdateTimer -= DeltaTime;
	if (UpdateTimer <= 0)
	{
		if (PlayerTeam == ETeam::NatureTeam)
		{
			auto PlayerControllerInfernale = Cast<APlayerControllerInfernale>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
			if (PlayerControllerInfernale != nullptr)
			{
				PlayerTeam = PlayerControllerInfernale->GetTeam();
				SetVision();
				if (bDebugPlayerTeams) GEngine->AddOnScreenDebugMessage(-1, 20.5f, FColor::Green, FString::Printf(TEXT("PlayerTeam: %s"), *UEnum::GetValueAsString(PlayerTeam)));
			}
			else if (bDebugPlayerTeams) GEngine->AddOnScreenDebugMessage(-1, 20.5f, FColor::Red, TEXT("AFogOfWarManager::PlayerState is null"));
		}
		SetVision();

		UpdateTimer = UpdateInterval;
	}
	
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Tool/ETATest.h"

#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "AI/NavigationSystemBase.h"
#include "Kismet/GameplayStatics.h"
#include "LD/Buildings/MainBuilding.h"
#include "LD/LDElement/Boss.h"
#include "LD/LDElement/NeutralCamp.h"

class UNavigationPath;

FDistanceResult::FDistanceResult(): Distance(0), ETA(0), ActorA(nullptr), ActorB(nullptr), Path(nullptr)
{
}

FDistanceResult::FDistanceResult(AActor* InActorA, AActor* InActorB, double InDistance, double InETA,
	UNavigationPath* InPath): Distance(InDistance), ETA(InETA), ActorA(InActorA), ActorB(InActorB), Path(InPath)
{}

// Sets default values
AETATest::AETATest()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavigationSystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("Navigation system not found"));
		return;
	}
	

}

// Called when the game starts or when spawned
void AETATest::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AETATest::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AETATest::CalculateETA()
{
	for (auto ActorA : PointsOfInterest)
	{
		TArray<AActor*> ListOfPointsOfInterest;
		for (auto ActorB : PointsOfInterest)
		{
			if (ActorA == ActorB) continue;
			if (FVector::Dist(ActorA->GetActorLocation(), ActorB->GetActorLocation()) > RadiusOfSearch) continue;
			ListOfPointsOfInterest.Add(ActorB);
		}

		for (AActor* InRangePointOfInterest : ListOfPointsOfInterest)
		{
			FNavLocation NavLocationA;

			auto Target = InRangePointOfInterest->GetActorLocation();
			bool TargetReachable = NavigationSystem->ProjectPointToNavigation(Target, NavLocationA, FVector(500, 500, 100));
			if (!TargetReachable)
			{
				UE_LOG(LogTemp, Warning, TEXT("Target not reachable"));
				continue;
			}
			Target = NavLocationA.Location;
			auto StartLocation = ActorA->GetActorLocation();

			TargetReachable = NavigationSystem->ProjectPointToNavigation(StartLocation, NavLocationA, FVector(500, 500, 100));
			if (!TargetReachable)
			{
				UE_LOG(LogTemp, Warning, TEXT("Start location not reachable"));
				continue;
			}

			Target = NavLocationA.Location;
			auto EndLocation = Target;

			UNavigationPath* Path = NavigationSystem->FindPathToActorSynchronously(GetWorld(), ActorA->GetActorLocation(), InRangePointOfInterest);
			// Vérifier si Path est valide
			if (!Path || !Path->GetPath().IsValid()) 
			{
				UE_LOG(LogTemp, Warning, TEXT("Path is invalid!"));
				continue;
			}

			const TArray<FNavPathPoint>& PathPoints = Path->GetPath()->GetPathPoints();
			if (PathPoints.Num() == 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("Path has no points!"));
				continue;
			}

			auto PreviousPoint = StartLocation;
			auto TotalDistance = 0.0f;
			for (auto Point : PathPoints)
			{
				auto PointLocation = Point.Location;
				TotalDistance += FVector::Dist(PreviousPoint, PointLocation);
				PreviousPoint = PointLocation;
			}
			auto ETA = TotalDistance / UnitsSpeed;
			DistanceResults.Add(FDistanceResult{ActorA, InRangePointOfInterest, TotalDistance, ETA, Path});
		}
	}
}


void AETATest::AutoFindPointsOfInterest()
{
	//find all actors of class AMainBuilding, ANeutralCamp & ABoss
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMainBuilding::StaticClass(), FoundActors);
	for (auto Actor : FoundActors)
	{
		auto MainBuilding = Cast<AMainBuilding>(Actor);
		if (!MainBuilding) continue;
		PointsOfInterest.Add(MainBuilding);
	}
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANeutralCamp::StaticClass(), FoundActors);
	for (auto Actor : FoundActors)
	{
		auto NeutralCamp = Cast<ANeutralCamp>(Actor);
		if (!NeutralCamp) continue;
		PointsOfInterest.Add(NeutralCamp);
	}
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABoss::StaticClass(), FoundActors);
	for (auto Actor : FoundActors)
	{
		auto Boss = Cast<ABoss>(Actor);
		if (!Boss) continue;
		PointsOfInterest.Add(Boss);
	}
}

void AETATest::DebugRadiusOfSearch()
{
	for (auto Actor : PointsOfInterest)
	{
		auto Location = Actor->GetActorLocation();
		DrawDebugCylinder(GetWorld(), Location, Location + FVector(0, 0, 100), RadiusOfSearch, 12, FColor::Red, false, 5, 0, 10);
	}
}

void AETATest::ShowETADebug()
{
	float MinETA = 1000000;
	float MaxETA = 0;
	for (auto DistanceResult : DistanceResults)
	{
		auto ActorA = DistanceResult.ActorA;
		auto ActorB = DistanceResult.ActorB;
		auto LocationA = ActorA->GetActorLocation();
		auto LocationB = ActorB->GetActorLocation();
		auto ETA = DistanceResult.ETA;
		auto Color = FColor::Green;
		//Gradiant from green to red passing by yellow using the MinMaxETA
		if (ETA < MinMaxETA.X) Color = FColor::Green;
		else if (ETA > MinMaxETA.Y) Color = FColor::Red;
		else Color = FColor::MakeRedToGreenColorFromScalar(FMath::GetMappedRangeValueClamped(FVector2D(MinMaxETA.Y,MinMaxETA.X), FVector2D(0, 1), ETA));
		auto PathPoints = DistanceResult.Path->GetPath()->GetPathPoints();
		auto PreviousPoint = PathPoints[0].Location;
		for (auto Point : PathPoints)
		{
			auto PointLocation = Point.Location;
			DrawDebugLine(GetWorld(), PreviousPoint, PointLocation, Color, false, 5, 0, 100);
			PreviousPoint = PointLocation;
		}
		if (ETA < MinETA) MinETA = ETA;
		if (ETA > MaxETA) MaxETA = ETA;
	}
	UE_LOG(LogTemp, Warning, TEXT("Min ETA: %f Max ETA: %f"), MinETA, MaxETA);
}

void AETATest::DebugAllPaths()
{
	for (auto DistanceResult : DistanceResults)
	{
		if (!DistanceResult.Path) continue;
		auto PathPoints = DistanceResult.Path->GetPath()->GetPathPoints();
		auto PreviousPoint = PathPoints[0].Location;
		for (auto Point : PathPoints)
		{
			auto PointLocation = Point.Location;
			DrawDebugLine(GetWorld(), PreviousPoint, PointLocation, FColor::Red, false, 5, 0, 100);
			PreviousPoint = PointLocation;
		}
	}
}

void AETATest::DebugSelectedETA()
{
	if (!SelectedActorA) return;
	auto SelectedActorDistanceResults = TArray<FDistanceResult>();
	for (auto DistanceResult : DistanceResults)
	{
		if (DistanceResult.ActorA == SelectedActorA || DistanceResult.ActorB == SelectedActorA)
		{
			SelectedActorDistanceResults.Add(DistanceResult);
		}
	}
	float MinETA = 1000000;
	float MaxETA = 0;
	for (auto DistanceResult : SelectedActorDistanceResults)
	{
		auto ETA = DistanceResult.ETA;
		if (ETA < MinETA) MinETA = ETA;
		if (ETA > MaxETA) MaxETA = ETA;
	}
	for (auto DistanceResult : SelectedActorDistanceResults)
	{
		auto ActorA = DistanceResult.ActorA;
		auto ActorB = DistanceResult.ActorB;
		auto LocationA = ActorA->GetActorLocation();
		auto LocationB = ActorB->GetActorLocation();
		auto ETA = DistanceResult.ETA;
		auto Color = FColor::Green;
		if (ETA < MinETA) Color = FColor::Green;
		else if (ETA > MaxETA) Color = FColor::Red;
		else Color = FColor::MakeRedToGreenColorFromScalar(FMath::GetMappedRangeValueClamped(FVector2D(MaxETA, MinETA), FVector2D(0, 1), ETA));
		DrawDebugLine(GetWorld(), LocationA, LocationB, Color, false, 5, 0, 100);
		UE_LOG(LogTemp, Warning, TEXT("ETA: %f"), ETA);
	}
}

void AETATest::DebugSelectedPath()
{
	if (!SelectedActorA) return;
	auto SelectedActorPaths = TArray<UNavigationPath*>();
	for (auto DistanceResult : DistanceResults)
	{
		if (DistanceResult.ActorA == SelectedActorA || DistanceResult.ActorB == SelectedActorA)
		{
			SelectedActorPaths.Add(DistanceResult.Path);
		}
	}

	for (auto Path : SelectedActorPaths)
	{
		if (!Path) continue;
		auto PathPoints = Path->GetPath()->GetPathPoints();
		auto PreviousPoint = PathPoints[0].Location;
		for (auto Point : PathPoints)
		{
			auto PointLocation = Point.Location;
			DrawDebugLine(GetWorld(), PreviousPoint, PointLocation, FColor::Red, false, 5, 0, 100);
			PreviousPoint = PointLocation;
		}
	}
}



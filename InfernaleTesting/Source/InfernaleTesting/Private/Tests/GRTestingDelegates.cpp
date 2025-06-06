// Fill out your copyright notice in the Description page of Project Settings.


#include "Tests/GRTestingDelegates.h"

// Sets default values
AGRTestingDelegates::AGRTestingDelegates()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGRTestingDelegates::BeginPlay()
{
	Super::BeginPlay();
	TestDelegate.AddDynamic(this, &AGRTestingDelegates::Testing);
}

// Called every frame
void AGRTestingDelegates::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AGRTestingDelegates::CallDelegate()
{
	TestDelegate.Broadcast();
}

void AGRTestingDelegates::Testing()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("SUUUUUUUU, only the first time"));
	TestDelegate.RemoveDynamic(this, &AGRTestingDelegates::Testing);
}


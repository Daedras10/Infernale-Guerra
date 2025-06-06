// Fill out your copyright notice in the Description page of Project Settings.


#include "Buildings/GRBase.h"

// Sets default values
AGRBase::AGRBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGRBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGRBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAsset/NameDataAsset.h"

FString UNameDataAsset::GetRandomName()
{
	return Names[FMath::RandRange(0, Names.Num() - 1)];
}

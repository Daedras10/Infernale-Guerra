// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAsset/MainBuildingDistrictsDataAsset.h"

FDistrictElementProportion::FDistrictElementProportion(): DistrictMesh(nullptr)
{
}

FDistrictElementSpawnInfo::FDistrictElementSpawnInfo()
{
}

FDistrictData::FDistrictData(): Breach(nullptr)
{
}

FDistrictElement::FDistrictElement(): Mesh(nullptr), Breach(nullptr), Scale(FVector(1, 1, 1)), ZOffset(0)
{
}

FCityElements::FCityElements()
{
}

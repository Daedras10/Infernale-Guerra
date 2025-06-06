// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAsset/GameSettingsDataAsset.h"

FMeshById::FMeshById(): Mesh(nullptr)
{
}

FDataAssetsSettings::FDataAssetsSettings(): FluxSettings(nullptr), SoundsAssets(nullptr), EconomyAssets(nullptr),
                                            TransmutationAssets(nullptr),
                                            BuildingListAssets(nullptr)
{
}

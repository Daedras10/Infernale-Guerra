// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Amalgam/Traits/ServerAmalgamTrait.h"
#include "MassEntityTemplateRegistry.h"
#include "Mass/Army/AmalgamTags.h"


void UServerAmalgamTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.AddTag<FAmalgamServerTag>();
}

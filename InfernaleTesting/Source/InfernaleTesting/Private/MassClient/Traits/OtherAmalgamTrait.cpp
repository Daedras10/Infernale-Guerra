// Fill out your copyright notice in the Description page of Project Settings.


#include "MassClient/Traits/OtherAmalgamTrait.h"
#include "MassEntityTemplateRegistry.h"
#include "Mass/Army/AmalgamTags.h"
#include "MassClient/Tags/ClientTags.h"

void UOtherAmalgamTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
    BuildContext.AddTag<FAmalgamClientTag>();
    BuildContext.AddTag<FAmalgamClientInitializeTag>();
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Interfaces/Interactable.h"


// Add default functionality here for any IInteractable functions that are not pure virtual.
void IInteractable::InteractStartMain(APlayerControllerInfernale* Interactor)
{
}

void IInteractable::InteractEndMain(APlayerControllerInfernale* Interactor)
{
}

void IInteractable::InteractStartHover(APlayerControllerInfernale* Interactor)
{
}

void IInteractable::InteractEndHover(APlayerControllerInfernale* Interactor)
{
}

void IInteractable::InteractStartHoverFlux(APlayerControllerInfernale* Interactor)
{
}

void IInteractable::InteractEndHoverFlux(APlayerControllerInfernale* Interactor)
{
}

void IInteractable::InteractStartSecondary(APlayerControllerInfernale* Interactor)
{
}

void IInteractable::InteractEndSecondary(APlayerControllerInfernale* Interactor)
{
}

bool IInteractable::InteractableHasUIOpen()
{
	return false;
}

bool IInteractable::ShouldEndMainInteractOnMove()
{
	return false;
}

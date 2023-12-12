// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "Myproject/Character/BlasterCharacter.h"
#include "Myproject/PlayerController/BlasterPlayerController.h"

void ABlasterGameMode::PlayerEliminated(class ABlasterCharacter* ElimmedCharacter, class ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)	//ÌÔÌ­
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
	}
}


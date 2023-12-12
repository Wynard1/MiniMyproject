// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "Myproject/Character/BlasterCharacter.h"
#include "Myproject/PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"

void ABlasterGameMode::PlayerEliminated(class ABlasterCharacter* ElimmedCharacter, class ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)	//ÌÔÌ­
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		////·ÖÀëCharacterºÍController
		ElimmedCharacter->Reset();
		
		ElimmedCharacter->Destroy();
	}
	if (ElimmedController)
	{
		/*
		*set random player start
		*/
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);

		//respawn
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}
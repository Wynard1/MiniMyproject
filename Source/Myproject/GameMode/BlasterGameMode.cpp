// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "Myproject/Character/BlasterCharacter.h"
#include "Myproject/PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Myproject/PlayerState/BlasterPlayerState.h"

void ABlasterGameMode::PlayerEliminated(class ABlasterCharacter* ElimmedCharacter, class ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)	//淘汰
{
	if (AttackerController == nullptr || AttackerController->PlayerState == nullptr) return;
	if (VictimController == nullptr || VictimController->PlayerState == nullptr) return;
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;

	//击杀后加分
	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(1.f);
	}

	//被击杀后记录死亡次数
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	//播放角色死亡的动效和音效
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	
	
	if (ElimmedCharacter)
	{
		//分离Character和Controller
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
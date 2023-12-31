// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "Myproject/Character/BlasterCharacter.h"
#include "Myproject/PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Myproject/PlayerState/BlasterPlayerState.h"
#include "Myproject/GameState/BlasterGameState.h"


namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

ABlasterGameMode::ABlasterGameMode()
{
	//开关，最开始会先进入“开始前状态”
	bDelayedStart = true;
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();

	//打点，记录最开始的时间
	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//赛前
	if (MatchState == MatchState::WaitingToStart)
	{
		//每个TICK重新开始计算准备时间
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;

		//倒计时到了，进入Inprogress
		if (CountdownTime <= 0.f)
		{
			//从"开始前"过渡到Play
			StartMatch();
		}
	}

	//局内
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}

	//赛后冷却环节
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}

void ABlasterGameMode::OnMatchStateSet()
{
	/*
	现在每当matchstate在游戏模式中改变时，它就会在所有玩家控制器中循环
	*/
	Super::OnMatchStateSet();

	//循环遍历所有的玩家控制器，进入MatchState
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(*It);
		if (BlasterPlayer)
		{
			BlasterPlayer->OnMatchStateSet(MatchState);
		}
	}
}

void ABlasterGameMode::PlayerEliminated(class ABlasterCharacter* ElimmedCharacter, class ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)	//淘汰
{
	if (AttackerController == nullptr || AttackerController->PlayerState == nullptr) return;
	if (VictimController == nullptr || VictimController->PlayerState == nullptr) return;
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	
	//击杀后加分
	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && BlasterGameState)
	{
		AttackerPlayerState->AddToScore(1.f);

		//当把击杀分数加进去后，检查他们是否属于得分最高的玩家。
		BlasterGameState->UpdateTopScore(AttackerPlayerState);
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
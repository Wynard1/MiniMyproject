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
	//���أ��ʼ���Ƚ��롰��ʼǰ״̬��
	bDelayedStart = true;
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();

	//��㣬��¼�ʼ��ʱ��
	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//��ǰ
	if (MatchState == MatchState::WaitingToStart)
	{
		//ÿ��TICK���¿�ʼ����׼��ʱ��
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;

		//����ʱ���ˣ�����Inprogress
		if (CountdownTime <= 0.f)
		{
			//��"��ʼǰ"���ɵ�Play
			StartMatch();
		}
	}

	//����
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}

	//������ȴ����
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
	����ÿ��matchstate����Ϸģʽ�иı�ʱ�����ͻ���������ҿ�������ѭ��
	*/
	Super::OnMatchStateSet();

	//ѭ���������е���ҿ�����������MatchState
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(*It);
		if (BlasterPlayer)
		{
			BlasterPlayer->OnMatchStateSet(MatchState);
		}
	}
}

void ABlasterGameMode::PlayerEliminated(class ABlasterCharacter* ElimmedCharacter, class ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)	//��̭
{
	if (AttackerController == nullptr || AttackerController->PlayerState == nullptr) return;
	if (VictimController == nullptr || VictimController->PlayerState == nullptr) return;
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	
	//��ɱ��ӷ�
	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && BlasterGameState)
	{
		AttackerPlayerState->AddToScore(1.f);

		//���ѻ�ɱ�����ӽ�ȥ�󣬼�������Ƿ����ڵ÷���ߵ���ҡ�
		BlasterGameState->UpdateTopScore(AttackerPlayerState);
	}

	//����ɱ���¼��������
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	//���Ž�ɫ�����Ķ�Ч����Ч
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	
	
	if (ElimmedCharacter)
	{
		//����Character��Controller
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
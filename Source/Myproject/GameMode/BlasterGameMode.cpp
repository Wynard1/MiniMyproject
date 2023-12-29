// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "Myproject/Character/BlasterCharacter.h"
#include "Myproject/PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Myproject/PlayerState/BlasterPlayerState.h"


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

	if (MatchState == MatchState::WaitingToStart)
	{
		//ÿ��TICK���¿�ʼ����׼��ʱ��
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		
		//����ʱ����
		if (CountdownTime <= 0.f)
		{
			//��"��ʼǰ"���ɵ�Play
			StartMatch();
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

	//��ɱ��ӷ�
	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(1.f);
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
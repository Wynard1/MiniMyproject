// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameState.h"
#include "Net/UnrealNetwork.h"
#include "Myproject/PlayerState/BlasterPlayerState.h"

void ABlasterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterGameState, TopScoringPlayers);
}

void ABlasterGameState::UpdateTopScore(class ABlasterPlayerState* ScoringPlayer)
{
	//����Ϊ��
	if (TopScoringPlayers.Num() == 0)
	{
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}

	//����һ�������ǳ�������һ��ͬ�ֵ�
	else if (ScoringPlayer->GetScore() == TopScore)
	{
		//AddUnique��ȷ������������������ظ���������Ҫ���һ���Ѿ��������е���ң���ֱ�Ӳ��������
		TopScoringPlayers.AddUnique(ScoringPlayer);
	}

	//�����д�
	else if (ScoringPlayer->GetScore() > TopScore)
	{
		//�������
		TopScoringPlayers.Empty();

		//�����߷�ѡ��
		TopScoringPlayers.AddUnique(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
}
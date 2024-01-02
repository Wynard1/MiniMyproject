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
	//数组为空
	if (TopScoringPlayers.Num() == 0)
	{
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}

	//已有一个，但是出现了另一个同分的
	else if (ScoringPlayer->GetScore() == TopScore)
	{
		//AddUnique将确保不会向数组中添加重复项；如果我们要添加一个已经在数组中的玩家，会直接不添加它。
		TopScoringPlayers.AddUnique(ScoringPlayer);
	}

	//比已有大
	else if (ScoringPlayer->GetScore() > TopScore)
	{
		//清空数组
		TopScoringPlayers.Empty();

		//添加最高分选手
		TopScoringPlayers.AddUnique(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
}
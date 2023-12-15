// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.generated.h"

/**
 *
 */
UCLASS()
class MYPROJECT_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty >& OutLifetimeProps) const override;

	/**
	* Replication notifies
	*/
	virtual void OnRep_Score() override;	//重写不需要UFUNCTION()
	
	UFUNCTION()
	virtual void OnRep_Defeats();			//自己写要UFUNCTION()

	//分数
	void AddToScore(float ScoreAmount);

	//死亡数
	void AddToDefeats(int32 DefeatsAmount);
private:
	UPROPERTY()
	class ABlasterCharacter* Character;
	UPROPERTY()
	class ABlasterPlayerController* Controller;

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats;
};
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

/**
 *
 */
UCLASS()
class MYPROJECT_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	//更新生命值
	void SetHUDHealth(float Health, float MaxHealth);

	//设置Score和Defeat
	void SetHUDScore(float Score);				//这个函数是继承的playerstate自带的，参数是float
	void SetHUDDefeats(int32 Defeats);			//我们自建的，用整数

	void SetHUDWeaponAmmo(int32 Ammo);

	//重生时更换Pawn会调用
	virtual void OnPossess(APawn* InPawn) override;
protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	class ABlasterHUD* BlasterHUD;

};

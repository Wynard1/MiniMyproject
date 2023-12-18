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
	//��������ֵ
	void SetHUDHealth(float Health, float MaxHealth);

	//����Score��Defeat
	void SetHUDScore(float Score);				//��������Ǽ̳е�playerstate�Դ��ģ�������float
	void SetHUDDefeats(int32 Defeats);			//�����Խ��ģ�������

	void SetHUDWeaponAmmo(int32 Ammo);

	//����ʱ����Pawn�����
	virtual void OnPossess(APawn* InPawn) override;
protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	class ABlasterHUD* BlasterHUD;

};

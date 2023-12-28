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

	void SetHUDCarriedAmmo(int32 Ammo);

	void SetHUDMatchCountdown(float CountdownTime);

	//����ʱ����Pawn�����
	virtual void OnPossess(APawn* InPawn) override;

	virtual void Tick(float DeltaTime) override;

	virtual float GetServerTime(); // Synced with server world clock

	virtual void ReceivedPlayer() override; // Sync with server clock as soon as possible
protected:
	virtual void BeginPlay() override;

	void SetHUDTime();

	/**
	* Sync time between client and server
	*/

	// Requests the current server time, passing in the client's time when the request was sent
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	// Reports the current server time to the client in response to ServerRequestServerTime
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	float ClientServerDelta = 0.f; // difference between client and server time

	//����Ƶ�Σ�δ����ʱ�䳬���˾�ִ�и���
	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;

	//δ���µ�ʱ�䡣ÿ��tick����delta time
	float TimeSyncRunningTime = 0.f;

	void CheckTimeSync(float DeltaTime);

private:
	UPROPERTY()
	class ABlasterHUD* BlasterHUD;

	//��ʼʱ��(�ܶԾ�ʱ��)
	float MatchTime = 120.f;

	//ʣ��ʱ��
	uint32 CountdownInt = 0;
};

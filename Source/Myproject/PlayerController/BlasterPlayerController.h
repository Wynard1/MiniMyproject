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

	void SetHUDCarriedAmmo(int32 Ammo);

	void SetHUDMatchCountdown(float CountdownTime);

	//重生时更换Pawn会调用
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

	//更新频次，未更新时间超过此就执行更新
	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;

	//未更新的时间。每次tick增加delta time
	float TimeSyncRunningTime = 0.f;

	void CheckTimeSync(float DeltaTime);

private:
	UPROPERTY()
	class ABlasterHUD* BlasterHUD;

	//初始时间(总对局时间)
	float MatchTime = 120.f;

	//剩余时间
	uint32 CountdownInt = 0;
};

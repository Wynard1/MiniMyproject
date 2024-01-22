// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnPoint.generated.h"

UCLASS()
class MYPROJECT_API APickupSpawnPoint : public AActor
{
	GENERATED_BODY()
	
public:
	APickupSpawnPoint();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	// 在编辑器中可配置的数组，用于存储不同类型的Pickup类
	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<class APickup>> PickupClasses;

	// 保存当前生成的Pickup对象的指针
	UPROPERTY()
	APickup* SpawnedPickup;

	// 生成Pickup的函数
	void SpawnPickup();

	// 定时器完成时调用的函数
	void SpawnPickupTimerFinished();

	// 绑定到OnDestroyed委托的回调函数，当生成的Pickup被销毁时调用
	UFUNCTION()
	void StartSpawnPickupTimer(AActor* DestroyedActor);
private:
	//定时器句柄，用于生成Pickup的定时器
	FTimerHandle SpawnPickupTimer;

	//生成Pickup的最小时间间隔
	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMin;

	//生成Pickup的最大时间间隔
	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMax;
public:



};

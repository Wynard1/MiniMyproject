// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupSpawnPoint.h"
#include "Pickup.h"

// 构造函数，设置主要的Actor属性
APickupSpawnPoint::APickupSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = true;  // 允许每帧调用
	bReplicates = true;  // 允许复制
}

void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	//启动定时器
	//因为StartSpawnPickupTimer绑定了OnDestroyed委托，所以格式必须一致、需要传参
	StartSpawnPickupTimer(nullptr);  
}

// 生成Pickup的函数
void APickupSpawnPoint::SpawnPickup()
{
	// 获取PickupClasses数组的长度
	int32 NumPickupClasses = PickupClasses.Num();

	if (NumPickupClasses > 0)
	{
		// 随机选择一个Pickup类
		int32 Selection = FMath::RandRange(0, NumPickupClasses - 1);

		// 使用SpawnActor函数生成生成Pickup类
		SpawnedPickup = GetWorld()->SpawnActor<APickup>(PickupClasses[Selection], GetActorTransform());

		// 如果具有服务器权限且成功生成Pickup
		if (HasAuthority() && SpawnedPickup)
		{
			//绑定OnDestroyed委托(OnDestroyed:当this也就是APickupSpawnPoint实例被销毁的时候，执行StartSpawnPickupTimer)
			//动态绑定AddDynamic后不需要显示调用，也会实时检测
			SpawnedPickup->OnDestroyed.AddDynamic(this, &APickupSpawnPoint::StartSpawnPickupTimer);
		}
	}
}

// 当生成Pickup的定时器完成时调用的函数
void APickupSpawnPoint::SpawnPickupTimerFinished()
{
	if (HasAuthority())// 如果具有服务器权限
	{
		SpawnPickup();  //生成Pickup
	}
}

// 启动生成Pickup的定时器的函数
void APickupSpawnPoint::StartSpawnPickupTimer(AActor* DestroyedActor)
{
	const float SpawnTime = FMath::FRandRange(SpawnPickupTimeMin, SpawnPickupTimeMax);

	// 设置生成Pickup的定时器，当时间到达时调用SpawnPickupTimerFinished函数
	GetWorldTimerManager().SetTimer(
		SpawnPickupTimer,
		this,
		&APickupSpawnPoint::SpawnPickupTimerFinished,
		SpawnTime
	);
}

void APickupSpawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
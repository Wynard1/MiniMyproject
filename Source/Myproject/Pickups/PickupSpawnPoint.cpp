// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupSpawnPoint.h"
#include "Pickup.h"

// ���캯����������Ҫ��Actor����
APickupSpawnPoint::APickupSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = true;  // ����ÿ֡����
	bReplicates = true;  // ������
}

void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	//������ʱ��
	//��ΪStartSpawnPickupTimer����OnDestroyedί�У����Ը�ʽ����һ�¡���Ҫ����
	StartSpawnPickupTimer(nullptr);  
}

// ����Pickup�ĺ���
void APickupSpawnPoint::SpawnPickup()
{
	// ��ȡPickupClasses����ĳ���
	int32 NumPickupClasses = PickupClasses.Num();

	if (NumPickupClasses > 0)
	{
		// ���ѡ��һ��Pickup��
		int32 Selection = FMath::RandRange(0, NumPickupClasses - 1);

		// ʹ��SpawnActor������������Pickup��
		SpawnedPickup = GetWorld()->SpawnActor<APickup>(PickupClasses[Selection], GetActorTransform());

		// ������з�����Ȩ���ҳɹ�����Pickup
		if (HasAuthority() && SpawnedPickup)
		{
			//��OnDestroyedί��(OnDestroyed:��thisҲ����APickupSpawnPointʵ�������ٵ�ʱ��ִ��StartSpawnPickupTimer)
			//��̬��AddDynamic����Ҫ��ʾ���ã�Ҳ��ʵʱ���
			SpawnedPickup->OnDestroyed.AddDynamic(this, &APickupSpawnPoint::StartSpawnPickupTimer);
		}
	}
}

// ������Pickup�Ķ�ʱ�����ʱ���õĺ���
void APickupSpawnPoint::SpawnPickupTimerFinished()
{
	if (HasAuthority())// ������з�����Ȩ��
	{
		SpawnPickup();  //����Pickup
	}
}

// ��������Pickup�Ķ�ʱ���ĺ���
void APickupSpawnPoint::StartSpawnPickupTimer(AActor* DestroyedActor)
{
	const float SpawnTime = FMath::FRandRange(SpawnPickupTimeMin, SpawnPickupTimeMax);

	// ��������Pickup�Ķ�ʱ������ʱ�䵽��ʱ����SpawnPickupTimerFinished����
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
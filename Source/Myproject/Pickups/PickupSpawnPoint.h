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

	// �ڱ༭���п����õ����飬���ڴ洢��ͬ���͵�Pickup��
	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<class APickup>> PickupClasses;

	// ���浱ǰ���ɵ�Pickup�����ָ��
	UPROPERTY()
	APickup* SpawnedPickup;

	// ����Pickup�ĺ���
	void SpawnPickup();

	// ��ʱ�����ʱ���õĺ���
	void SpawnPickupTimerFinished();

	// �󶨵�OnDestroyedί�еĻص������������ɵ�Pickup������ʱ����
	UFUNCTION()
	void StartSpawnPickupTimer(AActor* DestroyedActor);
private:
	//��ʱ���������������Pickup�Ķ�ʱ��
	FTimerHandle SpawnPickupTimer;

	//����Pickup����Сʱ����
	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMin;

	//����Pickup�����ʱ����
	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMax;
public:



};

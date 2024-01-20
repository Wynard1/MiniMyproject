// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "HealthPickup.generated.h"

UCLASS()
class MYPROJECT_API AHealthPickup : public APickup
{
	GENERATED_BODY()

public:
	// ���캯��
	AHealthPickup();

protected:
	// ���Ǹ����OnSphereOverlap����
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

private:
	// ������
	UPROPERTY(EditAnywhere, Category = "Healing")
	float HealAmount = 100.f;

	// ����ʱ��
	UPROPERTY(EditAnywhere, Category = "Healing")
	float HealingTime = 5.f;

};


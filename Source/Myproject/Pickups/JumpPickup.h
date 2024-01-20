// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "JumpPickup.generated.h"

/**
 * 
 */
UCLASS()
class MYPROJECT_API AJumpPickup : public APickup
{
	GENERATED_BODY()
protected:
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);
private:

	// ��Ծ�ٶ���������ֵ
	UPROPERTY(EditAnywhere)
	float JumpZVelocityBuff = 4000.f;

	// ��Ծ�����ĳ���ʱ��
	UPROPERTY(EditAnywhere)
	float JumpBuffTime = 30.f;

};

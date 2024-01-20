// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MYPROJECT_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBuffComponent();
	friend class ABlasterCharacter;

	void Heal(float HealAmount, float HealingTime);

	void BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime);

	void BuffJump(float BuffJumpVelocity, float BuffTime);

	void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed);

	void SetInitialJumpVelocity(float Velocity);
protected:
	virtual void BeginPlay() override;

	void HealRampUp(float DeltaTime);

private:
	UPROPERTY()
	class ABlasterCharacter* Character;

	/**
	* Heal buff
	*/

	bool bHealing = false;

	// ��������
	float HealingRate = 0;

	// ���Ƶľ�����ֵ
	float AmountToHeal = 0.f;

	/**
	* Speed buff
	*/

	// ���ڹ����ٶ��������ʱ��Ķ�ʱ�����
	FTimerHandle SpeedBuffTimer;

	// ���ý�ɫ�ٶȵ���ʼֵ�ĺ���
	void ResetSpeeds();

	// ��ɫ�ĳ�ʼ�����ٶ�
	float InitialBaseSpeed;

	// ��ɫ�ĳ�ʼ�׷��ٶ�
	float InitialCrouchSpeed;

	// ͨ���ಥRPCӦ���ٶ����浽���пͻ��˵ĺ���
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed);

	/**
	* Jump buff
	*/
	FTimerHandle JumpBuffTimer;
	void ResetJump();
	float InitialJumpVelocity;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpBuff(float JumpVelocity);

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};

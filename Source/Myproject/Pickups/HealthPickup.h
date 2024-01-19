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
	// 构造函数
	AHealthPickup();

	// 覆盖父类的Destroyed函数
	virtual void Destroyed() override;

protected:
	// 覆盖父类的OnSphereOverlap函数
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

private:
	// 治疗量
	UPROPERTY(EditAnywhere, Category = "Healing")
	float HealAmount = 100.f;

	// 治疗时间
	UPROPERTY(EditAnywhere, Category = "Healing")
	float HealingTime = 5.f;

	// 拾取物品展示用的粒子效果
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	class UNiagaraComponent* PickupEffectComponent;

	// 拾取时触发的特效
	UPROPERTY(EditAnywhere, Category = "Effects")
	class UNiagaraSystem* PickupEffect;
};


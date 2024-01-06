// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitScanWeapon.generated.h"

/**
 * HitScanWeapon类：用于处理射线命中的武器逻辑
 */
UCLASS()
class MYPROJECT_API AHitScanWeapon : public AWeapon
{
    GENERATED_BODY()

public:
    /**
     * 在命中目标位置进行射击
     * @param HitTarget 射击命中的目标位置
     */
    virtual void Fire(const FVector& HitTarget) override;

private:
    /**
     * 武器造成的伤害数值
     */
    UPROPERTY(EditAnywhere, Category = "Weapon")
    float Damage = 20.f;

    /**
     * 碰撞产生的粒子效果
     */
    UPROPERTY(EditAnywhere, Category = "Effects")
    class UParticleSystem* ImpactParticles;
};
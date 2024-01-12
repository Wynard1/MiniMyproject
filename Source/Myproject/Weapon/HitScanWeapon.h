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

protected:

    FVector TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget);
    void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit);
    /*
     * 碰撞产生的粒子效果
     */
    UPROPERTY(EditAnywhere, Category = "Effects")
    class UParticleSystem* ImpactParticles;
    
    // 命中时播放的声音
    UPROPERTY(EditAnywhere, Category = "Weapon")
    USoundCue* HitSound;
    
    /*
     * 武器造成的伤害数值
     */
    UPROPERTY(EditAnywhere)
    float Damage = 20.f;

private:
    // 尾气粒子特效
    UPROPERTY(EditAnywhere)
    UParticleSystem* BeamParticles;

    // 枪口火焰特效
    UPROPERTY(EditAnywhere)
    UParticleSystem* MuzzleFlash;

    // 射击时播放的声音
    UPROPERTY(EditAnywhere)
    USoundCue* FireSound;

    /**
     * 使用散射来确定射线终点
     */

     // 武器散射属性
    UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
    float DistanceToSphere = 800.f; // 到散射球的距离

    UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
    float SphereRadius = 75.f; // 散射球半径

    UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
    bool bUseScatter = false; // 是否使用散射


};
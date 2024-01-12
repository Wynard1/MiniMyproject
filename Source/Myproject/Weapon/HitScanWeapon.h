// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitScanWeapon.generated.h"

/**
 * HitScanWeapon�ࣺ���ڴ����������е������߼�
 */
UCLASS()
class MYPROJECT_API AHitScanWeapon : public AWeapon
{
    GENERATED_BODY()

public:
    /**
     * ������Ŀ��λ�ý������
     * @param HitTarget ������е�Ŀ��λ��
     */
    virtual void Fire(const FVector& HitTarget) override;

protected:

    FVector TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget);
    void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit);
    /*
     * ��ײ����������Ч��
     */
    UPROPERTY(EditAnywhere, Category = "Effects")
    class UParticleSystem* ImpactParticles;
    
    // ����ʱ���ŵ�����
    UPROPERTY(EditAnywhere, Category = "Weapon")
    USoundCue* HitSound;
    
    /*
     * ������ɵ��˺���ֵ
     */
    UPROPERTY(EditAnywhere)
    float Damage = 20.f;

private:
    // β��������Ч
    UPROPERTY(EditAnywhere)
    UParticleSystem* BeamParticles;

    // ǹ�ڻ�����Ч
    UPROPERTY(EditAnywhere)
    UParticleSystem* MuzzleFlash;

    // ���ʱ���ŵ�����
    UPROPERTY(EditAnywhere)
    USoundCue* FireSound;

    /**
     * ʹ��ɢ����ȷ�������յ�
     */

     // ����ɢ������
    UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
    float DistanceToSphere = 800.f; // ��ɢ����ľ���

    UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
    float SphereRadius = 75.f; // ɢ����뾶

    UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
    bool bUseScatter = false; // �Ƿ�ʹ��ɢ��


};
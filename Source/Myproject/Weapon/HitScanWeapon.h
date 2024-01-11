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

private:
    /**
     * ������ɵ��˺���ֵ
     */
    UPROPERTY(EditAnywhere, Category = "Weapon")
    float Damage = 20.f;

    /**
     * ��ײ����������Ч��
     */
    UPROPERTY(EditAnywhere, Category = "Effects")
    class UParticleSystem* ImpactParticles;

    // β��������Ч�����ڱ༭���и���
    UPROPERTY(EditAnywhere)
    UParticleSystem* BeamParticles;

    // ǹ�ڻ�����Ч�����ڱ༭���и���
    UPROPERTY(EditAnywhere)
    UParticleSystem* MuzzleFlash;

    // ���ʱ���ŵ����������ڱ༭���и���
    UPROPERTY(EditAnywhere)
    USoundCue* FireSound;

    // ����ʱ���ŵ����������ڱ༭���и���
    UPROPERTY(EditAnywhere)
    USoundCue* HitSound;

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
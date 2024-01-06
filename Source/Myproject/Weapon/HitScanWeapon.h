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
};
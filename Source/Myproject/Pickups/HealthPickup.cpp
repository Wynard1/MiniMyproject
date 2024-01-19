// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthPickup.h"
#include "Myproject/Character/BlasterCharacter.h"
#include "Myproject/BlasterComponents/BuffComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

//���캯��
AHealthPickup::AHealthPickup()
{
    // ����Ϊ�������и���
    bReplicates = true;

    // ���� Niagara ����������丽�ŵ������
    PickupEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PickupEffectComponent"));
    PickupEffectComponent->SetupAttachment(RootComponent);
}

// ���������ص�ʱ���õĺ���
void AHealthPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

    ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
    if (BlasterCharacter)
    {
        
    }

    // ����
    Destroy();
}

// �� HealthPickup ʵ��������ʱ���õĺ���
void AHealthPickup::Destroyed()
{
    if (PickupEffect)
    {
        // ��ʵ��������ʱ����λ������ Niagara ϵͳ
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            this,
            PickupEffect,
            GetActorLocation(),
            GetActorRotation()
        );
    }

    // ���ø���� Destroyed ����
    Super::Destroyed();
}

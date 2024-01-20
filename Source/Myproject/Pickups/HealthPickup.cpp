// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthPickup.h"
#include "Myproject/Character/BlasterCharacter.h"
#include "Myproject/BlasterComponents/BuffComponent.h"

//���캯��
AHealthPickup::AHealthPickup()
{
    // ����Ϊ�������и���
    bReplicates = true;

}

// ���������ص�ʱ���õĺ���
void AHealthPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

    ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
    
    //Ӧ������Ч��
    if (BlasterCharacter)
    {
        UBuffComponent* Buff = BlasterCharacter->GetBuff();
        if (Buff)
        {
            Buff->Heal(HealAmount, HealingTime);
        }
    }

    // ����
    Destroy();
}


// Fill out your copyright notice in the Description page of Project Settings.


#include "SpeedPickup.h"
#include "Myproject/Character/BlasterCharacter.h"
#include "Myproject/BlasterComponents/BuffComponent.h"

// �ص��¼�
void ASpeedPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

    ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);

    if (BlasterCharacter)
    {
        // ��ȡBlasterCharacter��BuffComponent
        UBuffComponent* Buff = BlasterCharacter->GetBuff();

        if (Buff)
        {
            // ����BuffComponent��BuffSpeed�����������ٶ��������
            Buff->BuffSpeed(BaseSpeedBuff, CrouchSpeedBuff, SpeedBuffTime);
        }
    }

    // �����ٶ�ʰȡ��
    Destroy();
}

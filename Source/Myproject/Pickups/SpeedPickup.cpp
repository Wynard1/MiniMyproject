// Fill out your copyright notice in the Description page of Project Settings.


#include "SpeedPickup.h"
#include "Myproject/Character/BlasterCharacter.h"
#include "Myproject/BlasterComponents/BuffComponent.h"

// 重叠事件
void ASpeedPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

    ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);

    if (BlasterCharacter)
    {
        // 获取BlasterCharacter的BuffComponent
        UBuffComponent* Buff = BlasterCharacter->GetBuff();

        if (Buff)
        {
            // 调用BuffComponent的BuffSpeed函数，传递速度增益参数
            Buff->BuffSpeed(BaseSpeedBuff, CrouchSpeedBuff, SpeedBuffTime);
        }
    }

    // 销毁速度拾取物
    Destroy();
}

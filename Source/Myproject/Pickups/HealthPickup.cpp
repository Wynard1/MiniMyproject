// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthPickup.h"
#include "Myproject/Character/BlasterCharacter.h"
#include "Myproject/BlasterComponents/BuffComponent.h"

//构造函数
AHealthPickup::AHealthPickup()
{
    // 设置为在网络中复制
    bReplicates = true;

}

// 当与球体重叠时调用的函数
void AHealthPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

    ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
    
    //应用治疗效果
    if (BlasterCharacter)
    {
        UBuffComponent* Buff = BlasterCharacter->GetBuff();
        if (Buff)
        {
            Buff->Heal(HealAmount, HealingTime);
        }
    }

    // 销毁
    Destroy();
}


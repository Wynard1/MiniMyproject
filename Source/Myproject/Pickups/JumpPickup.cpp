// Fill out your copyright notice in the Description page of Project Settings.


#include "JumpPickup.h"
#include "Myproject/Character/BlasterCharacter.h"
#include "Myproject/BlasterComponents/BuffComponent.h"

void AJumpPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
        // 获取BlasterCharacter的BuffComponent
        UBuffComponent* Buff = BlasterCharacter->GetBuff();
        if (Buff)
        {
            // 触发BuffJump函数，增加跳跃Z轴速度和持续时间
            Buff->BuffJump(JumpZVelocityBuff, JumpBuffTime);
        }
	}

	Destroy();
}
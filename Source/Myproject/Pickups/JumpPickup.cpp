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
        // ��ȡBlasterCharacter��BuffComponent
        UBuffComponent* Buff = BlasterCharacter->GetBuff();
        if (Buff)
        {
            // ����BuffJump������������ԾZ���ٶȺͳ���ʱ��
            Buff->BuffJump(JumpZVelocityBuff, JumpBuffTime);
        }
	}

	Destroy();
}
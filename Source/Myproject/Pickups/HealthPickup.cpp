// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthPickup.h"
#include "Myproject/Character/BlasterCharacter.h"
#include "Myproject/BlasterComponents/BuffComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

//构造函数
AHealthPickup::AHealthPickup()
{
    // 设置为在网络中复制
    bReplicates = true;

    // 创建 Niagara 组件并设置其附着到根组件
    PickupEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PickupEffectComponent"));
    PickupEffectComponent->SetupAttachment(RootComponent);
}

// 当与球体重叠时调用的函数
void AHealthPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

    ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
    if (BlasterCharacter)
    {
        
    }

    // 销毁
    Destroy();
}

// 当 HealthPickup 实例被销毁时调用的函数
void AHealthPickup::Destroyed()
{
    if (PickupEffect)
    {
        // 在实例被销毁时，在位置生成 Niagara 系统
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            this,
            PickupEffect,
            GetActorLocation(),
            GetActorRotation()
        );
    }

    // 调用父类的 Destroyed 函数
    Super::Destroyed();
}

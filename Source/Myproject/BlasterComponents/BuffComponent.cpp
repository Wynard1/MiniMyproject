// Fill out your copyright notice in the Description page of Project Settings.


#include "BuffComponent.h"
#include "Myproject/Character/BlasterCharacter.h"

// Sets default values for this component's properties
UBuffComponent::UBuffComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

//根据传入的HealAmount和HealingTime进行治疗设置
void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
    // 设置正在进行治疗的标志为true
    bHealing = true;

    // 计算治疗速率，即每秒治疗的量
    HealingRate = HealAmount / HealingTime;

    // 累加需要治疗的总量
    AmountToHeal += HealAmount;
}

// 在HealRampUp函数中，处理每帧的治疗逻辑
void UBuffComponent::HealRampUp(float DeltaTime)
{
    // 先决条件
    if (!bHealing || Character == nullptr || Character->IsElimmed()) return;

    // 计算本帧需要治疗的量
    const float HealThisFrame = HealingRate * DeltaTime;

    // 使用FMath::Clamp确保在0和最大生命值之间设置Character的健康值
    Character->SetHealth(FMath::Clamp(Character->GetHealth() + HealThisFrame, 0.f, Character->GetMaxHealth()));

    // 更新HUD中的健康信息
    Character->UpdateHUDHealth();

    // 减去已经治疗的量
    AmountToHeal -= HealThisFrame;

    // 如果已经治疗完成或者Character的健康值达到最大值，停止治疗
    if (AmountToHeal <= 0.f || Character->GetHealth() >= Character->GetMaxHealth())
    {
        bHealing = false;
        AmountToHeal = 0.f;
    }
}


// Called when the game starts
void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();
	
}


// Called every frame
void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    //每帧计算治疗结果
	HealRampUp(DeltaTime);
}


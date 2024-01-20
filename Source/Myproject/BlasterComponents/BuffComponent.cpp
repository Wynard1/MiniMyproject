// Fill out your copyright notice in the Description page of Project Settings.


#include "BuffComponent.h"
#include "Myproject/Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

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

// 获取角色初始速度并存储
void UBuffComponent::SetInitialSpeeds(float BaseSpeed, float CrouchSpeed)
{
    InitialBaseSpeed = BaseSpeed;
    InitialCrouchSpeed = CrouchSpeed;
}

// 获取角色初始跳跃速度并存储
void UBuffComponent::SetInitialJumpVelocity(float Velocity)
{
    InitialJumpVelocity = Velocity;
}

// 提供速度增益
void UBuffComponent::BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime)
{
    if (Character == nullptr) return;

    // 设置一个计时器，用于在一定时间后重置速度
    Character->GetWorldTimerManager().SetTimer(
        SpeedBuffTimer, //指定计时器句柄为SpeedBuffTimer
        this,   //回调函数所在类
        &UBuffComponent::ResetSpeeds,   //上述类里的回调函数
        BuffTime    // 计时器的时长
    );

    // 设置角色的最大行走速度和蹲伏速度
    if (Character->GetCharacterMovement())
    {
        Character->GetCharacterMovement()->MaxWalkSpeed = BuffBaseSpeed;
        Character->GetCharacterMovement()->MaxWalkSpeedCrouched = BuffCrouchSpeed;
    }

    // 调用多播函数，在所有客户端上同步速度增益
    MulticastSpeedBuff(BuffBaseSpeed, BuffCrouchSpeed);
}

// 重置速度到初始值
void UBuffComponent::ResetSpeeds()
{
    if (Character == nullptr || Character->GetCharacterMovement() == nullptr) return;

    // 将速度重置为初始值
    Character->GetCharacterMovement()->MaxWalkSpeed = InitialBaseSpeed;
    Character->GetCharacterMovement()->MaxWalkSpeedCrouched = InitialCrouchSpeed;

    // 调用多播函数，在所有客户端上同步速度重置
    MulticastSpeedBuff(InitialBaseSpeed, InitialCrouchSpeed);
}

// 多播函数，用于在所有客户端上同步速度信息
void UBuffComponent::MulticastSpeedBuff_Implementation(float BaseSpeed, float CrouchSpeed)
{
    // 设置所有客户端的角色速度信息
    if (Character && Character->GetCharacterMovement())
    {
        Character->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
        Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
    }

}

void UBuffComponent::BuffJump(float BuffJumpVelocity, float BuffTime)
{
    if (Character == nullptr) return;

    // 设置定时器，用于在Buff时间结束后调用ResetJump函数
    Character->GetWorldTimerManager().SetTimer(
        JumpBuffTimer,
        this,
        &UBuffComponent::ResetJump,
        BuffTime
    );

    // 如果存在角色移动组件，设置跳跃Z轴速度
    if (Character->GetCharacterMovement())
    {
        Character->GetCharacterMovement()->JumpZVelocity = BuffJumpVelocity;
    }

    // 调用MulticastJumpBuff，在所有客户端同步设置跳跃Z轴速度
    MulticastJumpBuff(BuffJumpVelocity);
}

void UBuffComponent::MulticastJumpBuff_Implementation(float JumpVelocity)
{
    // 同步设置跳跃Z轴速度
    if (Character && Character->GetCharacterMovement())
    {
        Character->GetCharacterMovement()->JumpZVelocity = JumpVelocity;
    }
}

void UBuffComponent::ResetJump()
{
    //重置跳跃Z轴速度为初始值
    if (Character->GetCharacterMovement())
    {
        Character->GetCharacterMovement()->JumpZVelocity = InitialJumpVelocity;
    }

    // 调用MulticastJumpBuff，在所有客户端同步重置跳跃Z轴速度为初始值
    MulticastJumpBuff(InitialJumpVelocity);
}


// Called every frame
void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    //每帧计算治疗结果
	HealRampUp(DeltaTime);
}


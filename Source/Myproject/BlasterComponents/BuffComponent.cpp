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

//���ݴ����HealAmount��HealingTime������������
void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
    // �������ڽ������Ƶı�־Ϊtrue
    bHealing = true;

    // �����������ʣ���ÿ�����Ƶ���
    HealingRate = HealAmount / HealingTime;

    // �ۼ���Ҫ���Ƶ�����
    AmountToHeal += HealAmount;
}

// ��HealRampUp�����У�����ÿ֡�������߼�
void UBuffComponent::HealRampUp(float DeltaTime)
{
    // �Ⱦ�����
    if (!bHealing || Character == nullptr || Character->IsElimmed()) return;

    // ���㱾֡��Ҫ���Ƶ���
    const float HealThisFrame = HealingRate * DeltaTime;

    // ʹ��FMath::Clampȷ����0���������ֵ֮������Character�Ľ���ֵ
    Character->SetHealth(FMath::Clamp(Character->GetHealth() + HealThisFrame, 0.f, Character->GetMaxHealth()));

    // ����HUD�еĽ�����Ϣ
    Character->UpdateHUDHealth();

    // ��ȥ�Ѿ����Ƶ���
    AmountToHeal -= HealThisFrame;

    // ����Ѿ�������ɻ���Character�Ľ���ֵ�ﵽ���ֵ��ֹͣ����
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

// ��ȡ��ɫ��ʼ�ٶȲ��洢
void UBuffComponent::SetInitialSpeeds(float BaseSpeed, float CrouchSpeed)
{
    InitialBaseSpeed = BaseSpeed;
    InitialCrouchSpeed = CrouchSpeed;
}

// �ṩ�ٶ�����
void UBuffComponent::BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime)
{
    if (Character == nullptr) return;

    // ����һ����ʱ����������һ��ʱ��������ٶ�
    Character->GetWorldTimerManager().SetTimer(
        SpeedBuffTimer, //ָ����ʱ�����ΪSpeedBuffTimer
        this,   //�ص�����������
        &UBuffComponent::ResetSpeeds,   //��������Ļص�����
        BuffTime    // ��ʱ����ʱ��
    );

    // ���ý�ɫ����������ٶȺͶ׷��ٶ�
    if (Character->GetCharacterMovement())
    {
        Character->GetCharacterMovement()->MaxWalkSpeed = BuffBaseSpeed;
        Character->GetCharacterMovement()->MaxWalkSpeedCrouched = BuffCrouchSpeed;
    }

    // ���öಥ�����������пͻ�����ͬ���ٶ�����
    MulticastSpeedBuff(BuffBaseSpeed, BuffCrouchSpeed);
}

// �����ٶȵ���ʼֵ
void UBuffComponent::ResetSpeeds()
{
    if (Character == nullptr || Character->GetCharacterMovement() == nullptr) return;

    // ���ٶ�����Ϊ��ʼֵ
    Character->GetCharacterMovement()->MaxWalkSpeed = InitialBaseSpeed;
    Character->GetCharacterMovement()->MaxWalkSpeedCrouched = InitialCrouchSpeed;

    // ���öಥ�����������пͻ�����ͬ���ٶ�����
    MulticastSpeedBuff(InitialBaseSpeed, InitialCrouchSpeed);
}

// �ಥ���������������пͻ�����ͬ���ٶ���Ϣ
void UBuffComponent::MulticastSpeedBuff_Implementation(float BaseSpeed, float CrouchSpeed)
{
    // �������пͻ��˵Ľ�ɫ�ٶ���Ϣ
    Character->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
    Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
}

// Called every frame
void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    //ÿ֡�������ƽ��
	HealRampUp(DeltaTime);
}


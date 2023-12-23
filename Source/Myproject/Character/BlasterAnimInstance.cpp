// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterAnimInstance.h" 
#include "BlasterCharacter.h" 
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Myproject/Weapon/Weapon.h"
#include "Myproject/BlasterTypes/CombatState.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (BlasterCharacter == nullptr)
	{
		BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
	}
	if (BlasterCharacter == nullptr) return;

	FVector Velocity = BlasterCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	//(ͨ��getter)��ʼ�������Ĵ����
	bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
	bWeaponEquipped = BlasterCharacter->IsWeaponEquipped();
	EquippedWeapon = BlasterCharacter->GetEquippedWeapon();
	bIsCrouched = BlasterCharacter->bIsCrouched;
	bAiming = BlasterCharacter->IsAiming();
	TurningInPlace = BlasterCharacter->GetTurningInPlace();
	bRotateRootBone = BlasterCharacter->ShouldRotateRootBone();

	bElimmed = BlasterCharacter->IsElimmed();

	// Offset Yaw for Strafing
	FRotator AimRotation = BlasterCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());
	YawOffset = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f);
	YawOffset = DeltaRotation.Yaw;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasterCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	AO_Yaw = BlasterCharacter->GetAO_Yaw();
	AO_Pitch = BlasterCharacter->GetAO_Pitch();

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BlasterCharacter->GetMesh())
	{
		//���ǹ��۵���Ϣ����������ΪFTransform
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		
		//TransformToBoneSpace�������õ��������������洢�ţ����ֲ������Ϣת��Ϊǹ����ϵ�е���Ϣ
		FVector OutPosition;
		FRotator OutRotation;
		
		//����������ϵת����ǹ����ϵ
		//�������OutPosition��OutRotation��
		BlasterCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		
		  
		//ת�������Ϣ���õ�LeftHandTransform��
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));//SetRotation��ҪFQuat

		/*
		//ǹ�� DEBUGLINE
		FTransform MuzzleTipTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleFlash"), ERelativeTransformSpace::RTS_World);
		FVector MuzzleX(FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X));
		DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX * 1000.f, FColor::Red);

		//׼��DEBUGLINE
		DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), BlasterCharacter->GetHitTarget(), FColor::Orange);
		*/
		
		if (BlasterCharacter->IsLocallyControlled())//ֻ�ڱ���ִ��,û��Ҫ����
		{
			bLocallyControlled = true;

			//������ֹ�����λ����Ϣ
			//FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("Hand_R"), ERelativeTransformSpace::RTS_World);
			FTransform RightHandTransform = BlasterCharacter->GetMesh()->GetSocketTransform(FName("hand_r"), ERelativeTransformSpace::RTS_World);

			//�������ֹ���Ҫת����λ�ã��ڶ�����ͼ��ʹ����
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BlasterCharacter->GetHitTarget()));
			
			//������ֵ����Ȼ������ͻȻ���ֱ��MESH������ת�����Ļ��������Ӳ
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f);
		}
	}

	bUseFABRIK = BlasterCharacter->GetCombatState() != ECombatState::ECS_Reloading;
}


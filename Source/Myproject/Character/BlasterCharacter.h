// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Myproject/BlasterTypes/TurningInPlace.h"
#include "Myproject/Interfaces/InteractWithCrosshairsInterface.h"
#include "BlasterCharacter.generated.h"


UCLASS()
class MYPROJECT_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;

	//��̫�沥��
	void PlayFireMontage(bool bAiming);
	
	//�ܻ�����RPC
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastHit();

	virtual void OnRep_ReplicatedMovement() override;

protected:
	virtual void BeginPlay() override;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void AimOffset(float DeltaTime);

	void CalculateAO_Pitch();

	//����ԭ����ת
	void SimProxiesTurn();

	virtual void Jump() override;//��дJump����

	//����������
	void FireButtonPressed();
	void FireButtonReleased();

	//�ܻ���������
	void PlayHitReactMontage();

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* Combat1;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	float AO_Yaw;
	float AO_Pitch;

	float InterpAO_Yaw;//4.20 �洢���ﳯ��ת���Ƕ�

	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;	//�����ö�����ͣ�Ҫ���ͷ�ļ���

	void TurnInPlace(float DeltaTime);//��������TurningInPlace�ĺ���������deltatimeΪ��ֵ��׼��

	//��̫�����͵ı���������ͼ������
	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage;

	//�ܻ�Montage
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage;

	//��ͷ̫�����ؽ�ɫ�ĺ�����TICKִ��
	void HideCameraIfCharacterClose();

	//���ؽ�ɫ�Ŀ�ǽ����
	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;

	//������ͼ�������ж�
	bool bRotateRootBone;

	//ԭ����תʱ���Ŷ�������Ĳ���
	float TurnThreshold = 0.5f;			//��ת��ֵ
	FRotator ProxyRotationLastFrame;	//���һ֡��ת
	FRotator ProxyRotation;				//��ǰ��ת
	float ProxyYaw;
	float TimeSinceLastMovementReplication;
	float CalculateSpeed();

	/**
* Player health
*/

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health();

	class ABlasterPlayerController* BlasterPlayerController;

public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	AWeapon* GetEquippedWeapon();//getter

	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }	//��������ͼ��getter

	FVector GetHitTarget() const;

	//����FOV�����getter
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
};
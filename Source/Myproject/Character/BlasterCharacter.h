// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Myproject/BlasterTypes/TurningInPlace.h"
#include "Myproject/Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "Myproject/BlasterTypes/CombatState.h"
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

	//������̫�沥��
	void PlayFireMontage(bool bAiming);

	void PlayReloadMontage();
	
	//���ű���ɱ����
	void PlayElimMontage();

	//Ͷ�����񵯶���
	void PlayThrowGrenadeMontage();
	
	virtual void OnRep_ReplicatedMovement() override;

	//������
	void Elim();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();

	virtual void Destroyed() override;

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	//������ʾ��׼���ĺ���
	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

	void UpdateHUDHealth();
	void UpdateHUDShield();
protected:
	virtual void BeginPlay() override;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void ReloadButtonPressed();
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

	void GrenadeButtonPressed();

	//apply damage�Ļص�����
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);

	// Poll for any relelvant classes and initialize our HUD
	void PollInit();

	void RotateInPlace(float DeltaTime);
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

	//BlueprintReadOnly ���� meta = (AllowPrivateAccess = "true") ʹ�˱�����������ͼ�����
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* Combat1;

	UPROPERTY(VisibleAnywhere)
	class UBuffComponent* Buff;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	float AO_Yaw;
	float AO_Pitch;

	float InterpAO_Yaw;//4.20 �洢���ﳯ��ת���Ƕ�

	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;	//�����ö�����ͣ�Ҫ���ͷ�ļ���

	void TurnInPlace(float DeltaTime);//��������TurningInPlace�ĺ���������deltatimeΪ��ֵ��׼��

	/**
	* Animation montages
	*/
	//��̫�����͵ı���������ͼ������
	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ReloadMontage;

	//�ܻ�Montage
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage;

	//��������
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ElimMontage;

	//���׶���
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ThrowGrenadeMontage;

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
	void OnRep_Health(float LastHealth);

	/**
	* Player shield
	*/

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxShield = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Shield, EditAnywhere, Category = "Player Stats")
	float Shield = 0.f;

	UFUNCTION()
	void OnRep_Shield(float LastShield);

	UPROPERTY()
	class ABlasterPlayerController* BlasterPlayerController;

	bool bElimmed = false;

	/**
* ElimTimer;
*/
	FTimerHandle ElimTimer;

	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;

	void ElimTimerFinished();

	/**
	* Dissolve effect
	*/

	//timeline
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;
	
	//Track
	FOnTimelineFloat DissolveTrack;	

	//Curve
	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;

	//Timeline�ĵ��úͻص�
	//����
	void StartDissolve();

	//���ڣ�ÿ���������������ʱ�������ʱ��������Ҫһ��ÿ֡�����õĻص������������Ǹ���ʱ����ʱ���⽫�ǽ�����Ӧ����ֵ�ĺ����������ϵ�λ�á�
	//�ص�
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	

	// Dynamic instance that we can change at runtime
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	// Material instance set on the Blueprint, used with the dynamic material instance
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance;

	/**
	* Elim bot
	*/

	UPROPERTY(EditAnywhere)
	UParticleSystem* ElimBotEffect;

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* ElimBotComponent;

	UPROPERTY(EditAnywhere)
	class USoundCue* ElimBotSound;

	UPROPERTY()
	class ABlasterPlayerState* BlasterPlayerState;

	/**
	* Grenade
	*/
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade;

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
	
	FORCEINLINE bool IsElimmed() const { return bElimmed; }

	//��ȡѪ����getter
	FORCEINLINE float GetHealth() const { return Health; }

	FORCEINLINE void SetHealth(float Amount) { Health = Amount; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE void SetShield(float Amount) { Shield = Amount; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }

	ECombatState GetCombatState() const;

	FORCEINLINE UCombatComponent* GetCombat() const { return Combat1; }
	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }

	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }

	// ��ȡ�Ѹ��ŵ����׵ľ�̬�������
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }

	FORCEINLINE UBuffComponent* GetBuff() const { return Buff; }
};
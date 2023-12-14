// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Myproject/BlasterTypes/TurningInPlace.h"
#include "Myproject/Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
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

	//开火蒙太奇播放
	void PlayFireMontage(bool bAiming);
	
	//播放被击杀动画
	void PlayElimMontage();
	
	virtual void OnRep_ReplicatedMovement() override;

	//清除玩家
	void Elim();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();

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

	//代理原地旋转
	void SimProxiesTurn();

	virtual void Jump() override;//重写Jump函数

	//鼠标左键开火
	void FireButtonPressed();
	void FireButtonReleased();

	//受击动画播放
	void PlayHitReactMontage();

	//apply damage的回调函数
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);

	void UpdateHUDHealth();

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

	float InterpAO_Yaw;//4.20 存储人物朝向转动角度

	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;	//定义的枚举类型，要添加头文件。

	void TurnInPlace(float DeltaTime);//用于设置TurningInPlace的函数。传入deltatime为插值做准备

	//蒙太奇类型的变量，在蓝图里设置
	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage;

	//受击Montage
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage;

	//死亡动画
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ElimMontage;

	//镜头太近隐藏角色的函数；TICK执行
	void HideCameraIfCharacterClose();

	//隐藏角色的靠墙距离
	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;

	//动画蓝图中用来判断
	bool bRotateRootBone;

	//原地旋转时播放动画所需的参数
	float TurnThreshold = 0.5f;			//旋转阈值
	FRotator ProxyRotationLastFrame;	//最后一帧旋转
	FRotator ProxyRotation;				//当前旋转
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

	//Timeline的调用和回调
	//调用
	void StartDissolve();

	//现在，每当我们添加这样的时间轴组件时，我们需要一个每帧被调用的回调函数。当我们更新时间轴时，这将是接收相应浮点值的函数到曲线上的位置。
	//回调
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	

	// Dynamic instance that we can change at runtime
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	// Material instance set on the Blueprint, used with the dynamic material instance
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance;

public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	AWeapon* GetEquippedWeapon();//getter

	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }	//给动画蓝图的getter

	FVector GetHitTarget() const;

	//用于FOV的相机getter
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	
	FORCEINLINE bool IsElimmed() const { return bElimmed; }
};
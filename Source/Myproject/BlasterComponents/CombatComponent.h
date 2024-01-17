// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Myproject/HUD/BlasterHUD.h"
#include "Myproject/Weapon/WeaponTypes.h"
#include "Myproject/BlasterTypes/CombatState.h"
#include "CombatComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MYPROJECT_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCombatComponent();
	
	friend class ABlasterCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void EquipWeapon(class AWeapon* WeaponToEquip);

	void Reload();

	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	//�������
	void FireButtonPressed(bool bPressed);

	//����ǹ��������(��������ͼ�е���)
	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();

	void JumpToShotgunEnd();

	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished();

	//�������׵�ʱ������ģ��
	UFUNCTION(BlueprintCallable)
	void LaunchGrenade();

	UFUNCTION(Server, Reliable)
	void ServerLaunchGrenade(const FVector_NetQuantize& Target);
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	void SetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);


	UFUNCTION()
	void OnRep_EquippedWeapon();

	void Fire();

	UFUNCTION(Server, Reliable)//��������fire
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(NetMulticast, Reliable)//Multicast RPC for fire
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	//shooting line trace
	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	void HandleReload();

	int32 AmountToReload();


	void ThrowGrenade();

	// �ڷ������˴���Ͷ�����񵯵ĺ���
	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();

	//�����ڱ༭����ѡ��һ�� AProjectile ���������࣬�����丳ֵ�� GrenadeClass 
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> GrenadeClass;

	// ���µ�ǰװ��������
	void DropEquippedWeapon();

	// ��Actor���ӵ�����
	void AttachActorToRightHand(AActor* ActorToAttach);

	// ��Actor���ӵ�����
	void AttachActorToLeftHand(AActor* ActorToAttach);

	// ����Я���ĵ�ҩ����
	void UpdateCarriedAmmo();

	// ����װ����������Ч
	void PlayEquipWeaponSound();

	// ����װ��յ�����
	void ReloadEmptyWeapon();

	void ShowAttachedGrenade(bool bShowGrenade);

private:
	UPROPERTY()
	class ABlasterCharacter* Character;

	UPROPERTY()
	class ABlasterPlayerController* Controller;

	UPROPERTY()
	class ABlasterHUD* HUD;
	
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	UPROPERTY(Replicated)
	bool bAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	bool bFireButtonPressed;//�ж��Ƿ��¿���

		/**
	* HUD and crosshairs 
	*/

	//׼��Ӱ�����
	float CrosshairVelocityFactor;//�����ٶȵ���ɢ����
	
	float CrosshairInAirFactor;//�����Ƿ��ڿ��е���ɢ����

	float CrosshairAimFactor;//�����Ƿ���׼�ı��׼����ɢ����
	
	float CrosshairShootingFactor;//���ʱӰ��׼�ĵĲ���

	FVector HitTarget;

	FHUDPackage HUDPackage;

	/**
* Aiming and FOV
*/

// Field of view when not aiming; set to the camera's base FOV in BeginPlay
	float DefaultFOV;

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomedFOV = 30.f;

	float CurrentFOV;

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomInterpSpeed = 20.f;

	void InterpFOV(float DeltaTime);//���ڲ�ֵ 

	/**
	* Automatic fire
	*/

	FTimerHandle FireTimer;
	bool bCanFire = true;

	void StartFireTimer();
	void FireTimerFinished();

	bool CanFire();

	// Carried ammo for the currently-equipped weapon
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	//�ͻ��˸���notify������ֻ��һ��SetHUDCarriedAmmo(CarriedAmmo)
	UFUNCTION()
	void OnRep_CarriedAmmo();

	//�洢ÿ����ҩ�����ݽṹ
	TMap<EWeaponType, int32> CarriedAmmoMap;

	/*
	��ʼ��CarriedAmmo
	*/

	// ��ʼ��ҩ����ͻ����ǹ
	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 45;

	// ��ʼ��ҩ�������������
	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 0;

	// ��ʼ��ҩ������ǹ
	UPROPERTY(EditAnywhere)
	int32 StartingPistolAmmo = 0;

	// ��ʼ��ҩ�������ǹ
	UPROPERTY(EditAnywhere)
	int32 StartingSMGAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingShotgunAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingSniperAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingGrenadeLauncherAmmo = 0;

	void InitializeCarriedAmmo();

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	void UpdateAmmoValues();

	void UpdateShotgunAmmoValues();

	//���׳�ʼ��
	UPROPERTY(ReplicatedUsing = OnRep_Grenades)
	int32 Grenades = 4;

	UFUNCTION()
	void OnRep_Grenades();

	//���������
	UPROPERTY(EditAnywhere)
	int32 MaxGrenades = 4;

	void UpdateHUDGrenades();

	UPROPERTY(EditAnywhere)
	double GrenadeThrowSpawnAdjustment = 1;


public:	
	FORCEINLINE int32 GetGrenades() const { return Grenades; }
};

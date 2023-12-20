// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Myproject/HUD/BlasterHUD.h"
#include "Myproject/Weapon/WeaponTypes.h"
#include "CombatComponent.generated.h"

#define TRACE_LENGTH 80000.f

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
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	void SetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);


	UFUNCTION()
	void OnRep_EquippedWeapon();

	void FireButtonPressed(bool bPressed);//�������

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
	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 45;

	void InitializeCarriedAmmo();

public:	
	 
		
};

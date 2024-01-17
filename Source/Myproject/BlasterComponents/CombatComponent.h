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

	//左键开火
	void FireButtonPressed(bool bPressed);

	//霰弹枪换弹函数(用于在蓝图中调用)
	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();

	void JumpToShotgunEnd();

	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished();

	//丢出手雷的时候隐藏模型
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

	UFUNCTION(Server, Reliable)//服务器的fire
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

	// 在服务器端触发投掷手榴弹的函数
	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();

	//允许在编辑器中选择一个 AProjectile 或其派生类，并将其赋值给 GrenadeClass 
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> GrenadeClass;

	// 丢下当前装备的武器
	void DropEquippedWeapon();

	// 将Actor附加到右手
	void AttachActorToRightHand(AActor* ActorToAttach);

	// 将Actor附加到左手
	void AttachActorToLeftHand(AActor* ActorToAttach);

	// 更新携带的弹药数量
	void UpdateCarriedAmmo();

	// 播放装备武器的音效
	void PlayEquipWeaponSound();

	// 重新装填空的武器
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

	bool bFireButtonPressed;//判断是否按下开火

		/**
	* HUD and crosshairs 
	*/

	//准心影响参数
	float CrosshairVelocityFactor;//根据速度的扩散参数
	
	float CrosshairInAirFactor;//根据是否在空中的扩散参数

	float CrosshairAimFactor;//根据是否瞄准改变的准心扩散参数
	
	float CrosshairShootingFactor;//射击时影响准心的参数

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

	void InterpFOV(float DeltaTime);//用于插值 

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

	//客户端复制notify，里面只有一个SetHUDCarriedAmmo(CarriedAmmo)
	UFUNCTION()
	void OnRep_CarriedAmmo();

	//存储每个弹药的数据结构
	TMap<EWeaponType, int32> CarriedAmmoMap;

	/*
	初始化CarriedAmmo
	*/

	// 初始弹药量：突击步枪
	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 45;

	// 初始弹药量：火箭发射器
	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 0;

	// 初始弹药量：手枪
	UPROPERTY(EditAnywhere)
	int32 StartingPistolAmmo = 0;

	// 初始弹药量：冲锋枪
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

	//手雷初始量
	UPROPERTY(ReplicatedUsing = OnRep_Grenades)
	int32 Grenades = 4;

	UFUNCTION()
	void OnRep_Grenades();

	//手雷最大量
	UPROPERTY(EditAnywhere)
	int32 MaxGrenades = 4;

	void UpdateHUDGrenades();

	UPROPERTY(EditAnywhere)
	double GrenadeThrowSpawnAdjustment = 1;


public:	
	FORCEINLINE int32 GetGrenades() const { return Grenades; }
};

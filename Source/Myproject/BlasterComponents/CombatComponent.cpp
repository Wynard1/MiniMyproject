// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Myproject/Weapon/Weapon.h"
#include "Myproject/Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Myproject/PlayerController/BlasterPlayerController.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"
#include "Myproject/Character/BlasterAnimInstance.h"
#include "Myproject/Weapon/Projectile.h"


// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);

	//携带弹药只会复制到拥有它的客户端，这将节省一些带宽
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);

	DOREPLIFETIME(UCombatComponent, CombatState);
}

void UCombatComponent::ShotgunShellReload()
{
	if (Character && Character->HasAuthority())
	{
		// 调用更新霰弹枪弹药值的函数
		UpdateShotgunAmmoValues();
	}

}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		//移速相关
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		//FOV的相机获取
		
		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;//FieldOfView：相机组件自带的变量，需包含头文件
			CurrentFOV = DefaultFOV;//初始化
		}
		
		//初始化携带弹药(服务器)
		if (Character->HasAuthority())
		{
			InitializeCarriedAmmo();
		}
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//每帧设置准心。后续有插值的可能所以传入DeltaTime
	//SetHUDCrosshairs(DeltaTime);

	
	if (Character && Character->IsLocallyControlled())
	{
		//每帧设置准心指向的目标位置
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;
		
		SetHUDCrosshairs(DeltaTime);//每帧设置准心。后续有插值的可能所以传入DeltaTime
		InterpFOV(DeltaTime);//每帧设置并调整FOV
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)//左键开火_服务器
{
	bFireButtonPressed = bPressed;//locally  check
	//有没有按键在本地可以检测，但是真正重要的开火要在服务器上实现

	if (bFireButtonPressed)//按下按钮的时候，调用函数本身
	{
		bFireButtonPressed = bPressed;
		//按下左键开火的时候修改准心开火参数
		if (EquippedWeapon)
		{
			Fire();
		}
	}
}

void UCombatComponent::Fire()
{
	if (CanFire() && EquippedWeapon)
	{
		bCanFire = false;

		//将上面的命中结果传到ServerFire
		ServerFire(HitTarget);

		//按下左键开火的时候修改准心开火参数
		if (EquippedWeapon)
		{
			CrosshairShootingFactor = .75f;
		}
		StartFireTimer();
	}
}

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || Character == nullptr) return;
	Character->GetWorldTimerManager().SetTimer(
		FireTimer,
		this,
		&UCombatComponent::FireTimerFinished,
		EquippedWeapon->FireDelay
	);
}

void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr) return;
	bCanFire = true;
	if (bFireButtonPressed && EquippedWeapon->bAutomatic)
	{
		Fire();
	}
	
	//FIRE弹夹打空了自动换弹
	ReloadEmptyWeapon();
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)		//左键开火_RPC
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)	//左键开火_MulticastRPC
{

	if (EquippedWeapon == nullptr) return;
	
	/*
	散弹枪特例、可以在换弹时射击
	*/
	if (Character && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
	{
		// 根据bAiming播放开火蒙太奇
		Character->PlayFireMontage(bAiming);

		// 执行开火功能
		EquippedWeapon->Fire(TraceHitTarget);

		// 射击完成后设置战斗状态为未占用
		CombatState = ECombatState::ECS_Unoccupied;
		return;
	}

	//如果在瞄准，就播放FireMontage
	if (Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		//角色开枪蒙太奇
		Character->PlayFireMontage(bAiming);

		//枪械模型的开火效果，不是整个fire过程
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;

	if (CombatState != ECombatState::ECS_Unoccupied) return;

	DropEquippedWeapon();

	//先设置WeaponState，在函数里取消模拟物理
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	
	AttachActorToRightHand(EquippedWeapon);
		
	EquippedWeapon->SetOwner(Character);
	//设置owner后给装备的武器初始化弹药
	EquippedWeapon->SetHUDAmmo();

	UpdateCarriedAmmo();
	PlayEquipWeaponSound();
	ReloadEmptyWeapon();

	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::DropEquippedWeapon()
{
	//已经持有武器的话，再装备武器就会丢弃原有的武器，之后装备捡起的武器
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}
}

void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr) return;

	//没有了模拟物理，才能保证attach to socket能执行
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr || EquippedWeapon == nullptr) return;
	
	// 判断是否使用手枪/SMG插槽
	bool bUsePistolSocket =
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol ||
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SubmachineGun;
	FName SocketName = bUsePistolSocket ? FName("PistolSocket") : FName("LeftHandSocket");

	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(SocketName);
	if (HandSocket)
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::UpdateCarriedAmmo()
{
	if (EquippedWeapon == nullptr) return;
	/*
	装备武器的时候初始化carried ammo
	*/
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))	//检查这个武器是否在Tmap里有
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	/*
	END OF 装备武器的时候初始化carried ammo
	*/
}
void UCombatComponent::PlayEquipWeaponSound()
{
	//捡起武器时播放音效 
	if (Character && EquippedWeapon && EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			EquippedWeapon->EquipSound,
			Character->GetActorLocation()
		);
	}
}
void UCombatComponent::ReloadEmptyWeapon()
{
	//捡起武器的时候空弹夹自动换弹
	if (EquippedWeapon && EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && CombatState == ECombatState::ECS_Unoccupied && EquippedWeapon && !EquippedWeapon->IsFull())
	{
		ServerReload();
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	CombatState = ECombatState::ECS_Reloading;		//一旦在这里设置好，它就会复制给所有客户，OnrepNotify也会被调用。
	HandleReload();	//通用换弹函数
}

void UCombatComponent::FinishReloading()
{
	if (Character == nullptr) return;
	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;

		//完成换弹时更新主弹药和备弹药
		UpdateAmmoValues();
	}
	
	//瞄准结束后依旧按着左键，那就直接开火
	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::UpdateAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	
	//调用备弹计算函数
	int32 ReloadAmount = AmountToReload();

	//携带弹药量(备弹)更新
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	//更新备弹HUD
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}

	//增加当前使用弹夹的弹药
	EquippedWeapon->AddAmmo(-ReloadAmount);
}

void UCombatComponent::UpdateShotgunAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr)
	{
		return;
	}

	// 弹药值更新
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		// 从携带的弹药映射中减去一发子弹
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
		
		// 更新携带弹药的值
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	// HUD更新
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		// 更新HUD中显示的携带弹药数量
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}

	// 弹药值更新
	EquippedWeapon->AddAmmo(-1);

	// 设置可以射击的标志为真
	bCanFire = true;

	// 如果武器已满或者携带的弹药为零，则跳转到霰弹枪结束动画(服务器Only)
	if (EquippedWeapon->IsFull() || CarriedAmmo == 0)
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::JumpToShotgunEnd()
{
	// Jump to ShotgunEnd section
	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (AnimInstance && Character->GetReloadMontage())
	{
		AnimInstance->Montage_JumpToSection(FName("ShotgunEnd"));
	}
}

//丢雷动画结束，调用此函数改变ECombatState
void UCombatComponent::ThrowGrenadeFinished()
{
	CombatState = ECombatState::ECS_Unoccupied;

	//丢雷结束插槽改回右手
	AttachActorToRightHand(EquippedWeapon);
}

void UCombatComponent::LaunchGrenade()
{
    // 隐藏手上的手榴弹模型
    ShowAttachedGrenade(false);

	if (Character && Character->IsLocallyControlled())
	{
		ServerLaunchGrenade(HitTarget);
	}
}

void UCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target)
{
	if (Character && GrenadeClass && Character->GetAttachedGrenade())
    {
        // 获取手上榴弹的位置作为生成榴弹的起始位置
        const FVector StartingLocation = Character->GetAttachedGrenade()->GetComponentLocation();
        
        // 计算榴弹飞向目标的向量
		FVector ToTarget = Target - StartingLocation;

		// Shift the grenade spawn so it does not collide with owner collision
		FVector ToTargetNormalized = (Target - StartingLocation).GetSafeNormal();
		FVector ShiftedStartingLocation = StartingLocation + (ToTargetNormalized * GrenadeThrowSpawnAdjustment);

        // 准备生成榴弹的参数
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = Character;
        SpawnParams.Instigator = Character;
        UWorld* World = GetWorld();
        
		if (World)
        {
            // 生成榴弹
            World->SpawnActor<AProjectile>(
                GrenadeClass,           // 榴弹类
                StartingLocation,       // 起始位置
                ToTarget.Rotation(),    // 飞向目标的旋转
                SpawnParams             // 生成参数
            );
        }
    }
}


void UCombatComponent::OnRep_CombatState()	//客户端换弹处理：只在CombatState改变的时候执行
{
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		HandleReload();
		break;

	case ECombatState::ECS_Unoccupied:
		if (bFireButtonPressed)
		{
			Fire();
		}
		break;
	
	//丢雷
	case ECombatState::ECS_ThrowingGrenade:
		//非初始客户端才播放丢雷动画
		if (Character && !Character->IsLocallyControlled())
		{
			//播放丢雷动画
			Character->PlayThrowGrenadeMontage();

			//插槽改到左手
			AttachActorToLeftHand(EquippedWeapon);

			//雷模型显示
			ShowAttachedGrenade(true);
		}
		break;
	}
}

void UCombatComponent::HandleReload()
{

	Character->PlayReloadMontage();
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		int32 Least = FMath::Min(RoomInMag, AmountCarried);
		return FMath::Clamp(RoomInMag, 0, Least);
	}
	return 0;
}

void UCombatComponent::ThrowGrenade()
{
	//禁止未播完动画立刻重复丢手雷
	if (CombatState != ECombatState::ECS_Unoccupied || EquippedWeapon == nullptr) return;
	
	CombatState = ECombatState::ECS_ThrowingGrenade;

	//本地播放丢雷动画
	if (Character)
	{
		//播放丢雷动画
		Character->PlayThrowGrenadeMontage();

		//插槽改到左手
		AttachActorToLeftHand(EquippedWeapon);

		ShowAttachedGrenade(true);
	}

	//如果本地是客户端，则进入服务端
	if (Character && !Character->HasAuthority())
	{
		ServerThrowGrenade();
	}
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
	//改变ECombatState从而调用OnrepNotify，从而让其他客户端播放丢雷动画
	CombatState = ECombatState::ECS_ThrowingGrenade;
	
	//服务端自己播放丢雷动画
	if (Character)
	{
		//播放丢雷动画
		Character->PlayThrowGrenadeMontage();

		//插槽改到左手
		AttachActorToLeftHand(EquippedWeapon);

		ShowAttachedGrenade(true);
	}
}

//根据Bool决定是否显示Grenade
void UCombatComponent::ShowAttachedGrenade(bool bShowGrenade)
{
	if (Character && Character->GetAttachedGrenade())
	{
		Character->GetAttachedGrenade()->SetVisibility(bShowGrenade);
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		//先设置WeaponState，在函数里取消模拟物理
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

		AttachActorToRightHand(EquippedWeapon);

		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;

		//捡起武器时播放音效
		PlayEquipWeaponSound();
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	//获取屏幕信息
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)//视窗来自引擎
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}


	//CrosshairLocation 赋值为屏幕中心
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);

	//坐标转换
	FVector CrosshairWorldPosition;//存储结果
	FVector CrosshairWorldDirection;//存储结果
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(//返回投射是否成功
		//四个参数
		UGameplayStatics::GetPlayerController(this, 0),//this:角色所在世界；0：玩家0(尽管这是针对每台机器的多人游戏，但Player0才是真正的"玩家")
		CrosshairLocation,//准心屏幕位置
		CrosshairWorldPosition, //被填充 存储函数执行输出的准心世界坐标
		CrosshairWorldDirection //被填充  存储函数执行输出的准心世界指向
	);

	if (bScreenToWorld)//line trace
	{
		//初始位置
		FVector Start = CrosshairWorldPosition;

		//把起始位置往前推，到角色前面一点
		if (Character)
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();//相机和角色之间的距离“大小”
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);//单位长度*大小，再多推一点点
			//DrawDebugSphere(GetWorld(), Start, 16.f, 12, FColor::Red, false);
		}

		//结束位置
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;//CrosshairWorldDirection长度是单位坐标，需要扩大

		//执行line trace
		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,//被填充
			Start,
			End,
			ECollisionChannel::ECC_Visibility	//检测碰撞类型
		);

		//只有实现了接口的类(角色类)才是我们要检测的
		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
		{
			HUDPackage.CrosshairsColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairsColor = FLinearColor::White;
		}

		//DEBUG Sphere

		/*
		if (!TraceHitResult.bBlockingHit)//未命中
		{
			TraceHitResult.ImpactPoint = End;
			HitTarget = End;
		}

		else //命中，用DrawDebugSphere输出目标
		{
			HitTarget = TraceHitResult.ImpactPoint;
			DrawDebugSphere(
				GetWorld(),
				TraceHitResult.ImpactPoint,//center of DrawDebugSphere
				12.f,//radius
				12,//segment
				FColor::Red
			);
		}*/
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (Character == nullptr || Character->Controller == nullptr) return;

	//转换_如果Controller为空，就执行cast并获取；不为空那就是它自身。
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		// set the HUD——转换_如果HUD为空，就执行cast并获取；不为空那就是它自身。
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD; 
		
		if (HUD)
		{
			//准心获取
			//FHUDPackage HUDPackage;

			//准心获取
			if (EquippedWeapon)
			{
				//获取
				HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
			}
			else
			{
				//没有武器就不显示
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
			}

			// Calculate crosshair spread

			//判断条件1:移动速度
			// map[0, MaxWalkSpeed:600] -> [0, 1]
			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0.f, 1.f);


			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;

			//GetMappedRangeValueClamped:返回Mapping后的值
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

			//让其他东西影响我们的准星分布，而不仅仅是速度——是否在空中
			//判断条件2:是否在空中
			if (Character->GetCharacterMovement()->IsFalling())
			{
				//在空中，和2.25做插值，速度为2.25
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				//在地面，和0做插值，速度为30
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}

			//添加扩散值
			//HUDPackage.CrosshairSpread = CrosshairVelocityFactor + CrosshairInAirFactor;

			//判断条件3  是否正在瞄准
			//根据是否瞄准改变准心扩散参数
			if (bAiming)//瞄准 -0.58(缩小)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
			}
			else//停止瞄准 回到0
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}

			//无时不刻把射击时的准心参数跟0做插值
			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 40.f);

			//准心扩散计算
			HUDPackage.CrosshairSpread =
				0.5f +	//准心基本大小。以防准心所在一坨
				CrosshairVelocityFactor +	//人物移动参数
				CrosshairInAirFactor -		//是否在空中
				CrosshairAimFactor +		//是否瞄准中
				CrosshairShootingFactor;	//是否射击

			//准心设置
			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;

	//瞄准时的FOV变换计算
	if (bAiming)
	{
		//瞄准的时候，将视野做插值
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		//为瞄准，跟默认视野做插值
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}

	//将计算的FOV同步给相机
	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);//SetFieldOfView：相机组件自带
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}

	//装备狙击枪的时候，打开瞄准镜
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		Character->ShowSniperScopeWidget(bIsAiming);
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr) return false;

	//霰弹枪特例
	if (!EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun) return true;

	//持有武器 且 达到开火最快间隔时间 且 状态是无占用(不在换弹期间)
	//(只要有弹药，可以通过重新上膛来打断武器射击。)
	return !EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}

	//霰弹枪专用、没备弹了提前结束动作
	bool bJumpToShotgunEnd =
		CombatState == ECombatState::ECS_Reloading &&
		EquippedWeapon != nullptr &&
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun &&
		CarriedAmmo == 0;	
	if (bJumpToShotgunEnd)
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::InitializeCarriedAmmo()
{
	// 初始化各种武器的携带弹药量
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo); // 突击步枪
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo); // 火箭发射器
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo); // 手枪
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SubmachineGun, StartingSMGAmmo); // 冲锋枪
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingShotgunAmmo);	
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartingShotgunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, StartingGrenadeLauncherAmmo);
}
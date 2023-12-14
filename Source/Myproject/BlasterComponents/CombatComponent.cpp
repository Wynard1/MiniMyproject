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
	if (bCanFire && EquippedWeapon)
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
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)		//左键开火_RPC
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)	//左键开火_MulticastRPC
{

	if (EquippedWeapon == nullptr) return;

	//如果在瞄准，就播放FireMontage
	if (Character)
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

	//先设置WeaponState，在函数里取消模拟物理
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	//没有了模拟物理，才能保证attach to socket能执行
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}
	EquippedWeapon->SetOwner(Character);
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		//先设置WeaponState，在函数里取消模拟物理
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

		//没有了模拟物理，才能保证attach to socket能执行
		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (HandSocket)
		{
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		}

		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
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
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
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


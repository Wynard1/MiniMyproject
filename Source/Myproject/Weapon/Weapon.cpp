// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Myproject/Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Casing.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Myproject/PlayerController/BlasterPlayerController.h"
#include "Myproject/BlasterComponents/CombatComponent.h"

// Sets default values
AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(RootComponent);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
	}
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(false);
	}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME(AWeapon, Ammo);
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		BlasterCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		BlasterCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::SetHUDAmmo()
{
	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if (BlasterOwnerCharacter)
	{
		BlasterOwnerController = BlasterOwnerController == nullptr ? Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller) : BlasterOwnerController;
		if (BlasterOwnerController)
		{
			BlasterOwnerController->SetHUDWeaponAmmo(Ammo);
		}
	}
}

void AWeapon::SpendRound()
{
	//将Ammo-1但小于0的时候clamp为0
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	
	SetHUDAmmo();
}

void AWeapon::OnRep_Ammo()
{
	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	
	//霰弹枪用、弹药装满提前结束换弹动作
	if (BlasterOwnerCharacter && BlasterOwnerCharacter->GetCombat() && IsFull())
	{
		BlasterOwnerCharacter->GetCombat()->JumpToShotgunEnd();
	}

	SetHUDAmmo();
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	if (Owner == nullptr)
	{
		BlasterOwnerCharacter = nullptr;
		BlasterOwnerController = nullptr;
	}
	else
	{
		//owner更新且存在的时候初始化弹药
		SetHUDAmmo();
	}
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (WeaponType == EWeaponType::EWT_SubmachineGun)
		{
			// 如果装备的是冲锋枪(Submachine Gun)
			// 启用查询和物理碰撞，使得武器可以与世界发生物理交互并检测碰撞
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

			// 启用重力，使得武器在世界中受到重力影响，类似真实物体的下落
			WeaponMesh->SetEnableGravity(true);

			// 将武器的碰撞响应设置为忽略所有通道，使其不会与其他对象产生碰撞
			// 通常用于装备在角色身上时，避免与角色及其他物体发生碰撞
			WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		}

		break;
	
	case EWeaponState::EWS_Dropped:
		//现在如果我们掉落武器，我们需要模拟物理并为武器设置碰撞
		if (HasAuthority())
		{
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}

		// 启用模拟物理，使武器在世界中表现出物理属性，如重力和碰撞
		WeaponMesh->SetSimulatePhysics(true);

		// 启用重力，使武器在世界中受到重力影响，类似真实物体的下落
		WeaponMesh->SetEnableGravity(true);

		// 设置武器碰撞为查询和物理，允许与世界进行物理交互和碰撞检测
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		// 设置武器对所有通道的碰撞响应为阻挡（Block），使其与其他对象产生碰撞
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);

		// 将武器对于Pawn通道的碰撞响应设置为忽略（Ignore），避免与角色产生碰撞
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);

		// 将武器对于Camera通道的碰撞响应设置为忽略（Ignore），避免与相机产生碰撞
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

		break;
	}
}

void AWeapon::OnRep_WeaponState()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);

		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		//如果装备的是SMG 
		if (WeaponType == EWeaponType::EWT_SubmachineGun)
		{
			// 如果装备的是冲锋枪(Submachine Gun)
			// 启用查询和物理碰撞，使得武器可以与世界发生物理交互并检测碰撞
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

			// 启用重力，使得武器在世界中受到重力影响，类似真实物体的下落
			WeaponMesh->SetEnableGravity(true);

			// 将武器的碰撞响应设置为忽略所有通道，使其不会与其他对象产生碰撞
			// 通常用于装备在角色身上时，避免与角色及其他物体发生碰撞
			WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		}

		break;

	//现在如果我们掉落武器，我们需要模拟物理并为武器设置碰撞
	case EWeaponState::EWS_Dropped:

		// 启用模拟物理，使武器在世界中表现出物理属性，如重力和碰撞
		WeaponMesh->SetSimulatePhysics(true);

		// 启用重力，使武器在世界中受到重力影响，类似真实物体的下落
		WeaponMesh->SetEnableGravity(true);

		// 设置武器碰撞为查询和物理，允许与世界进行物理交互和碰撞检测
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		// 设置武器对所有通道的碰撞响应为阻挡（Block），使其与其他对象产生碰撞
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);

		// 将武器对于Pawn通道的碰撞响应设置为忽略（Ignore），避免与角色产生碰撞
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);

		// 将武器对于Camera通道的碰撞响应设置为忽略（Ignore），避免与相机产生碰撞
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

		break;
	}
}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);
	}
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation)//if it is valid, then we can play an animation using our skeletal mesh.
	{
		//fales for loop
		WeaponMesh->PlayAnimation(FireAnimation, false);//In order to useWeaponMesh,need to include skeletal mesh component.
	}
	
	//射击后弹出弹壳儿
	if (CasingClass)
	{
		//获取socket。因为这是父类，所以所有武器到时候都要设置成一样的socket name
		const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(FName("AmmoEject"));
		if (AmmoEjectSocket)
		{
			//获取插槽坐标信息
			FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);

			UWorld* World = GetWorld();
			if (World)
			{
				World->SpawnActor<ACasing>(
					CasingClass,
					SocketTransform.GetLocation(),
					SocketTransform.GetRotation().Rotator()
					);
			}
		}
	}

	//更新子弹
	SpendRound();
}

void AWeapon::Dropped()
{
	//修改武器状态
	SetWeaponState(EWeaponState::EWS_Dropped);

	//初始化使用DetachFromComponent的参数(固定)
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	
	//使用DetachFromComponent
	WeaponMesh->DetachFromComponent(DetachRules);
	SetOwner(nullptr);

	BlasterOwnerCharacter = nullptr;
	BlasterOwnerController = nullptr;
}

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo - AmmoToAdd, 0, MagCapacity);
	SetHUDAmmo();
}

bool AWeapon::IsEmpty()
{
	return Ammo <= 0;
}

bool AWeapon::IsFull()
{
	return Ammo == MagCapacity;
}
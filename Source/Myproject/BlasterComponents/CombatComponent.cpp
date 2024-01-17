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

	//Я����ҩֻ�Ḵ�Ƶ�ӵ�����Ŀͻ��ˣ��⽫��ʡһЩ����
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);

	DOREPLIFETIME(UCombatComponent, CombatState);
}

void UCombatComponent::ShotgunShellReload()
{
	if (Character && Character->HasAuthority())
	{
		// ���ø�������ǹ��ҩֵ�ĺ���
		UpdateShotgunAmmoValues();
	}

}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		//�������
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		//FOV�������ȡ
		
		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;//FieldOfView���������Դ��ı����������ͷ�ļ�
			CurrentFOV = DefaultFOV;//��ʼ��
		}
		
		//��ʼ��Я����ҩ(������)
		if (Character->HasAuthority())
		{
			InitializeCarriedAmmo();
		}
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//ÿ֡����׼�ġ������в�ֵ�Ŀ������Դ���DeltaTime
	//SetHUDCrosshairs(DeltaTime);

	
	if (Character && Character->IsLocallyControlled())
	{
		//ÿ֡����׼��ָ���Ŀ��λ��
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;
		
		SetHUDCrosshairs(DeltaTime);//ÿ֡����׼�ġ������в�ֵ�Ŀ������Դ���DeltaTime
		InterpFOV(DeltaTime);//ÿ֡���ò�����FOV
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)//�������_������
{
	bFireButtonPressed = bPressed;//locally  check
	//��û�а����ڱ��ؿ��Լ�⣬����������Ҫ�Ŀ���Ҫ�ڷ�������ʵ��

	if (bFireButtonPressed)//���°�ť��ʱ�򣬵��ú�������
	{
		bFireButtonPressed = bPressed;
		//������������ʱ���޸�׼�Ŀ������
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

		//����������н������ServerFire
		ServerFire(HitTarget);

		//������������ʱ���޸�׼�Ŀ������
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
	
	//FIRE���д�����Զ�����
	ReloadEmptyWeapon();
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)		//�������_RPC
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)	//�������_MulticastRPC
{

	if (EquippedWeapon == nullptr) return;
	
	/*
	ɢ��ǹ�����������ڻ���ʱ���
	*/
	if (Character && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
	{
		// ����bAiming���ſ�����̫��
		Character->PlayFireMontage(bAiming);

		// ִ�п�����
		EquippedWeapon->Fire(TraceHitTarget);

		// �����ɺ�����ս��״̬Ϊδռ��
		CombatState = ECombatState::ECS_Unoccupied;
		return;
	}

	//�������׼���Ͳ���FireMontage
	if (Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		//��ɫ��ǹ��̫��
		Character->PlayFireMontage(bAiming);

		//ǹеģ�͵Ŀ���Ч������������fire����
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;

	if (CombatState != ECombatState::ECS_Unoccupied) return;

	DropEquippedWeapon();

	//������WeaponState���ں�����ȡ��ģ������
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	
	AttachActorToRightHand(EquippedWeapon);
		
	EquippedWeapon->SetOwner(Character);
	//����owner���װ����������ʼ����ҩ
	EquippedWeapon->SetHUDAmmo();

	UpdateCarriedAmmo();
	PlayEquipWeaponSound();
	ReloadEmptyWeapon();

	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::DropEquippedWeapon()
{
	//�Ѿ����������Ļ�����װ�������ͻᶪ��ԭ�е�������֮��װ�����������
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}
}

void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr) return;

	//û����ģ���������ܱ�֤attach to socket��ִ��
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr || EquippedWeapon == nullptr) return;
	
	// �ж��Ƿ�ʹ����ǹ/SMG���
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
	װ��������ʱ���ʼ��carried ammo
	*/
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))	//�����������Ƿ���Tmap����
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	/*
	END OF װ��������ʱ���ʼ��carried ammo
	*/
}
void UCombatComponent::PlayEquipWeaponSound()
{
	//��������ʱ������Ч 
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
	//����������ʱ��յ����Զ�����
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
	CombatState = ECombatState::ECS_Reloading;		//һ�����������úã����ͻḴ�Ƹ����пͻ���OnrepNotifyҲ�ᱻ���á�
	HandleReload();	//ͨ�û�������
}

void UCombatComponent::FinishReloading()
{
	if (Character == nullptr) return;
	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;

		//��ɻ���ʱ��������ҩ�ͱ���ҩ
		UpdateAmmoValues();
	}
	
	//��׼���������ɰ���������Ǿ�ֱ�ӿ���
	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::UpdateAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	
	//���ñ������㺯��
	int32 ReloadAmount = AmountToReload();

	//Я����ҩ��(����)����
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	//���±���HUD
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}

	//���ӵ�ǰʹ�õ��еĵ�ҩ
	EquippedWeapon->AddAmmo(-ReloadAmount);
}

void UCombatComponent::UpdateShotgunAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr)
	{
		return;
	}

	// ��ҩֵ����
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		// ��Я���ĵ�ҩӳ���м�ȥһ���ӵ�
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
		
		// ����Я����ҩ��ֵ
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	// HUD����
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		// ����HUD����ʾ��Я����ҩ����
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}

	// ��ҩֵ����
	EquippedWeapon->AddAmmo(-1);

	// ���ÿ�������ı�־Ϊ��
	bCanFire = true;

	// ���������������Я���ĵ�ҩΪ�㣬����ת������ǹ��������(������Only)
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

//���׶������������ô˺����ı�ECombatState
void UCombatComponent::ThrowGrenadeFinished()
{
	CombatState = ECombatState::ECS_Unoccupied;

	//���׽�����۸Ļ�����
	AttachActorToRightHand(EquippedWeapon);
}

void UCombatComponent::LaunchGrenade()
{
    // �������ϵ�����ģ��
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
        // ��ȡ�����񵯵�λ����Ϊ�����񵯵���ʼλ��
        const FVector StartingLocation = Character->GetAttachedGrenade()->GetComponentLocation();
        
        // �����񵯷���Ŀ�������
		FVector ToTarget = Target - StartingLocation;

		// Shift the grenade spawn so it does not collide with owner collision
		FVector ToTargetNormalized = (Target - StartingLocation).GetSafeNormal();
		FVector ShiftedStartingLocation = StartingLocation + (ToTargetNormalized * GrenadeThrowSpawnAdjustment);

        // ׼�������񵯵Ĳ���
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = Character;
        SpawnParams.Instigator = Character;
        UWorld* World = GetWorld();
        
		if (World)
        {
            // ������
            World->SpawnActor<AProjectile>(
                GrenadeClass,           // ����
                StartingLocation,       // ��ʼλ��
                ToTarget.Rotation(),    // ����Ŀ�����ת
                SpawnParams             // ���ɲ���
            );
        }
    }
}


void UCombatComponent::OnRep_CombatState()	//�ͻ��˻�������ֻ��CombatState�ı��ʱ��ִ��
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
	
	//����
	case ECombatState::ECS_ThrowingGrenade:
		//�ǳ�ʼ�ͻ��˲Ų��Ŷ��׶���
		if (Character && !Character->IsLocallyControlled())
		{
			//���Ŷ��׶���
			Character->PlayThrowGrenadeMontage();

			//��۸ĵ�����
			AttachActorToLeftHand(EquippedWeapon);

			//��ģ����ʾ
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
	//��ֹδ���궯�������ظ�������
	if (CombatState != ECombatState::ECS_Unoccupied || EquippedWeapon == nullptr) return;
	
	CombatState = ECombatState::ECS_ThrowingGrenade;

	//���ز��Ŷ��׶���
	if (Character)
	{
		//���Ŷ��׶���
		Character->PlayThrowGrenadeMontage();

		//��۸ĵ�����
		AttachActorToLeftHand(EquippedWeapon);

		ShowAttachedGrenade(true);
	}

	//��������ǿͻ��ˣ����������
	if (Character && !Character->HasAuthority())
	{
		ServerThrowGrenade();
	}
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
	//�ı�ECombatState�Ӷ�����OnrepNotify���Ӷ��������ͻ��˲��Ŷ��׶���
	CombatState = ECombatState::ECS_ThrowingGrenade;
	
	//������Լ����Ŷ��׶���
	if (Character)
	{
		//���Ŷ��׶���
		Character->PlayThrowGrenadeMontage();

		//��۸ĵ�����
		AttachActorToLeftHand(EquippedWeapon);

		ShowAttachedGrenade(true);
	}
}

//����Bool�����Ƿ���ʾGrenade
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
		//������WeaponState���ں�����ȡ��ģ������
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

		AttachActorToRightHand(EquippedWeapon);

		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;

		//��������ʱ������Ч
		PlayEquipWeaponSound();
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	//��ȡ��Ļ��Ϣ
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)//�Ӵ���������
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}


	//CrosshairLocation ��ֵΪ��Ļ����
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);

	//����ת��
	FVector CrosshairWorldPosition;//�洢���
	FVector CrosshairWorldDirection;//�洢���
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(//����Ͷ���Ƿ�ɹ�
		//�ĸ�����
		UGameplayStatics::GetPlayerController(this, 0),//this:��ɫ�������磻0�����0(�����������ÿ̨�����Ķ�����Ϸ����Player0����������"���")
		CrosshairLocation,//׼����Ļλ��
		CrosshairWorldPosition, //����� �洢����ִ�������׼����������
		CrosshairWorldDirection //�����  �洢����ִ�������׼������ָ��
	);

	if (bScreenToWorld)//line trace
	{
		//��ʼλ��
		FVector Start = CrosshairWorldPosition;

		//����ʼλ����ǰ�ƣ�����ɫǰ��һ��
		if (Character)
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();//����ͽ�ɫ֮��ľ��롰��С��
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);//��λ����*��С���ٶ���һ���
			//DrawDebugSphere(GetWorld(), Start, 16.f, 12, FColor::Red, false);
		}

		//����λ��
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;//CrosshairWorldDirection�����ǵ�λ���꣬��Ҫ����

		//ִ��line trace
		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,//�����
			Start,
			End,
			ECollisionChannel::ECC_Visibility	//�����ײ����
		);

		//ֻ��ʵ���˽ӿڵ���(��ɫ��)��������Ҫ����
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
		if (!TraceHitResult.bBlockingHit)//δ����
		{
			TraceHitResult.ImpactPoint = End;
			HitTarget = End;
		}

		else //���У���DrawDebugSphere���Ŀ��
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

	//ת��_���ControllerΪ�գ���ִ��cast����ȡ����Ϊ���Ǿ���������
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		// set the HUD����ת��_���HUDΪ�գ���ִ��cast����ȡ����Ϊ���Ǿ���������
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD; 
		
		if (HUD)
		{
			//׼�Ļ�ȡ
			//FHUDPackage HUDPackage;

			//׼�Ļ�ȡ
			if (EquippedWeapon)
			{
				//��ȡ
				HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
			}
			else
			{
				//û�������Ͳ���ʾ
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
			}

			// Calculate crosshair spread

			//�ж�����1:�ƶ��ٶ�
			// map[0, MaxWalkSpeed:600] -> [0, 1]
			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0.f, 1.f);


			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;

			//GetMappedRangeValueClamped:����Mapping���ֵ
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

			//����������Ӱ�����ǵ�׼�Ƿֲ��������������ٶȡ����Ƿ��ڿ���
			//�ж�����2:�Ƿ��ڿ���
			if (Character->GetCharacterMovement()->IsFalling())
			{
				//�ڿ��У���2.25����ֵ���ٶ�Ϊ2.25
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				//�ڵ��棬��0����ֵ���ٶ�Ϊ30
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}

			//�����ɢֵ
			//HUDPackage.CrosshairSpread = CrosshairVelocityFactor + CrosshairInAirFactor;

			//�ж�����3  �Ƿ�������׼
			//�����Ƿ���׼�ı�׼����ɢ����
			if (bAiming)//��׼ -0.58(��С)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
			}
			else//ֹͣ��׼ �ص�0
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}

			//��ʱ���̰����ʱ��׼�Ĳ�����0����ֵ
			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 40.f);

			//׼����ɢ����
			HUDPackage.CrosshairSpread =
				0.5f +	//׼�Ļ�����С���Է�׼������һ��
				CrosshairVelocityFactor +	//�����ƶ�����
				CrosshairInAirFactor -		//�Ƿ��ڿ���
				CrosshairAimFactor +		//�Ƿ���׼��
				CrosshairShootingFactor;	//�Ƿ����

			//׼������
			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;

	//��׼ʱ��FOV�任����
	if (bAiming)
	{
		//��׼��ʱ�򣬽���Ұ����ֵ
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		//Ϊ��׼����Ĭ����Ұ����ֵ
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}

	//�������FOVͬ�������
	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);//SetFieldOfView���������Դ�
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

	//װ���ѻ�ǹ��ʱ�򣬴���׼��
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

	//����ǹ����
	if (!EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun) return true;

	//�������� �� �ﵽ���������ʱ�� �� ״̬����ռ��(���ڻ����ڼ�)
	//(ֻҪ�е�ҩ������ͨ������������������������)
	return !EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}

	//����ǹר�á�û��������ǰ��������
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
	// ��ʼ������������Я����ҩ��
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo); // ͻ����ǹ
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo); // ���������
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo); // ��ǹ
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SubmachineGun, StartingSMGAmmo); // ���ǹ
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingShotgunAmmo);	
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartingShotgunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, StartingGrenadeLauncherAmmo);
}
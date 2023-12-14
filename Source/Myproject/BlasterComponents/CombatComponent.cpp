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
		//�������
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		//FOV�������ȡ
		
		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;//FieldOfView���������Դ��ı����������ͷ�ļ�
			CurrentFOV = DefaultFOV;//��ʼ��
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
	if (bCanFire && EquippedWeapon)
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
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)		//�������_RPC
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)	//�������_MulticastRPC
{

	if (EquippedWeapon == nullptr) return;

	//�������׼���Ͳ���FireMontage
	if (Character)
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

	//������WeaponState���ں�����ȡ��ģ������
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	//û����ģ���������ܱ�֤attach to socket��ִ��
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
		//������WeaponState���ں�����ȡ��ģ������
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

		//û����ģ���������ܱ�֤attach to socket��ִ��
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


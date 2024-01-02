// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Myproject/Weapon/Weapon.h"
#include "Myproject/BlasterComponents/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "BlasterAnimInstance.h"
#include "Myproject/Myproject.h"
#include "Myproject/PlayerController/BlasterPlayerController.h"
#include "Myproject/GameMode/BlasterGameMode.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "Myproject/PlayerState/BlasterPlayerState.h"
#include "Myproject/Weapon/WeaponTypes.h"

ABlasterCharacter::ABlasterCharacter()//���캯��
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat1 = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat1->SetIsReplicated(true);

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	
	//����BlasterCharacter����ײ����Ϊ�����Խ���ECC_SkeletalMesh
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore); 

	//������ģ�Ͷ�ECC_Visibility���л�Ӧ(��Ļ׼��linetrace)
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);//ת���ٶ�

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;//Ĭ��ֵ���óɲ���ת��

	//net update frequency
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	//TimelineComponent
	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//����ע��
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);

	DOREPLIFETIME(ABlasterCharacter, bDisableGameplay);
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

void ABlasterCharacter::Elim()	//only on server
{
	//����ʱ��������
	if (Combat1 && Combat1->EquippedWeapon)
	{
		Combat1->EquippedWeapon->Dropped();
	}
	
	// call RPC
	MulticastElim();
	
	//set elim timer
	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&ABlasterCharacter::ElimTimerFinished,
		ElimDelay
	);
}

void ABlasterCharacter::MulticastElim_Implementation()
{
	//����ɱ��ʱ��ҩ��0
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDWeaponAmmo(0);
	}
	
	bElimmed = true;
	PlayElimMontage();

	//����ʱ�滻���ܽ����
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);

		//�ܽ���ʵĳ�ʼֵ
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}

	//����timeline�����ܽ���ʵ�Dissolve����
	StartDissolve();

	// Disable character movement
	//����ɱ���ֹ��ɫWASD�ƶ�&��ֹͨ��ת�ӽǸı��ɫ����
	bDisableGameplay = true;
	GetCharacterMovement()->DisableMovement();
	
	//����ɱ��ֹͣ����
	if (Combat1)
	{
		Combat1->FireButtonPressed(false);
	}
	
	// Disable collision
	//�رս�������ײ&�ر�������ײ
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);	
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);			

	// Spawn elim bot
	//��ɱ��ͷ�ϻ����˵���Ч
	if (ElimBotEffect)
	{
		//��Ч���ڵ㡪��ͷ��200��λ��
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		
		//������Ч
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()		//����ElimBot�ͻ������ǵĽ�ɫһ����ת��
		);
	}

	//��ɱ��ͷ�ϻ����˵���Ч
	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		);
	}
}

void ABlasterCharacter::ElimTimerFinished() //only on server
{
	/*
	respawn
	*/
	ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	if (BlasterGameMode)
	{
		BlasterGameMode->RequestRespawn(this, Controller);
	}
}

void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();

	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}

	//���ڵ�ʱ�������ǲ������ģ�����Ҫ��ȡGetMatchState���������ж�
	ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	bool bMatchNotInProgress = BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress;

	//�վ������Ի�
	if (Combat1 && Combat1->EquippedWeapon && bMatchNotInProgress)
	{
		Combat1->EquippedWeapon->Destroy();
	}
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	//Ѫ������
	UpdateHUDHealth();

	//���˺��ص�����
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
	}
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RotateInPlace(DeltaTime);
	HideCameraIfCharacterClose();

	//ÿ֡ͨ��+0.0ͬ������
	PollInit();
}

void ABlasterCharacter::RotateInPlace(float DeltaTime)
{
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	//�ͻ��˻��߱��أ���ִ������AimOffset
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	//�ǡ��ͻ��˻��߱��ء�������OnRep_ReplicatedMovement��ִ����Щ
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}


}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	//PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump); �����Դ��ģ�������
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ABlasterCharacter::Jump);//������д�󴴽��ģ������

	PlayerInputComponent->BindAxis("MoveForward", this, &ABlasterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABlasterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ABlasterCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ABlasterCharacter::LookUp);

	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ABlasterCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ABlasterCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ABlasterCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ABlasterCharacter::AimButtonReleased);

	//����    
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ABlasterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ABlasterCharacter::FireButtonReleased);

	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ABlasterCharacter::ReloadButtonPressed);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat1)
	{
		Combat1->Character = this;
	}
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	//û���������ܲ���
	if (Combat1 == nullptr || Combat1->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		
		//��������̫�棬��Ҫ��ת����Ӧ����̫�沿�֡�
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");//��̫���������Լ�����������section name
		
		//�������ֲ��Ŷ�������
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayReloadMontage()
{
	//û���������ܲ���
	if (Combat1 == nullptr || Combat1->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		
		//��������̫�棬��Ҫ��ת����Ӧ����̫�沿�֡�
		FName SectionName;

		//ѡ�񶯻�Ҫ���Ķ���
		switch (Combat1->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		}

		//�������ֲ��Ŷ�������
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		//�������ֲ��Ŷ�������
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	//û���������ܲ���
	if (Combat1 == nullptr || Combat1->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage && !AnimInstance->IsAnyMontagePlaying() )
	{
		AnimInstance->Montage_Play(HitReactMontage);

		//��������̫�棬��Ҫ��ת����Ӧ����̫�沿�֡�
		FName SectionName("FromFront");

		//�������ֲ��Ŷ�������
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	UpdateHUDHealth();
	PlayHitReactMontage();

	//����ֵΪ0ʱ��ִ����̭����
	if (Health == 0.f)
	{
		ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
		if (BlasterGameMode)
		{
			//��̭�����Ĳ���׼��
			BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
			ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatorController);
			
			//��̭
			BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
		}
	}
}

void ABlasterCharacter::MoveForward(float Value)
{
	if (bDisableGameplay) return;
	
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::MoveRight(float Value)
{
	if (bDisableGameplay) return;
	
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ABlasterCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ABlasterCharacter::EquipButtonPressed()
{
	if (bDisableGameplay) return;
	
	if (Combat1)
	{
		if (HasAuthority())
		{
			Combat1->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerEquipButtonPressed();
		}
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat1)
	{
		Combat1->EquipWeapon(OverlappingWeapon);
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (bDisableGameplay) return;

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}


void ABlasterCharacter::ReloadButtonPressed()
{
	if (bDisableGameplay) return;
	
	if (Combat1)
	{
		Combat1->Reload();
	}
}

void ABlasterCharacter::AimButtonPressed()
{
	if (bDisableGameplay) return;
	
	if (Combat1)
	{
		Combat1->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if (bDisableGameplay) return;
	
	if (Combat1)
	{
		Combat1->SetAiming(false);
	}
}

float ABlasterCharacter::CalculateSpeed()
{
	//if (Combat1 && Combat1->EquippedWeapon == nullptr) return;//�������ǻ�����
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	//float Speed = Velocity.Size();
	return Velocity.Size();
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if (Combat1 && Combat1->EquippedWeapon == nullptr) return;
	float Speed = CalculateSpeed();

	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bIsInAir) // ��ɫ�������Ҳ��ڿ���(��ʱ��Ż���AO)
	{
		//�����Ҳ�������ִ��RotateRootBone
		bRotateRootBone = true;

		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);//��ǰ��׼����
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);//�����ӽ�ƫ��
		AO_Yaw = DeltaAimRotation.Yaw;//��ֵ
		
		//��ֹ��ʱ���ɫ�������ӽ�ת��
		//bUseControllerRotationYaw = false;

		//��ɫ����ת���ʱ�򣬰�AO_Yaw��ֵ��InterpAO_Yaw
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}

		//��ֹ��ʱ���ɫ�����ӽ�ת��
		bUseControllerRotationYaw = true;

		TurnInPlace(DeltaTime);//ͨ��AO_Yaw�ж�ԭ��ת���Ƿ����
	}
	if (Speed > 0.f || bIsInAir) // ���ܻ���Ծ
	{
		//���ص����ƶ����򲻿���ִ��RotateRootBone
		bRotateRootBone = false;
		
		//����ɫ�ڱ��ܻ��ڿ���������ʱ��ÿһ֡��Ϣ���ᱻ����������
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);//reset 'StartingAimRotation'
		AO_Yaw = 0.f;
		
		bUseControllerRotationYaw = true; //�ƶ���ʱ���ɫ�����ӽ�ת��

		TurningInPlace = ETurningInPlace::ETIP_NotTurning;//�ܲ���ʱ�򲻿���ת
	}
	CalculateAO_Pitch();
}

void ABlasterCharacter::CalculateAO_Pitch()
{

	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map pitch from [270, 360) to [-90, 0)
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABlasterCharacter::SimProxiesTurn()
{
	if (Combat1 == nullptr || Combat1->EquippedWeapon == nullptr) return;

	//���������ô�����û�����ʹ��RotateRootBone
	bRotateRootBone = false;


	float Speed = CalculateSpeed();
	if (Speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	//��������һ֡��ת�Ĳ�ֵ
	ProxyRotationLastFrame = ProxyRotation;//��һ֡
	ProxyRotation = GetActorRotation();//��һ֡

	//��֡��ת��ֵ������һ��
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	//UE_LOG(LogTemp, Warning, TEXT("ProxyYaw: %f"), ProxyYaw);

	//��֮֡���ת���������ֵ�󣬾Ͳ���ת�򶯻�����ͨ������ETurningInPlace
	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}

	//��֮֡���ת���������ֵ�󣬾Ͳ�����ת�򶯻�
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

}

void ABlasterCharacter::Jump()
{
	if (bDisableGameplay) return;
	
	if (bIsCrouched)//���µ�ʱ�򣬲�����
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();////���µ�ʱ�򣬲�����
	}
}

//������������Ч��
void ABlasterCharacter::FireButtonPressed()
{
	if (bDisableGameplay) return;

	if (Combat1)
	{
		Combat1->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if (bDisableGameplay) return;
	
	if (Combat1)
	{
		Combat1->FireButtonPressed(false);
	}
}


void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	//�ٽ�ֵ��-90��90���Դ�����TurningInPlace
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)	//�µ�������䡢�����жϵó���������Ҫת���ʱ��Ҳ����˵��ʱ�Ƕ�>90/<-90
	{
		//��InterpAO_Yaw��ֵ��0(4.f�Ǳ任�ٶ�)
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;
		
		//���������ת���Ƿ��㹻��
		if (FMath::Abs(AO_Yaw) < 15.f)//����ֵ��飬��15�ȶԱ�
		{
			//���С��15�ȣ��ǲ����ܹ���ת
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;//ö��״̬����
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);//��ʼ��reset 'StartingAimRotation'
		}
	}
}

/*
void ABlasterCharacter::MulticastHit_Implementation()
{
	PlayHitReactMontage();
}*/

void ABlasterCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled()) return;

	//������ƵĽ�ɫ�Ƿ������̫��(��ǽ)���ǵĻ����ؽ�ɫ����Ȼ�ᵲ��Ұ
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		
		//����ģ������
		GetMesh()->SetVisibility(false);//ֻ�ڱ���ִ�еģ����Ա��˻��ܿ�����
		
		//ǹеģ������
		if (Combat1 && Combat1->EquippedWeapon && Combat1->EquippedWeapon->GetWeaponMesh())
		{
			Combat1->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;//bOwnerNoSee��ֻ��owner������
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat1 && Combat1->EquippedWeapon && Combat1->EquippedWeapon->GetWeaponMesh())
		{
			Combat1->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ABlasterCharacter::OnRep_Health() //Ѫ���ı�ʱ
{
	UpdateHUDHealth();//����Ѫ��
	if (!bElimmed)
	{
		PlayHitReactMontage();
	}
}

void ABlasterCharacter::UpdateHUDHealth()//����Ѫ��
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}


void ABlasterCharacter::StartDissolve()
{
	//�ѻص���������Track��
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial); 
	
	//����TimeLine
	if (DissolveCurve && DissolveTimeline)
	{
		//Ϊ�˰����߼ӵ�ʱ�����ϣ�����Ҫ���ܽ��ʱ���ᡣ
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		
		//����֮��ʣ�µľ��ǿ�ʼʱ����
		DissolveTimeline->Play();
	}
}

void ABlasterCharacter::PollInit()
{
	/*
	check
	*/
	if (BlasterPlayerState == nullptr)
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if (BlasterPlayerState)
		{
			//ÿ֡ͬ����ɱ�� ����
			BlasterPlayerState->AddToScore(0.f);
			BlasterPlayerState->AddToDefeats(0);
		}
	}
}

//dissolve Timeline�Ļص���������Timeline�Ͳ��ʵ�Dissolve��
void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		//��Timeline�Ͳ��ʵ�Dissolve��
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	OverlappingWeapon = Weapon;
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

bool ABlasterCharacter::IsWeaponEquipped()
{
	return (Combat1 && Combat1->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming()
{
	return (Combat1 && Combat1->bAiming);
}

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	if (Combat1 == nullptr) return nullptr;
	return Combat1->EquippedWeapon;
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if (Combat1 == nullptr) return FVector();
	return Combat1->HitTarget;
}

ECombatState ABlasterCharacter::GetCombatState() const
{
	if (Combat1 == nullptr) return ECombatState::ECS_MAX;
	return Combat1->CombatState;
}
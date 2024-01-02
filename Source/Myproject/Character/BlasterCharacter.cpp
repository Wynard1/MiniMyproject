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

ABlasterCharacter::ABlasterCharacter()//构造函数
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
	
	//设置BlasterCharacter的碰撞类型为我们自建的ECC_SkeletalMesh
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore); 

	//把人物模型对ECC_Visibility进行回应(屏幕准心linetrace)
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);//转身速度

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;//默认值设置成不能转向

	//net update frequency
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	//TimelineComponent
	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//复制注册
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
	//死亡时武器掉落
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
	//被击杀的时候弹药清0
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDWeaponAmmo(0);
	}
	
	bElimmed = true;
	PlayElimMontage();

	//死亡时替换成溶解材质
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);

		//溶解材质的初始值
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}

	//播放timeline调整溶解材质的Dissolve参数
	StartDissolve();

	// Disable character movement
	//被击杀后禁止角色WASD移动&禁止通过转视角改变角色朝向
	bDisableGameplay = true;
	GetCharacterMovement()->DisableMovement();
	
	//被击杀后停止开火
	if (Combat1)
	{
		Combat1->FireButtonPressed(false);
	}
	
	// Disable collision
	//关闭胶囊体碰撞&关闭网格碰撞
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);	
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);			

	// Spawn elim bot
	//击杀后头上机器人的视效
	if (ElimBotEffect)
	{
		//特效所在点——头上200单位处
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		
		//发射特效
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()		//这样ElimBot就会像我们的角色一样旋转。
		);
	}

	//击杀后头上机器人的音效
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

	//局内的时候武器是不消除的，所以要获取GetMatchState用于条件判断
	ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	bool bMatchNotInProgress = BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress;

	//终局武器自毁
	if (Combat1 && Combat1->EquippedWeapon && bMatchNotInProgress)
	{
		Combat1->EquippedWeapon->Destroy();
	}
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	//血量更新
	UpdateHUDHealth();

	//绑定伤害回调函数
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

	//每帧通过+0.0同步分数
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

	//客户端或者本地，则执行完整AimOffset
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	//非“客户端或者本地”，则在OnRep_ReplicatedMovement里执行这些
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

	//PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump); 引擎自带的，不用了
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ABlasterCharacter::Jump);//我们重写后创建的，用这个

	PlayerInputComponent->BindAxis("MoveForward", this, &ABlasterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABlasterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ABlasterCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ABlasterCharacter::LookUp);

	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ABlasterCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ABlasterCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ABlasterCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ABlasterCharacter::AimButtonReleased);

	//开火    
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
	//没有武器不能播放
	if (Combat1 == nullptr || Combat1->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		
		//将播放蒙太奇，需要跳转到相应的蒙太奇部分。
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");//蒙太奇里我们自己命名的两个section name
		
		//根据名字播放动画段落
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayReloadMontage()
{
	//没有武器不能播放
	if (Combat1 == nullptr || Combat1->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		
		//将播放蒙太奇，需要跳转到相应的蒙太奇部分。
		FName SectionName;

		//选择动画要播的段落
		switch (Combat1->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		}

		//根据名字播放动画段落
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		//根据名字播放动画段落
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	//没有武器不能播放
	if (Combat1 == nullptr || Combat1->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage && !AnimInstance->IsAnyMontagePlaying() )
	{
		AnimInstance->Montage_Play(HitReactMontage);

		//将播放蒙太奇，需要跳转到相应的蒙太奇部分。
		FName SectionName("FromFront");

		//根据名字播放动画段落
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	UpdateHUDHealth();
	PlayHitReactMontage();

	//生命值为0时，执行淘汰函数
	if (Health == 0.f)
	{
		ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
		if (BlasterGameMode)
		{
			//淘汰函数的参数准备
			BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
			ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatorController);
			
			//淘汰
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
	//if (Combat1 && Combat1->EquippedWeapon == nullptr) return;//有武器是基本的
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

	if (Speed == 0.f && !bIsInAir) // 角色不动、且不在空中(的时候才会有AO)
	{
		//本地且不动可以执行RotateRootBone
		bRotateRootBone = true;

		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);//当前瞄准方向
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);//计算视角偏移
		AO_Yaw = DeltaAimRotation.Yaw;//赋值
		
		//静止的时候角色不随着视角转身
		//bUseControllerRotationYaw = false;

		//角色不能转身的时候，把AO_Yaw赋值给InterpAO_Yaw
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}

		//静止的时候角色随着视角转身
		bUseControllerRotationYaw = true;

		TurnInPlace(DeltaTime);//通过AO_Yaw判断原地转身是否合适
	}
	if (Speed > 0.f || bIsInAir) // 奔跑或跳跃
	{
		//本地但在移动，则不可以执行RotateRootBone
		bRotateRootBone = false;
		
		//当角色在奔跑或在空中有武器时，每一帧信息都会被储存起来。
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);//reset 'StartingAimRotation'
		AO_Yaw = 0.f;
		
		bUseControllerRotationYaw = true; //移动的时候角色随着视角转身

		TurningInPlace = ETurningInPlace::ETIP_NotTurning;//跑步的时候不可以转
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

	//创建变量让代理的用户不再使用RotateRootBone
	bRotateRootBone = false;


	float Speed = CalculateSpeed();
	if (Speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	//计算与上一帧旋转的差值
	ProxyRotationLastFrame = ProxyRotation;//上一帧
	ProxyRotation = GetActorRotation();//这一帧

	//两帧旋转插值并作归一化
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	//UE_LOG(LogTemp, Warning, TEXT("ProxyYaw: %f"), ProxyYaw);

	//两帧之间的转向如果比阈值大，就播放转向动画——通过设置ETurningInPlace
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

	//两帧之间的转向如果比阈值大，就不播放转向动画
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

}

void ABlasterCharacter::Jump()
{
	if (bDisableGameplay) return;
	
	if (bIsCrouched)//蹲下的时候，不蹲了
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();////蹲下的时候，不蹲了
	}
}

//按下鼠标左键的效果
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
	//临界值是-90和90，以此设置TurningInPlace
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)	//新的条件语句、反向判断得出当我们需要转身的时候，也就是说此时角度>90/<-90
	{
		//把InterpAO_Yaw插值到0(4.f是变换速度)
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;
		
		//检查我们旋转得是否足够了
		if (FMath::Abs(AO_Yaw) < 15.f)//绝对值检查，跟15度对比
		{
			//如果小于15度，那不再能够旋转
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;//枚举状态设置
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);//初始化reset 'StartingAimRotation'
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

	//自身控制的角色是否离相机太近(靠墙)，是的话隐藏角色，不然会挡视野
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		
		//人物模型隐藏
		GetMesh()->SetVisibility(false);//只在本地执行的，所以别人还能看到的
		
		//枪械模型隐藏
		if (Combat1 && Combat1->EquippedWeapon && Combat1->EquippedWeapon->GetWeaponMesh())
		{
			Combat1->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;//bOwnerNoSee：只有owner看不到
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

void ABlasterCharacter::OnRep_Health() //血量改变时
{
	UpdateHUDHealth();//更新血量
	if (!bElimmed)
	{
		PlayHitReactMontage();
	}
}

void ABlasterCharacter::UpdateHUDHealth()//更新血量
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}


void ABlasterCharacter::StartDissolve()
{
	//把回调函数绑定在Track上
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial); 
	
	//调用TimeLine
	if (DissolveCurve && DissolveTimeline)
	{
		//为了把曲线加到时间轴上，我们要用溶解的时间轴。
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		
		//在这之后剩下的就是开始时间轴
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
			//每帧同步击杀和 死亡
			BlasterPlayerState->AddToScore(0.f);
			BlasterPlayerState->AddToDefeats(0);
		}
	}
}

//dissolve Timeline的回调函数，让Timeline和材质的Dissolve绑定
void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		//让Timeline和材质的Dissolve绑定
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
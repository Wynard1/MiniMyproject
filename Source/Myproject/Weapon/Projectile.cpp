// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "Myproject/Character/BlasterCharacter.h"
#include "Myproject/Myproject.h"

// Sets default values


AProjectile::AProjectile()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;//可复制

	//碰撞盒组件构建并设置成ROOT
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);

	//碰撞和查询
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	//忽略所有碰撞
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

	//子弹跟可见物体都要碰撞
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	//子弹跟ECC_WorldStatic碰撞
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);

	//让子弹跟我们自己创建的通道ECC_SkeletalMesh碰撞
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);

	//构建射弹移动组件并挂在到root
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));//自动添加到根(CollisionBox)
	ProjectileMovementComponent->bRotationFollowsVelocity = true;//初始化、这将确保子弹的旋转与速度保持一致
																 //所以如果加上重力的衰减，旋转会沿着那个轨迹。
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	if (Tracer)
	{
		//赋值给我们.h里定义的TracerComponent
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(	
			Tracer,	//粒子名称，这里是我们在.h里定义的
			CollisionBox,	//attach to的物品
			FName(),	//如果要附加到某有名字的东西，如骨骼，就在这里
			GetActorLocation(),	//Location,跟子弹一样
			GetActorRotation(),	//Rotation,跟子弹一样
			EAttachLocation::KeepWorldPosition	//跟CollisionBox一样，会跟着走
		);
	}

	//子弹销毁
	if (HasAuthority())//只在服务器发生
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit); //参数：子弹、回调函数
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	/*
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		BlasterCharacter->MulticastHit();
	}*/

	Destroy();
}

// Called every frame
void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AProjectile::Destroyed()
{
	Super::Destroyed();

	if (ImpactParticles)
	{
		//撞击后发出粒子
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());//参数：UWorld,.h里创建的粒子系统,Transform，生成的位置信息(跟子弹一样)
	}
	if (ImpactSound)
	{	//撞击后发出声音
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());//参数：UWorld,.h里创建的粒子系统,Transform，生成的位置信息(跟子弹一样)
	}
}


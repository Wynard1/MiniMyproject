// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileGrenade.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

// AProjectileGrenade 构造函数
AProjectileGrenade::AProjectileGrenade()
{
	// Mesh初始化
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Grenade Mesh"));
	ProjectileMesh->SetupAttachment(RootComponent); 
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 设置碰撞状态为无碰撞

	// ProjectileMovementComponent初始化
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true; // 设置飞行方向朝向速度方向
	ProjectileMovementComponent->SetIsReplicated(true); // 设置支持复制（Replication）
	
	ProjectileMovementComponent->bShouldBounce = true; // 设置允许弹跳
}

// AProjectileGrenade 开始游戏时调用的函数
void AProjectileGrenade::BeginPlay()
{
	// 跳过Projectile里的 BeginPlay 函数，不要执行Onhit函数的功能，而是执行OnBounce
	AActor::BeginPlay();

	// 生成拖尾效果
	SpawnTrailSystem();

	// 启动销毁计时器
	StartDestroyTimer();

	// 绑定 OnProjectileBounce 事件，当手榴弹弹跳时调用 OnBounce 函数
	ProjectileMovementComponent->OnProjectileBounce.AddDynamic(this, &AProjectileGrenade::OnBounce);
}

// 手榴弹弹跳时调用的函数
void AProjectileGrenade::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	// 检查是否设置了弹跳音效
	if (BounceSound)
	{
		// 在手榴弹弹跳的位置播放弹跳音效
		UGameplayStatics::PlaySoundAtLocation(
			this,
			BounceSound,
			GetActorLocation()
		);
	}
}

// 手榴弹销毁时调用的函数
void AProjectileGrenade::Destroyed()
{
	// 执行榴弹爆炸造成的伤害
	ExplodeDamage();

	// 调用父类的 Destroyed 函数
	Super::Destroyed();
}

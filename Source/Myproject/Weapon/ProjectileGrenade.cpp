// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileGrenade.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

// AProjectileGrenade ���캯��
AProjectileGrenade::AProjectileGrenade()
{
	// Mesh��ʼ��
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Grenade Mesh"));
	ProjectileMesh->SetupAttachment(RootComponent); 
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // ������ײ״̬Ϊ����ײ

	// ProjectileMovementComponent��ʼ��
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true; // ���÷��з������ٶȷ���
	ProjectileMovementComponent->SetIsReplicated(true); // ����֧�ָ��ƣ�Replication��
	
	ProjectileMovementComponent->bShouldBounce = true; // ����������
}

// AProjectileGrenade ��ʼ��Ϸʱ���õĺ���
void AProjectileGrenade::BeginPlay()
{
	// ����Projectile��� BeginPlay ��������Ҫִ��Onhit�����Ĺ��ܣ�����ִ��OnBounce
	AActor::BeginPlay();

	// ������βЧ��
	SpawnTrailSystem();

	// �������ټ�ʱ��
	StartDestroyTimer();

	// �� OnProjectileBounce �¼��������񵯵���ʱ���� OnBounce ����
	ProjectileMovementComponent->OnProjectileBounce.AddDynamic(this, &AProjectileGrenade::OnBounce);
}

// ���񵯵���ʱ���õĺ���
void AProjectileGrenade::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	// ����Ƿ������˵�����Ч
	if (BounceSound)
	{
		// �����񵯵�����λ�ò��ŵ�����Ч
		UGameplayStatics::PlaySoundAtLocation(
			this,
			BounceSound,
			GetActorLocation()
		);
	}
}

// ��������ʱ���õĺ���
void AProjectileGrenade::Destroyed()
{
	// ִ���񵯱�ը��ɵ��˺�
	ExplodeDamage();

	// ���ø���� Destroyed ����
	Super::Destroyed();
}

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

	bReplicates = true;//�ɸ���

	//��ײ��������������ó�ROOT
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);

	//��ײ�Ͳ�ѯ
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	//����������ײ
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

	//�ӵ����ɼ����嶼Ҫ��ײ
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	//�ӵ���ECC_WorldStatic��ײ
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);

	//���ӵ��������Լ�������ͨ��ECC_SkeletalMesh��ײ
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);

	//�����䵯�ƶ���������ڵ�root
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));//�Զ���ӵ���(CollisionBox)
	ProjectileMovementComponent->bRotationFollowsVelocity = true;//��ʼ�����⽫ȷ���ӵ�����ת���ٶȱ���һ��
																 //�����������������˥������ת�������Ǹ��켣��
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	if (Tracer)
	{
		//��ֵ������.h�ﶨ���TracerComponent
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(	
			Tracer,	//�������ƣ�������������.h�ﶨ���
			CollisionBox,	//attach to����Ʒ
			FName(),	//���Ҫ���ӵ�ĳ�����ֵĶ��������������������
			GetActorLocation(),	//Location,���ӵ�һ��
			GetActorRotation(),	//Rotation,���ӵ�һ��
			EAttachLocation::KeepWorldPosition	//��CollisionBoxһ�����������
		);
	}

	//�ӵ�����
	if (HasAuthority())//ֻ�ڷ���������
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit); //�������ӵ����ص�����
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
		//ײ���󷢳�����
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());//������UWorld,.h�ﴴ��������ϵͳ,Transform�����ɵ�λ����Ϣ(���ӵ�һ��)
	}
	if (ImpactSound)
	{	//ײ���󷢳�����
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());//������UWorld,.h�ﴴ��������ϵͳ,Transform�����ɵ�λ����Ϣ(���ӵ�һ��)
	}
}


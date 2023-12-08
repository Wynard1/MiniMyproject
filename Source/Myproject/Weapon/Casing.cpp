// Fill out your copyright notice in the Description page of Project Settings.


#include "Casing.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

// Sets default values
ACasing::ACasing()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	//����Ϊ�����
	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);

	//�õ��ǲ�Ҫ������ͷ��ײ
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	//����ģ������
	CasingMesh->SetSimulatePhysics(true);

	//�������� 
	CasingMesh->SetEnableGravity(true);

	//��ײʱ�Ƿ���㡪����
	CasingMesh->SetNotifyRigidBodyCollision(true);

	//��ʼ�����Ƿɳ�ȥ�ĳ���
	ShellEjectionImpulse = 10.f;

}

// Called when the game starts or when spawned
void ACasing::BeginPlay()
{
	Super::BeginPlay();
	
	//OnHit������Ҫ�󶨵�OnComponentHit����
	CasingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit);////����������(����)���ص�����

	//���Ƿɳ�ȥ�ĳ���
	CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectionImpulse);

	//����ʱ��
	SetLifeSpan(3.f);
}

void ACasing::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//���ŵ����������
	if (ShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
	}

	////��ײʱ�Ƿ���㡪��������ڵ��ϻ��������ײ��һֱ��������
	// Deactivate further sounds when a bullet is hit by another one, because it annoys me
	CasingMesh->SetNotifyRigidBodyCollision(false);

}





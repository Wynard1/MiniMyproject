// Fill out your copyright notice in the Description page of Project Settings.


#include "Casing.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

// Sets default values
ACasing::ACasing()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	//设置为根组件
	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);

	//让弹壳不要跟摄像头碰撞
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	//弹壳模拟物理
	CasingMesh->SetSimulatePhysics(true);

	//弹壳重力 
	CasingMesh->SetEnableGravity(true);

	//碰撞时是否计算——是
	CasingMesh->SetNotifyRigidBodyCollision(true);

	//初始化弹壳飞出去的冲量
	ShellEjectionImpulse = 10.f;

}

// Called when the game starts or when spawned
void ACasing::BeginPlay()
{
	Super::BeginPlay();
	
	//OnHit函数需要绑定到OnComponentHit函数
	CasingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit);////参数：自身(弹壳)、回调函数

	//弹壳飞出去的冲量
	CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectionImpulse);

	//存在时常
	SetLifeSpan(3.f);
}

void ACasing::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//播放弹壳落地声音
	if (ShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
	}

	////碰撞时是否计算——否；免得在地上会计算多次碰撞，一直播放声音
	// Deactivate further sounds when a bullet is hit by another one, because it annoys me
	CasingMesh->SetNotifyRigidBodyCollision(false);

}





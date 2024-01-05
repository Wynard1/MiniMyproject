// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	/*
	* ApplyDamage的使用
	*/

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());	//为了获取OwnerController，要使用Character类(Character->Controller)，并且include它的头文件
	if (OwnerCharacter)
	{
		AController* OwnerController = OwnerCharacter->Controller;//OwnerController
		if (OwnerController)
		{
			UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
		}
	}

	//最后调用，因为会执行destroy
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}

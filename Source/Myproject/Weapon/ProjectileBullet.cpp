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
	* ApplyDamage��ʹ��
	*/

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());	//Ϊ�˻�ȡOwnerController��Ҫʹ��Character��(Character->Controller)������include����ͷ�ļ�
	if (OwnerCharacter)
	{
		AController* OwnerController = OwnerCharacter->Controller;//OwnerController
		if (OwnerController)
		{
			UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
		}
	}

	//�����ã���Ϊ��ִ��destroy
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}

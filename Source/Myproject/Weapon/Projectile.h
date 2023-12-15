// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"


UCLASS()
class MYPROJECT_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProjectile();

	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;
	 
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	//伤害
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

private:
	//在找到合适的理由之前不暴露到蓝图 
	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;

	//射弹移动组件
	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	//粒子生成
	UPROPERTY(EditAnywhere)
	class UParticleSystem* Tracer;

	//粒子生成后存储
	UPROPERTY()
	class UParticleSystemComponent* TracerComponent;

	
	//粒子系统
	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;

	//粒子声音
	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactSound;

public:	


};

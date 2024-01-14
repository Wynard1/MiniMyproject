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

	void StartDestroyTimer();	//�������ټ�ʱ��
	void DestroyTimerFinished();	//��ʱ������
	void SpawnTrailSystem();	//β��ϵͳ
	void ExplodeDamage();	//��Χ�˺�

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	//�˺�
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

	//����ϵͳ
	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;

	//��������
	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactSound;

	//���ҵ����ʵ�����֮ǰ����¶����ͼ 
	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem;

	UPROPERTY()
	class UNiagaraComponent* TrailSystemComponent;

	//�䵯�ƶ����
	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh;

	UPROPERTY(EditAnywhere)
	float DamageInnerRadius = 200.f;

	UPROPERTY(EditAnywhere)
	float DamageOuterRadius = 500.f;

private:
	//��������
	UPROPERTY(EditAnywhere)
	UParticleSystem* Tracer;

	//�������ɺ�洢
	UPROPERTY()
	class UParticleSystemComponent* TracerComponent;

	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;
public:	


};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Casing.generated.h"

UCLASS()
class MYPROJECT_API ACasing : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACasing();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);


private:
	//添加一个静态网格组件
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* CasingMesh;

	//弹壳飞出去的冲量,暴露在子类蓝图里可以在不同子弹里面编辑
	UPROPERTY(EditAnywhere)
	float ShellEjectionImpulse;

	//弹壳落地声音
	UPROPERTY(EditAnywhere)
	class USoundCue* ShellSound;

};

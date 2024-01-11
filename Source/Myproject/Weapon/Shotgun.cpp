// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Myproject/Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

void AShotgun::Fire(const FVector& HitTarget)
{
	AWeapon::Fire(HitTarget);
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();
	
	// 获取武器模型上的 MuzzleFlash 骨架插槽
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		// 获取 MuzzleFlash 插槽的变换信息
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		// 获取发射起点位置
		FVector Start = SocketTransform.GetLocation();

		// 循环执行多次，模拟散射效果
		for (uint32 i = 0; i < NumberOfPellets; i++)
		{
			// 调用 TraceEndWithScatter 函数获取经过散射计算后的射线终点位置
			FVector End = TraceEndWithScatter(Start, HitTarget);

			// 这里可以处理具体的射击效果，例如生成粒子效果、播放音效等
			// ... 处理射击效果的代码 ...
		}
	}
}
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

	// 获取武器所有者的 Pawn
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;

	// 获取武器所有者的 Controller，作为伤害的引发者
	AController* InstigatorController = OwnerPawn->GetController();

	// 获取武器模型上的 MuzzleFlash 骨架插槽
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		// 获取 MuzzleFlash 插槽的变换信息
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		// 获取发射起点位置
		FVector Start = SocketTransform.GetLocation();

		// 创建存储被命中次数的映射
		TMap<ABlasterCharacter*, uint32> HitMap;

		// 循环执行多次，模拟散射效果
		for (uint32 i = 0; i < NumberOfPellets; i++)
		{
			FHitResult FireHit;
			// 调用 WeaponTraceHit 进行射线追踪
			WeaponTraceHit(Start, HitTarget, FireHit);

			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
			if (BlasterCharacter && HasAuthority() && InstigatorController)
			{
				// 如果 HitMap 包含该角色，则增加其命中次数；否则，添加该角色到映射中
				if (HitMap.Contains(BlasterCharacter))
				{
					HitMap[BlasterCharacter]++;
				}
				else
				{
					HitMap.Emplace(BlasterCharacter, 1);
				}
			}

			// 生成命中效果粒子
			if (ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					ImpactParticles,
					FireHit.ImpactPoint,
					FireHit.ImpactNormal.Rotation()
				);
			}

			// 播放命中音效
			if (HitSound)
			{
				UGameplayStatics::PlaySoundAtLocation(
					this,
					HitSound,
					FireHit.ImpactPoint,
					.5f,
					FMath::FRandRange(-.5f, .5f)
				);
			}
		}

		// 遍历命中映射，对每个角色应用相应次数的伤害
		for (auto HitPair : HitMap)
		{
			// 检查角色、权限和有效的引发者控制器，然后应用伤害
			if (HitPair.Key && HasAuthority() && InstigatorController)
			{
				UGameplayStatics::ApplyDamage(
					HitPair.Key,
					Damage * HitPair.Value,
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);
			}
		}
	}
}

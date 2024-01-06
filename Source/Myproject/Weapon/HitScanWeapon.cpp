#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Myproject/Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();

	// 获取武器的“MuzzleFlash”骨骼插槽
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket && InstigatorController)
	{
		// 获取“MuzzleFlash”插槽的位置和方向
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();
		FVector End = Start + (HitTarget - Start) * 1.25f;	//*1.25可以保证命中

		// 用于存储射线命中结果的结构体
		FHitResult FireHit;

		UWorld* World = GetWorld();
		if (World)
		{
			// 发射射线，检测碰撞
			World->LineTraceSingleByChannel(
				FireHit, // 射线命中结果
				Start,   // 射线的起始位置
				End,     // 射线的结束位置
				ECollisionChannel::ECC_Visibility // 碰撞通道类型
			);

			// 检查射线是否与物体发生碰撞，有则bBlockingHit为true
			if (FireHit.bBlockingHit)
			{
				ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
				if (BlasterCharacter)
				{
					// 服务器上，在碰撞到的角色上应用伤害
					if (HasAuthority())
					{
						UGameplayStatics::ApplyDamage(
							BlasterCharacter,          // 目标受伤者
							Damage,                    // 造成的伤害值
							InstigatorController,      // 伤害来源的控制器
							this,                      // 伤害由哪个对象造成
							UDamageType::StaticClass() // 伤害的类型
						);
					}
				}

				//生成碰撞特效
				if (ImpactParticles)
				{
					UGameplayStatics::SpawnEmitterAtLocation(
						World,                          // 当前世界的引用
						ImpactParticles,                // 碰撞特效
						FireHit.ImpactPoint,            // 碰撞点
						FireHit.ImpactNormal.Rotation() // 碰撞法线的旋转
					);
				} 
			}
		}
	}
}
// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	//super类调用父类基本函数，比如fire里已有的播放动画函数
	Super::Fire(HitTarget);

	if (!HasAuthority()) return;//要求开火的操作只能在服务器执行

	//目前只是让SpawnParams的参数填满，所以随便填一个进去
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());

	//想办法获取骨骼，从而使用GetSocketByName获取枪口socket(MuzzleFlash)
	//另外GetWeaponMesh是weapon.h里我们自己写的Getter
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	
	if (MuzzleFlashSocket)
	{
		//获得枪口插槽的朝向信息，回头给到子弹
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		
		// From muzzle flash socket to  “‘hit location ’from TraceUnderCrosshairs”
		//枪口到集中位置的连线方向，作为子弹发射方向
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		FRotator TargetRotation = ToTarget.Rotation();

		if (ProjectileClass && InstigatorPawn)
		{
			FActorSpawnParameters SpawnParams;//要设置它的两个变量

			//在combatComponent中设置了ownenr——EquippedWeapon->SetOwner(Character)]；   所以这么写可以直接获得开枪者赋给owner
			SpawnParams.Owner = GetOwner();
			
			//上面语句创建的变量，目前占位用，后面用来判断谁造成的伤害
			SpawnParams.Instigator = InstigatorPawn;
			

			//上面的一切都是为了给SpawnActor提供参数
			UWorld* World = GetWorld();
			if (World)
			{
				World->SpawnActor<AProjectile>(
					ProjectileClass,//发射的类
					SocketTransform.GetLocation(),//生成位置
					TargetRotation,//发射的方向
					SpawnParams//发射类的其他信息
					);
			}
		}
	}
}

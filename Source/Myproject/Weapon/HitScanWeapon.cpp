#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Myproject/Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/KismetMathLibrary.h"
#include "WeaponTypes.h"

#include "DrawDebugHelpers.h"


void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	// 获取武器所属的Pawn对象
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	// 获取武器所属Pawn的控制器
	AController* InstigatorController = OwnerPawn->GetController();

	// 获取武器的“MuzzleFlash”骨骼插槽，确定开火位置
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		// 获取“MuzzleFlash”插槽的位置和方向
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();
		//FVector End = Start + (HitTarget - Start) * 1.25f;	// *1.25可以保证命中

		// 用于存储射线命中结果的结构体
		FHitResult FireHit;
		
		// 调用武器射线命中函数，计算命中位置和相关信息
		WeaponTraceHit(Start, HitTarget, FireHit);

		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
		if (BlasterCharacter && HasAuthority() && InstigatorController)
		{
			// 对命中的BlasterCharacter应用伤害
			UGameplayStatics::ApplyDamage(
				BlasterCharacter,
				Damage,
				InstigatorController,
				this,
				UDamageType::StaticClass()
			);
		}

		// 生成命中粒子效果
		if (ImpactParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				ImpactParticles,
				FireHit.ImpactPoint,
				FireHit.ImpactNormal.Rotation()
			);
		}

		// 播放命中声音
		if (HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				HitSound,
				FireHit.ImpactPoint
			);
		}

		// 生成枪口火焰特效
		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),               // 当前世界的引用
				MuzzleFlash,         // 枪口火焰特效
				SocketTransform      // 武器枪口的位置和方向
			);
		}

		// 播放射击声音
		if (FireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,                // 当前武器对象
				FireSound,           // 射击声音
				GetActorLocation()   // 获取当前武器所在的位置
			);
		}
	}
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	UWorld* World = GetWorld();
	if (World)
	{
		// 根据是否使用散射，计算线段的终点位置
		FVector End = bUseScatter ? TraceEndWithScatter(TraceStart, HitTarget) : TraceStart + (HitTarget - TraceStart) * 1.25f;

		// 执行单一线段跟踪，将结果存储在OutHit中
		World->LineTraceSingleByChannel(
			OutHit,
			TraceStart,
			End,
			ECollisionChannel::ECC_Visibility	//线段跟踪的碰撞通道
		);

		// 初始化BeamEnd为End
		FVector BeamEnd = End;
		// 如果存在阻挡击中，将BeamEnd更新为碰撞点
		if (OutHit.bBlockingHit)
		{
			BeamEnd = OutHit.ImpactPoint;
		}

		// 如果存在BeamParticles
		if (BeamParticles)
		{
			// 在指定位置生成粒子效果
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				World,
				BeamParticles,
				TraceStart,
				FRotator::ZeroRotator,
				true
			);
			// 如果生成成功
			if (Beam)
			{
				// 设置粒子效果
				Beam->SetVectorParameter(FName("Target"), BeamEnd);
			}
		}
	}
}

FVector AHitScanWeapon::TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget)
{
	/*
	创建一个虚拟球
	*/
	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();	// 计算从 TraceStart 到 HitTarget 的单位向量
	
	//将起始点 TraceStart 沿着指向目标的方向向量 ToTargetNormalized 移动了一定距离 DistanceToSphere。
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;	// 计算虚拟球体的中心位置。

	//在虚拟球内随机取一个点
	//UKismetMathLibrary::RandomUnitVector() 返回一个单位球体表面上均匀分布的随机单位向量
	//然后通过乘以 FMath::FRandRange(0.f, SphereRadius) 将其缩放到半径为 SphereRadius 的球体内。
	FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);		

	// 计算线段的结束位置
	//将虚拟球体的中心位置 SphereCenter 和在球体内部随机选择的点 RandVec 相加，得到线段的终点。
	FVector EndLoc = SphereCenter + RandVec;

	// 计算线段的方向向量
	//从 TraceStart 指向 EndLoc 的向量。
	FVector ToEndLoc = EndLoc - TraceStart;

	/*
	// 在虚拟球体的中心位置和线段结束位置处绘制调试球体
	DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);	//虚拟球体的中心位置
	DrawDebugSphere(GetWorld(), EndLoc, 4.f, 12, FColor::Orange, true);	//线段的结束位置

	// 绘制一条调试线，表示线段的路径.从 TraceStart 开始，沿着方向向量 ToEndLoc 绘制一条线段，长度被限制在 TRACE_LENGTH 内。
	DrawDebugLine(
		GetWorld(),
		TraceStart,
		FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size()), // 限制线段的长度
		FColor::Cyan,
		true);
	*/
	// 返回线段的结束位置，确保线段的长度不超过 TRACE_LENGTH。
	return FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size());
}


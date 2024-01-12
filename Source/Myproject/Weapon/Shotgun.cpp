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

	// ��ȡ���������ߵ� Pawn
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;

	// ��ȡ���������ߵ� Controller����Ϊ�˺���������
	AController* InstigatorController = OwnerPawn->GetController();

	// ��ȡ����ģ���ϵ� MuzzleFlash �Ǽܲ��
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		// ��ȡ MuzzleFlash ��۵ı任��Ϣ
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		// ��ȡ�������λ��
		FVector Start = SocketTransform.GetLocation();

		// �����洢�����д�����ӳ��
		TMap<ABlasterCharacter*, uint32> HitMap;

		// ѭ��ִ�ж�Σ�ģ��ɢ��Ч��
		for (uint32 i = 0; i < NumberOfPellets; i++)
		{
			FHitResult FireHit;
			// ���� WeaponTraceHit ��������׷��
			WeaponTraceHit(Start, HitTarget, FireHit);

			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
			if (BlasterCharacter && HasAuthority() && InstigatorController)
			{
				// ��� HitMap �����ý�ɫ�������������д�����������Ӹý�ɫ��ӳ����
				if (HitMap.Contains(BlasterCharacter))
				{
					HitMap[BlasterCharacter]++;
				}
				else
				{
					HitMap.Emplace(BlasterCharacter, 1);
				}
			}

			// ��������Ч������
			if (ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					ImpactParticles,
					FireHit.ImpactPoint,
					FireHit.ImpactNormal.Rotation()
				);
			}

			// ����������Ч
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

		// ��������ӳ�䣬��ÿ����ɫӦ����Ӧ�������˺�
		for (auto HitPair : HitMap)
		{
			// ����ɫ��Ȩ�޺���Ч�������߿�������Ȼ��Ӧ���˺�
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

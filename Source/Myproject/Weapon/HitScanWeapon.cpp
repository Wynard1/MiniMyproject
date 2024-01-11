#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Myproject/Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/KismetMathLibrary.h"
#include "WeaponTypes.h"

#include "DrawDebugHelpers.h"

// ����������������������߼�
void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();

	// ��ȡ�����ġ�MuzzleFlash��������ۣ�ȷ������λ��
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		// ��ȡ��MuzzleFlash����۵�λ�úͷ���
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();
		FVector End = Start + (HitTarget - Start) * 1.25f;	// *1.25���Ա�֤����

		// ���ڴ洢�������н���Ľṹ��
		FHitResult FireHit;

		UWorld* World = GetWorld();
		if (World)
		{
			// �������ߣ������ײ
			World->LineTraceSingleByChannel(
				FireHit, // �������н��
				Start,   // ���ߵ���ʼλ��
				End,     // ���ߵĽ���λ��
				ECollisionChannel::ECC_Visibility // ��ײͨ������
			);

			FVector BeamEnd = End;

			// ��������Ƿ������巢����ײ������bBlockingHitΪtrue
			if (FireHit.bBlockingHit)
			{
				BeamEnd = FireHit.ImpactPoint;

				ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
				if (BlasterCharacter && HasAuthority() && InstigatorController)
				{
					// �������ϣ�����ײ���Ľ�ɫ��Ӧ���˺�
					UGameplayStatics::ApplyDamage(
						BlasterCharacter,          // Ŀ��������
						Damage,                    // ��ɵ��˺�ֵ
						InstigatorController,      // �˺���Դ�Ŀ�����
						this,                      // �˺����ĸ��������
						UDamageType::StaticClass() // �˺�������
					);
				}

				// ������ײ��Ч
				if (ImpactParticles)
				{
					UGameplayStatics::SpawnEmitterAtLocation(
						World,                          // ��ǰ���������
						ImpactParticles,                // ��ײ��Ч
						FireHit.ImpactPoint,            // ��ײ��
						FireHit.ImpactNormal.Rotation() // ��ײ���ߵ���ת
					);
				}

				// ������������
				if (HitSound)
				{
					UGameplayStatics::PlaySoundAtLocation(
						this,
						HitSound,
						FireHit.ImpactPoint
					);
				}
			}

			// ���ɹ�����Ч
			if (BeamParticles)
			{
				// ���ɹ�����Ч����������ĩ��λ��
				UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
					World,               // ��ǰ���������
					BeamParticles,       // ������Ч
					SocketTransform     // ���ɹ�����Ч��λ�úͷ���
				);
				if (Beam)
				{
					// ���ù�����ĩ��λ��
					Beam->SetVectorParameter(FName("Target"), BeamEnd);
				}
			}
		}

		// ����ǹ�ڻ�����Ч
		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				World,               // ��ǰ���������
				MuzzleFlash,         // ǹ�ڻ�����Ч
				SocketTransform      // ����ǹ�ڵ�λ�úͷ���
			);
		}

		// �����������
		if (FireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,                // ��ǰ��������
				FireSound,           // �������
				GetActorLocation()   // ��ȡ��ǰ�������ڵ�λ��
			);
		}
	}
}


FVector AHitScanWeapon::TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget)
{
	/*
	����һ��������
	*/
	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();	// ����� TraceStart �� HitTarget �ĵ�λ����
	
	//����ʼ�� TraceStart ����ָ��Ŀ��ķ������� ToTargetNormalized �ƶ���һ������ DistanceToSphere��
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;	// �����������������λ�á�

	//�������������ȡһ����
	//UKismetMathLibrary::RandomUnitVector() ����һ����λ��������Ͼ��ȷֲ��������λ����
	//Ȼ��ͨ������ FMath::FRandRange(0.f, SphereRadius) �������ŵ��뾶Ϊ SphereRadius �������ڡ�
	FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);		

	// �����߶εĽ���λ��
	//���������������λ�� SphereCenter ���������ڲ����ѡ��ĵ� RandVec ��ӣ��õ��߶ε��յ㡣
	FVector EndLoc = SphereCenter + RandVec;

	// �����߶εķ�������
	//�� TraceStart ָ�� EndLoc ��������
	FVector ToEndLoc = EndLoc - TraceStart;

	// ���������������λ�ú��߶ν���λ�ô����Ƶ�������
	DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);	//�������������λ��
	DrawDebugSphere(GetWorld(), EndLoc, 4.f, 12, FColor::Orange, true);	//�߶εĽ���λ��

	// ����һ�������ߣ���ʾ�߶ε�·��.�� TraceStart ��ʼ�����ŷ������� ToEndLoc ����һ���߶Σ����ȱ������� TRACE_LENGTH �ڡ�
	DrawDebugLine(
		GetWorld(),
		TraceStart,
		FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size()), // �����߶εĳ���
		FColor::Cyan,
		true);

	// �����߶εĽ���λ�ã�ȷ���߶εĳ��Ȳ����� TRACE_LENGTH��
	return FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size());
}


#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Myproject/Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

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

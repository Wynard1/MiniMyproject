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

	// ��ȡ�����ġ�MuzzleFlash���������
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket && InstigatorController)
	{
		// ��ȡ��MuzzleFlash����۵�λ�úͷ���
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();
		FVector End = Start + (HitTarget - Start) * 1.25f;	//*1.25���Ա�֤����

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

			// ��������Ƿ������巢����ײ������bBlockingHitΪtrue
			if (FireHit.bBlockingHit)
			{
				ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
				if (BlasterCharacter)
				{
					// �������ϣ�����ײ���Ľ�ɫ��Ӧ���˺�
					if (HasAuthority())
					{
						UGameplayStatics::ApplyDamage(
							BlasterCharacter,          // Ŀ��������
							Damage,                    // ��ɵ��˺�ֵ
							InstigatorController,      // �˺���Դ�Ŀ�����
							this,                      // �˺����ĸ��������
							UDamageType::StaticClass() // �˺�������
						);
					}
				}

				//������ײ��Ч
				if (ImpactParticles)
				{
					UGameplayStatics::SpawnEmitterAtLocation(
						World,                          // ��ǰ���������
						ImpactParticles,                // ��ײ��Ч
						FireHit.ImpactPoint,            // ��ײ��
						FireHit.ImpactNormal.Rotation() // ��ײ���ߵ���ת
					);
				} 
			}
		}
	}
}
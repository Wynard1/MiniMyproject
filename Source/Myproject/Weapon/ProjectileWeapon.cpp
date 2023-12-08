// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	//super����ø����������������fire�����еĲ��Ŷ�������
	Super::Fire(HitTarget);

	if (!HasAuthority()) return;//Ҫ�󿪻�Ĳ���ֻ���ڷ�����ִ��

	//Ŀǰֻ����SpawnParams�Ĳ������������������һ����ȥ
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());

	//��취��ȡ�������Ӷ�ʹ��GetSocketByName��ȡǹ��socket(MuzzleFlash)
	//����GetWeaponMesh��weapon.h�������Լ�д��Getter
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	
	if (MuzzleFlashSocket)
	{
		//���ǹ�ڲ�۵ĳ�����Ϣ����ͷ�����ӵ�
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		
		// From muzzle flash socket to  ����hit location ��from TraceUnderCrosshairs��
		//ǹ�ڵ�����λ�õ����߷�����Ϊ�ӵ����䷽��
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		FRotator TargetRotation = ToTarget.Rotation();

		if (ProjectileClass && InstigatorPawn)
		{
			FActorSpawnParameters SpawnParams;//Ҫ����������������

			//��combatComponent��������ownenr����EquippedWeapon->SetOwner(Character)]��   ������ôд����ֱ�ӻ�ÿ�ǹ�߸���owner
			SpawnParams.Owner = GetOwner();
			
			//������䴴���ı�����Ŀǰռλ�ã����������ж�˭��ɵ��˺�
			SpawnParams.Instigator = InstigatorPawn;
			

			//�����һ�ж���Ϊ�˸�SpawnActor�ṩ����
			UWorld* World = GetWorld();
			if (World)
			{
				World->SpawnActor<AProjectile>(
					ProjectileClass,//�������
					SocketTransform.GetLocation(),//����λ��
					TargetRotation,//����ķ���
					SpawnParams//�������������Ϣ
					);
			}
		}
	}
}

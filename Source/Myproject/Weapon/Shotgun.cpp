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
	
	// ��ȡ����ģ���ϵ� MuzzleFlash �Ǽܲ��
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		// ��ȡ MuzzleFlash ��۵ı任��Ϣ
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		// ��ȡ�������λ��
		FVector Start = SocketTransform.GetLocation();

		// ѭ��ִ�ж�Σ�ģ��ɢ��Ч��
		for (uint32 i = 0; i < NumberOfPellets; i++)
		{
			// ���� TraceEndWithScatter ������ȡ����ɢ������������յ�λ��
			FVector End = TraceEndWithScatter(Start, HitTarget);

			// ������Դ����������Ч����������������Ч����������Ч��
			// ... �������Ч���Ĵ��� ...
		}
	}
}
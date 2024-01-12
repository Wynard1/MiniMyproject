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

	// ��ȡ����������Pawn����
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	// ��ȡ��������Pawn�Ŀ�����
	AController* InstigatorController = OwnerPawn->GetController();

	// ��ȡ�����ġ�MuzzleFlash��������ۣ�ȷ������λ��
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		// ��ȡ��MuzzleFlash����۵�λ�úͷ���
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();
		//FVector End = Start + (HitTarget - Start) * 1.25f;	// *1.25���Ա�֤����

		// ���ڴ洢�������н���Ľṹ��
		FHitResult FireHit;
		
		// ���������������к�������������λ�ú������Ϣ
		WeaponTraceHit(Start, HitTarget, FireHit);

		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
		if (BlasterCharacter && HasAuthority() && InstigatorController)
		{
			// �����е�BlasterCharacterӦ���˺�
			UGameplayStatics::ApplyDamage(
				BlasterCharacter,
				Damage,
				InstigatorController,
				this,
				UDamageType::StaticClass()
			);
		}

		// ������������Ч��
		if (ImpactParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				ImpactParticles,
				FireHit.ImpactPoint,
				FireHit.ImpactNormal.Rotation()
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

		// ����ǹ�ڻ�����Ч
		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),               // ��ǰ���������
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

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	UWorld* World = GetWorld();
	if (World)
	{
		// �����Ƿ�ʹ��ɢ�䣬�����߶ε��յ�λ��
		FVector End = bUseScatter ? TraceEndWithScatter(TraceStart, HitTarget) : TraceStart + (HitTarget - TraceStart) * 1.25f;

		// ִ�е�һ�߶θ��٣�������洢��OutHit��
		World->LineTraceSingleByChannel(
			OutHit,
			TraceStart,
			End,
			ECollisionChannel::ECC_Visibility	//�߶θ��ٵ���ײͨ��
		);

		// ��ʼ��BeamEndΪEnd
		FVector BeamEnd = End;
		// ��������赲���У���BeamEnd����Ϊ��ײ��
		if (OutHit.bBlockingHit)
		{
			BeamEnd = OutHit.ImpactPoint;
		}

		// �������BeamParticles
		if (BeamParticles)
		{
			// ��ָ��λ����������Ч��
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				World,
				BeamParticles,
				TraceStart,
				FRotator::ZeroRotator,
				true
			);
			// ������ɳɹ�
			if (Beam)
			{
				// ��������Ч��
				Beam->SetVectorParameter(FName("Target"), BeamEnd);
			}
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

	/*
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
	*/
	// �����߶εĽ���λ�ã�ȷ���߶εĳ��Ȳ����� TRACE_LENGTH��
	return FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size());
}


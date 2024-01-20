#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

UCLASS()
class MYPROJECT_API APickup : public AActor
{
	GENERATED_BODY()

public:
	// ���캯��
	APickup();

	// ÿ֡����ʱ����
	virtual void Tick(float DeltaTime) override;

	// ����ʱ����
	virtual void Destroyed() override;

protected:
	// �ڿ�ʼ����ʱ����
	virtual void BeginPlay() override;

	// ����Overlap�¼�������
	UFUNCTION()
	virtual void OnSphereOverlap(
			UPrimitiveComponent* OverlappedComponent,    // �����ص������
			AActor* OtherActor,                          // ��Overlap Sphere�����ص�������Actor
			UPrimitiveComponent* OtherComp,              // ��Overlap Sphere�����ص����������
			int32 OtherBodyIndex,                        // ��Overlap Sphere�����ص������������Body Index
			bool bFromSweep,                             // �Ƿ�����ɨ�����������ص�
			const FHitResult& SweepResult                // ɨ����������н��
		);

	UPROPERTY(EditAnywhere)
	float BaseTurnRate = 45.f;

private:
	// ����Overlap������
	UPROPERTY(EditAnywhere)
	class USphereComponent* OverlapSphere;

	// Pickup����ʱ���ŵ���Ч
	UPROPERTY(EditAnywhere)
	class USoundCue* PickupSound;

	// ��̬�������
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* PickupMesh;

	// ʰȡ��Ʒչʾ�õ�����Ч��
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	class UNiagaraComponent* PickupEffectComponent;

	// ʰȡʱ��������Ч
	UPROPERTY(EditAnywhere, Category = "Effects")
	class UNiagaraSystem* PickupEffect;

public:

};
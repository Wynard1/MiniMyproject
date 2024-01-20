#include "Pickup.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Components/SphereComponent.h"
#include "Myproject/Weapon/WeaponTypes.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

// ���캯��
APickup::APickup()
{
	// �������ÿ֡����
	PrimaryActorTick.bCanEverTick = true;
	// ����ΪReplicated��ʹ�ø�Actor��������Ϸ���ܹ�ͬ��
	bReplicates = true;

	// ���������ø����
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// ��OverlapSphere���ӵ������
	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
	OverlapSphere->SetupAttachment(RootComponent);  // ��OverlapSphere������ӵ������
	OverlapSphere->SetSphereRadius(150.f);  // ����OverlapSphere�뾶Ϊ150
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);  // ����OverlapSphereΪ��ѯģʽ��������������ײ
	OverlapSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);  // ���Զ�����Ƶ������ײ��Ӧ
	OverlapSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);  // ��PawnƵ������ΪOverlap��Ӧ
	OverlapSphere->AddLocalOffset(FVector(0.f, 0.f, 85.f));	//��ʼ��OverlapSphereλ��

	// ����������PickupMesh���Ϊ��̬��������������丽�ӵ�OverlapSphere���
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(OverlapSphere);  // ��PickupMesh������ӵ�OverlapSphere���
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);  // ����PickupMeshΪ����ײ
	PickupMesh->SetRelativeScale3D(FVector(5.f, 5.f, 5.f));	//��ʼ����С
	PickupMesh->SetRenderCustomDepth(true);	//��ʼ�����
	PickupMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_PURPLE);	//��ʼ�����Ϊ��ɫ

	// ���� Niagara ����������丽�ŵ������
	PickupEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PickupEffectComponent"));
	PickupEffectComponent->SetupAttachment(RootComponent);
}

void APickup::BeginPlay()
{
	Super::BeginPlay();

	// ����Ƿ�����ʵ�������Overlap�¼�
	if (HasAuthority())
	{
		OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &APickup::OnSphereOverlap);
	}
}

// ��Overlap Sphere���������巢���ص�ʱ���õĺ���
void APickup::OnSphereOverlap(
	UPrimitiveComponent* OverlappedComponent,    // �����ص������
	AActor* OtherActor,                          // ��Overlap Sphere�����ص�������Actor
	UPrimitiveComponent* OtherComp,              // ��Overlap Sphere�����ص����������
	int32 OtherBodyIndex,                        // ��Overlap Sphere�����ص������������Body Index
	bool bFromSweep,                             // �Ƿ�����ɨ�����������ص�
	const FHitResult& SweepResult                // ɨ����������н��
)
{

}

// ÿ֡����ʱ����
void APickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//ģ����ת
	if (PickupMesh)
	{
		PickupMesh->AddWorldRotation(FRotator(0.f, BaseTurnRate * DeltaTime, 0.f));
	}
}

// ����ʱ����
void APickup::Destroyed()
{
	Super::Destroyed();

	// �������PickupSound��������Ч
	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			PickupSound,
			GetActorLocation()
		);
	}

	//�����PickupEffect������
	if (PickupEffect)
	{
		// ��ʵ��������ʱ����λ������ Niagara ϵͳ
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			PickupEffect,
			GetActorLocation(),
			GetActorRotation()
		);
	}
}
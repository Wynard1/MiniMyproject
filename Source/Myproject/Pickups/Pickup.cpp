#include "Pickup.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Components/SphereComponent.h"
#include "Myproject/Weapon/WeaponTypes.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

// 构造函数
APickup::APickup()
{
	// 允许进行每帧更新
	PrimaryActorTick.bCanEverTick = true;
	// 设置为Replicated，使得该Actor在网络游戏中能够同步
	bReplicates = true;

	// 创建并设置根组件
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// 将OverlapSphere附加到根组件
	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
	OverlapSphere->SetupAttachment(RootComponent);  // 将OverlapSphere组件附加到根组件
	OverlapSphere->SetSphereRadius(150.f);  // 设置OverlapSphere半径为150
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);  // 设置OverlapSphere为查询模式，不进行物理碰撞
	OverlapSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);  // 忽略对所有频道的碰撞响应
	OverlapSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);  // 对Pawn频道设置为Overlap响应
	OverlapSphere->AddLocalOffset(FVector(0.f, 0.f, 85.f));	//初始化OverlapSphere位置

	// 创建并设置PickupMesh组件为静态网格组件，并将其附加到OverlapSphere组件
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(OverlapSphere);  // 将PickupMesh组件附加到OverlapSphere组件
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);  // 设置PickupMesh为无碰撞
	PickupMesh->SetRelativeScale3D(FVector(5.f, 5.f, 5.f));	//初始化大小
	PickupMesh->SetRenderCustomDepth(true);	//初始化描边
	PickupMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_PURPLE);	//初始化描边为紫色

	// 创建 Niagara 组件并设置其附着到根组件
	PickupEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PickupEffectComponent"));
	PickupEffectComponent->SetupAttachment(RootComponent);
}

void APickup::BeginPlay()
{
	Super::BeginPlay();

	// 如果是服务器实例，则绑定Overlap事件
	if (HasAuthority())
	{
		OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &APickup::OnSphereOverlap);
	}
}

// 当Overlap Sphere与其他物体发生重叠时调用的函数
void APickup::OnSphereOverlap(
	UPrimitiveComponent* OverlappedComponent,    // 发生重叠的组件
	AActor* OtherActor,                          // 与Overlap Sphere发生重叠的其他Actor
	UPrimitiveComponent* OtherComp,              // 与Overlap Sphere发生重叠的其他组件
	int32 OtherBodyIndex,                        // 与Overlap Sphere发生重叠的其他组件的Body Index
	bool bFromSweep,                             // 是否是由扫描操作引起的重叠
	const FHitResult& SweepResult                // 扫描操作的命中结果
)
{

}

// 每帧更新时调用
void APickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//模型自转
	if (PickupMesh)
	{
		PickupMesh->AddWorldRotation(FRotator(0.f, BaseTurnRate * DeltaTime, 0.f));
	}
}

// 销毁时调用
void APickup::Destroyed()
{
	Super::Destroyed();

	// 如果存在PickupSound，播放音效
	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			PickupSound,
			GetActorLocation()
		);
	}

	//如果有PickupEffect，播放
	if (PickupEffect)
	{
		// 在实例被销毁时，在位置生成 Niagara 系统
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			PickupEffect,
			GetActorLocation(),
			GetActorRotation()
		);
	}
}
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

UCLASS()
class MYPROJECT_API APickup : public AActor
{
	GENERATED_BODY()

public:
	// 构造函数
	APickup();

	// 每帧更新时调用
	virtual void Tick(float DeltaTime) override;

	// 销毁时调用
	virtual void Destroyed() override;

protected:
	// 在开始播放时调用
	virtual void BeginPlay() override;

	// 球体Overlap事件处理函数
	UFUNCTION()
	virtual void OnSphereOverlap(
			UPrimitiveComponent* OverlappedComponent,    // 发生重叠的组件
			AActor* OtherActor,                          // 与Overlap Sphere发生重叠的其他Actor
			UPrimitiveComponent* OtherComp,              // 与Overlap Sphere发生重叠的其他组件
			int32 OtherBodyIndex,                        // 与Overlap Sphere发生重叠的其他组件的Body Index
			bool bFromSweep,                             // 是否是由扫描操作引起的重叠
			const FHitResult& SweepResult                // 扫描操作的命中结果
		);

	UPROPERTY(EditAnywhere)
	float BaseTurnRate = 45.f;

private:
	// 球体Overlap检测组件
	UPROPERTY(EditAnywhere)
	class USphereComponent* OverlapSphere;

	// Pickup销毁时播放的音效
	UPROPERTY(EditAnywhere)
	class USoundCue* PickupSound;

	// 静态网格组件
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* PickupMesh;

	// 拾取物品展示用的粒子效果
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	class UNiagaraComponent* PickupEffectComponent;

	// 拾取时触发的特效
	UPROPERTY(EditAnywhere, Category = "Effects")
	class UNiagaraSystem* PickupEffect;

public:

};
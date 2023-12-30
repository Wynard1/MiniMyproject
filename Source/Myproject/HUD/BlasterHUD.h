// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

//准心的数据结构
USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()
public:
	//图像信息
	class UTexture2D* CrosshairsCenter;
	UTexture2D* CrosshairsLeft;
	UTexture2D* CrosshairsRight;
	UTexture2D* CrosshairsTop;
	UTexture2D* CrosshairsBottom;

	//准心扩散
	float CrosshairSpread;

	FLinearColor CrosshairsColor;
};

/**
 *
 */
UCLASS()
class MYPROJECT_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;

	//蓝图里暴露的选择CharacterOverlay
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<class UUserWidget> CharacterOverlayClass;

	void AddCharacterOverlay();

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	//蓝图设置项
	UPROPERTY(EditAnywhere, Category = "Announcements")
	TSubclassOf<UUserWidget> AnnouncementClass;

	//函数变量存储项
	UPROPERTY()
	class UAnnouncement* Announcement;

	//显示用函数
	void AddAnnouncement();


protected:
	virtual void BeginPlay() override;

	
private:
	//在这个头文件里自定义的数据结构
	FHUDPackage HUDPackage;

	//自己创建的，用于画准心的函数
	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor);

	//准心最大扩散，蓝图内可编辑
	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 16.f;
public:
	//public setter
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
};

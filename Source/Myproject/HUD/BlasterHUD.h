// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

//׼�ĵ����ݽṹ
USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()
public:
	//ͼ����Ϣ
	class UTexture2D* CrosshairsCenter;
	UTexture2D* CrosshairsLeft;
	UTexture2D* CrosshairsRight;
	UTexture2D* CrosshairsTop;
	UTexture2D* CrosshairsBottom;

	//׼����ɢ
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

	//��ͼ�ﱩ¶��ѡ��CharacterOverlay
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<class UUserWidget> CharacterOverlayClass;

	void AddCharacterOverlay();

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	//��ͼ������
	UPROPERTY(EditAnywhere, Category = "Announcements")
	TSubclassOf<UUserWidget> AnnouncementClass;

	//���������洢��
	UPROPERTY()
	class UAnnouncement* Announcement;

	//��ʾ�ú���
	void AddAnnouncement();


protected:
	virtual void BeginPlay() override;

	
private:
	//�����ͷ�ļ����Զ�������ݽṹ
	FHUDPackage HUDPackage;

	//�Լ������ģ����ڻ�׼�ĵĺ���
	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor);

	//׼�������ɢ����ͼ�ڿɱ༭
	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 16.f;
public:
	//public setter
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
};

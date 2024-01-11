#pragma once

#define TRACE_LENGTH 80000.f

UENUM(BlueprintType)	// ����Ժ�Ҫ����ͼ��ʹ����
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),       // ͻ����ǹ
	EWT_RocketLauncher UMETA(DisplayName = "Rocket Launcher"),   // ���������
	EWT_Pistol UMETA(DisplayName = "Pistol"),                   // ��ǹ
	EWT_SubmachineGun UMETA(DisplayName = "Submachine Gun"),     // ���ǹ
	EWT_Shotgun UMETA(DisplayName = "Shotgun"),

	EWT_MAX UMETA(DisplayName = "DefaultMAX")
};

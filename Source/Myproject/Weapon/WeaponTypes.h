#pragma once

#define TRACE_LENGTH 80000.f

#define CUSTOM_DEPTH_PURPLE 250
#define CUSTOM_DEPTH_BLUE 251
#define CUSTOM_DEPTH_TAN 252

UENUM(BlueprintType)	// ����Ժ�Ҫ����ͼ��ʹ����
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),       // ͻ����ǹ
	EWT_RocketLauncher UMETA(DisplayName = "Rocket Launcher"),   // ���������
	EWT_Pistol UMETA(DisplayName = "Pistol"),                   // ��ǹ
	EWT_SubmachineGun UMETA(DisplayName = "Submachine Gun"),     // ���ǹ
	EWT_Shotgun UMETA(DisplayName = "Shotgun"),
	EWT_SniperRifle UMETA(DisplayName = "Sniper Rifle"),
	EWT_GrenadeLauncher UMETA(DisplayName = "Grenade Launcher"),

	EWT_MAX UMETA(DisplayName = "DefaultMAX")
};

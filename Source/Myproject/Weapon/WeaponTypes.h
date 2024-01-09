#pragma once

UENUM(BlueprintType)	// 如果以后要在蓝图中使用它
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),       // 突击步枪
	EWT_RocketLauncher UMETA(DisplayName = "Rocket Launcher"),   // 火箭发射器
	EWT_Pistol UMETA(DisplayName = "Pistol"),                   // 手枪
	EWT_SubmachineGun UMETA(DisplayName = "Submachine Gun"),     // 冲锋枪

	EWT_MAX UMETA(DisplayName = "DefaultMAX")
};

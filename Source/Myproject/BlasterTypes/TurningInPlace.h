#pragma once

UENUM(BlueprintType)	//宏、可以在蓝图里用作一个类型
enum class ETurningInPlace : uint8	//用E开头，Unreal命名约定；uint8:枚举的话最好用一下
{
	ETIP_Left UMETA(DisplayName = "Turning Left"),	//ENUM、用E开头
	ETIP_Right UMETA(DisplayName = "Turning Right"),
	ETIP_NotTurning UMETA(DisplayName = "Not Turning"),

	ETIP_MAX UMETA(DisplayName = "DefaultMAX")
};
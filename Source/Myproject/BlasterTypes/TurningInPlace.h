#pragma once

UENUM(BlueprintType)	//�ꡢ��������ͼ������һ������
enum class ETurningInPlace : uint8	//��E��ͷ��Unreal����Լ����uint8:ö�ٵĻ������һ��
{
	ETIP_Left UMETA(DisplayName = "Turning Left"),	//ENUM����E��ͷ
	ETIP_Right UMETA(DisplayName = "Turning Right"),
	ETIP_NotTurning UMETA(DisplayName = "Not Turning"),

	ETIP_MAX UMETA(DisplayName = "DefaultMAX")
};
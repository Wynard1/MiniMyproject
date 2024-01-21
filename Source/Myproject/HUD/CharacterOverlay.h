// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

/**
 *
 */
UCLASS()
class MYPROJECT_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()
public:
	/*
	绑定血量
	*/
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* HealthText;

	/*
	绑定护盾
	*/
	UPROPERTY(meta = (BindWidget))
	UProgressBar* ShieldBar;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ShieldText;

	/*
	score
	*/
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ScoreAmount;

	/*
	defeat
	*/
	UPROPERTY(meta = (BindWidget))
	UTextBlock* DefeatsAmount;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* WeaponAmmoAmount;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CarriedAmmoAmount;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MatchCountdownText;

	//手雷数量
	UPROPERTY(meta = (BindWidget))
	UTextBlock* GrenadesText;
};
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
	//绑定WBP控件
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* HealthText;

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
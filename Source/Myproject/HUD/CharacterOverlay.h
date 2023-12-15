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
	//°ó¶¨WBP¿Ø¼þ
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
};
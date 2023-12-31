// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerController.h"
#include "Myproject/HUD/BlasterHUD.h"
#include "Myproject/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Myproject/Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Myproject/GameMode/BlasterGameMode.h"
#include "Myproject/PlayerState/BlasterPlayerState.h"
#include "Myproject/HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "Myproject/BlasterComponents/CombatComponent.h"
#include "Myproject/Weapon/Weapon.h"
#include "Myproject/GameState/BlasterGameState.h"

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	BlasterHUD = Cast<ABlasterHUD>(GetHUD());

	//流程开始执行
	ServerCheckMatchState();
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerController, MatchState);
}

void ABlasterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime(); 
	
	//每一tick，调用check同步时间的函数
	CheckTimeSync(DeltaTime);

	//每帧检查HUD数据能否设置
	PollInit();
}

void ABlasterPlayerController::CheckTimeSync(float DeltaTime)
{
	//未更新时间。每个tick增加直到超过上限
	TimeSyncRunningTime += DeltaTime;

	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());

		//更新后清0
		TimeSyncRunningTime = 0.f;
	}
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	ABlasterGameMode* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		//从Gamemode里获取数据，开始流程
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();

		//客户端获得当前流程进度
		ClientJoinMidgame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);
	}
}

void ABlasterPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	//客户端拿到当前流程进度，执行流程
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	OnMatchStateSet(MatchState);
	if (BlasterHUD && MatchState == MatchState::WaitingToStart)
	{
		BlasterHUD->AddAnnouncement();
	}
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
	if (BlasterCharacter)
	{
		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
	}
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	//保证BlasterHUD是有效的
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	
	//更简洁的写法，整合条件判断
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HealthBar &&
		BlasterHUD->CharacterOverlay->HealthText;

	if (bHUDValid)
	{
		//SetPercent的使用——血条百分比
		const float HealthPercent = Health / MaxHealth;
		BlasterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);

		//血条文字打印，FMath::CeilToInt——向上取整
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));

		//FString HealthText和CharacterOverlay->HealthText里的UTextBlock* HealthText，都是HealthText但是含义不同。
		BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));//SetText需要传入FText，所以要从FString转换成FText
	}

	else
	{
		bInitializeCharacterOverlay = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
	/*
	check
	*/
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->ScoreAmount;
	
	if (bHUDValid)
	{
		//向下取整
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		BlasterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}

	else
	{
		bInitializeCharacterOverlay = true;
		HUDScore = Score;
	}
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats)
{
	/*
	check
	*/
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->DefeatsAmount;

	if (bHUDValid)
	{
		//
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		BlasterHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDDefeats = Defeats;
	}
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->MatchCountdownText;

	if (bHUDValid)
	{
		//状态刚切换的时候，时间会有短暂负数的时候，短暂隐藏
		if (CountdownTime < 0.f)
		{
			BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}

		//设置分和秒
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;
		
		//字符打印(%02d:将数字格式化为两个数字，左边填充为0)
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void ABlasterPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->Announcement &&
		BlasterHUD->Announcement->WarmupTime;

	if (bHUDValid)
	{
		//状态刚切换的时候，时间会有短暂负数的时候，短暂隐藏
		if (CountdownTime < 0.f)
		{
			BlasterHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}
		
		//设置分和秒
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		//字符打印(%02d:将数字格式化为两个数字，左边填充为0)
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BlasterHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void ABlasterPlayerController::SetHUDTime()
{
	//获取关卡创建时间
	if (HasAuthority())
	{
		ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
		if (BlasterGameMode)
		{
			//LevelStartingTime = BlasterGameMode->GetLevelStartingTime();
			LevelStartingTime = BlasterGameMode->LevelStartingTime;
		}
	}
	
	/*
	计算各个阶段的时间
	*/
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::Cooldown) TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;

	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);
	/*
	END OF 计算各个阶段的时间
	*/

	//HUD里设置各个阶段的时间
	if (CountdownInt != SecondsLeft)
	{
		//倒计时，等待阶段和结束缓冲阶段共用
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}

		//局内阶段
		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}

	//显示部分更新完毕，剩余时间更新以进行下一秒的比较
	CountdownInt = SecondsLeft;
}

void ABlasterPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (BlasterHUD && BlasterHUD->CharacterOverlay)
		{
			CharacterOverlay = BlasterHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				SetHUDHealth(HUDHealth, HUDMaxHealth);
				SetHUDScore(HUDScore);
				SetHUDDefeats(HUDDefeats);
			}
		}
	}
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	//在服务器上，服务器会得到它自己的当前时间
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();

	//通过Client RPC调用把这个传递回来
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime);
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float ABlasterPlayerController::GetServerTime()
{
	//服务端上 ：本地时间
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();

	//客户端上 ：本地时间+修正
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ABlasterPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;

	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}

	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}

	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::HandleMatchHasStarted()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		//展示局内HUD
		//BlasterHUD->AddCharacterOverlay();
		if (BlasterHUD->CharacterOverlay == nullptr) BlasterHUD->AddCharacterOverlay();
		
		//隐藏Announcement
		if (BlasterHUD->Announcement)
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ABlasterPlayerController::HandleCooldown()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		//隐藏局内HUD
		BlasterHUD->CharacterOverlay->RemoveFromParent();
		
		bool bHUDValid = BlasterHUD->Announcement &&
			BlasterHUD->Announcement->AnnouncementText &&
			BlasterHUD->Announcement->InfoText;

		//展示Announcement
		if (bHUDValid)
		{
			//静态Announcement的HUD显示复用
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);

			//HUD内容更改
			FString AnnouncementText("New Match Starts In:");
			BlasterHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));
			
			/*
			显示获胜者
			*/
			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			
			ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();	//本地玩家
			
			if (BlasterGameState && BlasterPlayerState)
			{
				//获得获胜者
				TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScoringPlayers;

				//本地存储获胜者，用于后续打印
				FString InfoTextString;
				
				/*
				判断获胜者，取决于这个数组中有多少玩家
				*/
				//没人获胜
				if (TopPlayers.Num() == 0)
				{
					InfoTextString = FString("There is no winner.");
				}

				//一个本地玩家获胜
				else if (TopPlayers.Num() == 1 && TopPlayers[0] == BlasterPlayerState)
				{
					InfoTextString = FString("You are the winner!");
				}

				//一个非本地玩家获胜
				else if (TopPlayers.Num() == 1)
				{
					//GetPlayerName()返回FString，需要星号*
					InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerName());
				}
				
				//多个玩家
				else if (TopPlayers.Num() > 1)
				{
					InfoTextString = FString("Players tied for the win:\n");
					for (auto TiedPlayer : TopPlayers)	//每轮赋值给TiedPlayer
					{
						//Append：在尾部添加
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
					}
				}

				//打印获胜者
				BlasterHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
			}
			/*
			END OF 显示获胜者
			*/
		}
	}

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BlasterCharacter && BlasterCharacter->GetCombat())
	{
		BlasterCharacter->bDisableGameplay = true;
		BlasterCharacter->GetCombat()->FireButtonPressed(false);
	}
}
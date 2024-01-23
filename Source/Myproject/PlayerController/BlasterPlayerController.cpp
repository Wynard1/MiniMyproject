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

	//���̿�ʼִ��
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
	
	//ÿһtick������checkͬ��ʱ��ĺ���
	CheckTimeSync(DeltaTime);

	//ÿ֡���HUD�����ܷ�����
	PollInit();
}

void ABlasterPlayerController::CheckTimeSync(float DeltaTime)
{
	//δ����ʱ�䡣ÿ��tick����ֱ����������
	TimeSyncRunningTime += DeltaTime;

	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());

		//���º���0
		TimeSyncRunningTime = 0.f;
	}
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	ABlasterGameMode* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		//��Gamemode���ȡ���ݣ���ʼ����
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();

		//�ͻ��˻�õ�ǰ���̽���
		ClientJoinMidgame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);
	}
}

void ABlasterPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	//�ͻ����õ���ǰ���̽��ȣ�ִ������
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
	//��֤BlasterHUD����Ч��
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	
	//������д�������������ж�
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HealthBar &&
		BlasterHUD->CharacterOverlay->HealthText;

	//��������Ѿ���ʼ��HUD�ˣ�ֱ�ӿ�������
	if (bHUDValid)
	{
		//SetPercent��ʹ�á���Ѫ���ٷֱ�
		const float HealthPercent = Health / MaxHealth;
		BlasterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);

		//Ѫ�����ִ�ӡ��FMath::CeilToInt��������ȡ��
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));

		//FString HealthText��CharacterOverlay->HealthText���UTextBlock* HealthText������HealthText���Ǻ��岻ͬ��
		BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));	//SetText��Ҫ����FText������Ҫ��FStringת����FText
	}

	//���滹û�г�ʼ��HUD��ʱ���Ȱѱ����洢�������ȵ�PollInit�����µ��������
	else
	{
		//���������Ƿ�Ҫ��ʼ����Bool
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ABlasterPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->ShieldBar &&
		BlasterHUD->CharacterOverlay->ShieldText;
	
	//��������Ѿ���ʼ��HUD�ˣ�ֱ�ӿ�������
	if (bHUDValid)
	{
		//SetPercent��ʹ�á������ܰٷֱ�
		const float ShieldPercent = Shield / MaxShield;
		BlasterHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);

		//�������ִ�ӡ��FMath::CeilToInt��������ȡ��
		FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));

		//FString HealthText��CharacterOverlay->HealthText���UTextBlock* HealthText������HealthText���Ǻ��岻ͬ��
		BlasterHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));	//SetText��Ҫ����FText������Ҫ��FStringת����FText
	}

	//���滹û�г�ʼ��HUD��ʱ���Ȱѱ����洢�������ȵ�PollInit�����µ��������
	else
	{
		//���������Ƿ�Ҫ��ʼ����Bool
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
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
	
	//��������Ѿ���ʼ��HUD�ˣ�ֱ�ӿ�������
	if (bHUDValid)
	{
		//����ȡ��
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		BlasterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}

	//���滹û�г�ʼ��HUD��ʱ���Ȱѱ����洢�������ȵ�PollInit�����µ��������
	else
	{
		//���������Ƿ�Ҫ��ʼ����Bool
		bInitializeScore = true;
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

	//��������Ѿ���ʼ��HUD�ˣ�ֱ�ӿ�������
	if (bHUDValid)
	{
		//
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		BlasterHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	
	//���滹û�г�ʼ��HUD��ʱ���Ȱѱ����洢�������ȵ�PollInit�����µ��������
	else
	{
		//���������Ƿ�Ҫ��ʼ����Bool
		bInitializeScore = true;
		HUDDefeats = Defeats;
	}
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount;
	
	//���滹û�г�ʼ��HUD��ʱ���Ȱѱ����洢�������ȵ�PollInit�����µ��������
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		//���������Ƿ�Ҫ��ʼ����Bool
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
	}
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount;
	
	//���滹û�г�ʼ��HUD��ʱ���Ȱѱ����洢�������ȵ�PollInit�����µ��������
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		//���������Ƿ�Ҫ��ʼ����Bool
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = Ammo;
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
		//״̬���л���ʱ��ʱ����ж��ݸ�����ʱ�򣬶�������
		if (CountdownTime < 0.f)
		{
			BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}

		//���÷ֺ���
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;
		
		//�ַ���ӡ(%02d:�����ָ�ʽ��Ϊ�������֣�������Ϊ0)
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
		//״̬���л���ʱ��ʱ����ж��ݸ�����ʱ�򣬶�������
		if (CountdownTime < 0.f)
		{
			BlasterHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}
		
		//���÷ֺ���
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		//�ַ���ӡ(%02d:�����ָ�ʽ��Ϊ�������֣�������Ϊ0)
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BlasterHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void ABlasterPlayerController::SetHUDGrenades(int32 Grenades)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->GrenadesText;

	if (bHUDValid)
	{
		FString GrenadesText = FString::Printf(TEXT("%d"), Grenades);
		BlasterHUD->CharacterOverlay->GrenadesText->SetText(FText::FromString(GrenadesText));
	}

	//���HUD��û�е���ʼ����˳���Ȱѱ�������������������Getter��ȡ
	else
	{
		bInitializeGrenades = true;
		HUDGrenades = Grenades;
	}
}

void ABlasterPlayerController::SetHUDTime()
{
	//��ȡ�ؿ�����ʱ��
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
	��������׶ε�ʱ��
	*/
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::Cooldown) TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;

	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);
	/*
	END OF ��������׶ε�ʱ��
	*/

	//HUD�����ø����׶ε�ʱ��
	if (CountdownInt != SecondsLeft)
	{
		//����ʱ���ȴ��׶κͽ�������׶ι���
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}

		//���ڽ׶�
		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}

	//��ʾ���ָ�����ϣ�ʣ��ʱ������Խ�����һ��ıȽ�
	CountdownInt = SecondsLeft;
}

//ÿ֡���á�����HUD����
void ABlasterPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (BlasterHUD && BlasterHUD->CharacterOverlay)
		{
			//���������ܡ���������������
			CharacterOverlay = BlasterHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				//���ж��Ƿ�����ǰ�洢���еĻ�������
				if (bInitializeHealth) SetHUDHealth(HUDHealth, HUDMaxHealth);
				if (bInitializeShield) SetHUDShield(HUDShield, HUDMaxShield);
				if (bInitializeScore) SetHUDScore(HUDScore);
				if (bInitializeDefeats) SetHUDDefeats(HUDDefeats);
				if (bInitializeCarriedAmmo) SetHUDCarriedAmmo(HUDCarriedAmmo);
				if (bInitializeWeaponAmmo) SetHUDWeaponAmmo(HUDWeaponAmmo);
			}

			//������������
			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
			if (BlasterCharacter && BlasterCharacter->GetCombat())
			{
				//���ж��Ƿ�����ǰ�洢���еĻ�������
				if (bInitializeGrenades) SetHUDGrenades(BlasterCharacter->GetCombat()->GetGrenades());
			}
		}
	}
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	//�ڷ������ϣ���������õ����Լ��ĵ�ǰʱ��
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();

	//ͨ��Client RPC���ð�������ݻ���
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
	//������� ������ʱ��
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();

	//�ͻ����� ������ʱ��+����
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
		//չʾ����HUD
		//BlasterHUD->AddCharacterOverlay();
		if (BlasterHUD->CharacterOverlay == nullptr) BlasterHUD->AddCharacterOverlay();
		
		//����Announcement
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
		//���ؾ���HUD
		BlasterHUD->CharacterOverlay->RemoveFromParent();
		
		bool bHUDValid = BlasterHUD->Announcement &&
			BlasterHUD->Announcement->AnnouncementText &&
			BlasterHUD->Announcement->InfoText;

		//չʾAnnouncement
		if (bHUDValid)
		{
			//��̬Announcement��HUD��ʾ����
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);

			//HUD���ݸ���
			FString AnnouncementText("New Match Starts In:");
			BlasterHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));
			
			/*
			��ʾ��ʤ��
			*/
			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			
			ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();	//�������
			
			if (BlasterGameState && BlasterPlayerState)
			{
				//��û�ʤ��
				TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScoringPlayers;

				//���ش洢��ʤ�ߣ����ں�����ӡ
				FString InfoTextString;
				
				/*
				�жϻ�ʤ�ߣ�ȡ��������������ж������
				*/
				//û�˻�ʤ
				if (TopPlayers.Num() == 0)
				{
					InfoTextString = FString("There is no winner.");
				}

				//һ��������һ�ʤ
				else if (TopPlayers.Num() == 1 && TopPlayers[0] == BlasterPlayerState)
				{
					InfoTextString = FString("You are the winner!");
				}

				//һ���Ǳ�����һ�ʤ
				else if (TopPlayers.Num() == 1)
				{
					//GetPlayerName()����FString����Ҫ�Ǻ�*
					InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerName());
				}
				
				//������
				else if (TopPlayers.Num() > 1)
				{
					InfoTextString = FString("Players tied for the win:\n");
					for (auto TiedPlayer : TopPlayers)	//ÿ�ָ�ֵ��TiedPlayer
					{
						//Append����β�����
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
					}
				}

				//��ӡ��ʤ��
				BlasterHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
			}
			/*
			END OF ��ʾ��ʤ��
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
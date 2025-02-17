// Copyright Epic Games, Inc. All Rights Reserved.

#include "TP_SideScrollerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Weapons/TP_WeaponBase.h"
#include "TP_AttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"

void ATP_SideScrollerCharacter::PlayerDeath_Implementation()
{
	
}

ATP_SideScrollerCharacter::ATP_SideScrollerCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Create a camera boom attached to the root (capsule)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Rotation of the character should not affect rotation of boom
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->TargetArmLength = 500.f;
	CameraBoom->SocketOffset = FVector(0.f,0.f,75.f);
	CameraBoom->SetRelativeRotation(FRotator(0.f,180.f,0.f));

	// Create a camera and attach to boom
	SideViewCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	SideViewCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	SideViewCameraComponent->bUsePawnControlRotation = false; // We don't want the controller rotating the camera

	//AmmoAttributeSet
	AmmoAttributeSet = CreateDefaultSubobject<UTP_AmmoAttributeSet>(TEXT("AmmoAttributeSet"));

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Face in the direction we are moving..
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->GravityScale = 2.f;
	GetCharacterMovement()->AirControl = 0.80f;
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->GroundFriction = 3.f;
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->MaxFlySpeed = 600.f;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

ATP_WeaponBase* ATP_SideScrollerCharacter::GetCurrentWeapon() const
{
	return CurrentWeapon;
}

void ATP_SideScrollerCharacter::NextWeapon()
{
	if (Weapons.Num() < 2)
	{
		return;
	}

	int32 CurrentWeaponIndex = Weapons.Find(CurrentWeapon);
	CurrentWeapon->Unequip();

	if (CurrentWeaponIndex == INDEX_NONE)
	{
		CurrentWeapon=Weapons[0];
		CurrentWeaponTag = CurrentWeapon->WeaponTag;
		CurrentWeapon->Equip();
	}
	else
	{
		CurrentWeapon=Weapons[(CurrentWeaponIndex + 1) % Weapons.Num()];
		CurrentWeaponTag = CurrentWeapon->WeaponTag;
		CurrentWeapon->Equip();
	}
}

void ATP_SideScrollerCharacter::PreviousWeapon()
{
	if (Weapons.Num() < 2)
	{
		return;
	}

	int32 CurrentWeaponIndex = Weapons.Find(CurrentWeapon);
	CurrentWeapon->Unequip();

	if (CurrentWeaponIndex == INDEX_NONE)
	{
		CurrentWeapon=Weapons[0];
		CurrentWeaponTag = CurrentWeapon->WeaponTag;
		CurrentWeapon->Equip();
	}
	else
	{
		int32 IndexOfPrevWeapon = FMath::Abs(CurrentWeaponIndex - 1 + Weapons.Num()) % Weapons.Num();
		CurrentWeapon=Weapons[IndexOfPrevWeapon];
		CurrentWeaponTag = CurrentWeapon->WeaponTag;
		CurrentWeapon->Equip();
	}
}

void ATP_SideScrollerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		// Attribute change callbacks
		HealthChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).AddUObject(this, &ATP_SideScrollerCharacter::HealthChanged);
	}

	InitializeWeapons();
	
}
void ATP_SideScrollerCharacter::HealthChanged(const FOnAttributeChangeData& Data) {
	if (Data.NewValue <= 0)
		PlayerDeath();
}

void ATP_SideScrollerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	AbilitySystemComponent->BindAbilityActivationToInputComponent(InputComponent, FGameplayAbilityInputBinds("ConfirmInput", "CancelInput", "ETP_AbilityInputID"));
}

bool ATP_SideScrollerCharacter::IsAlive() const
{
	return AttributeSet->GetHealth() > 0.0f;
}


FName ATP_SideScrollerCharacter::GetWeaponAttachPoint()
{
	return WeaponAttachPoint;
}

void ATP_SideScrollerCharacter::InitializeWeapons()
{
	for (TSubclassOf<ATP_WeaponBase>& WeaponClass: WeaponClasses)
	{
		FVector Location(0.0f, 0.0f, 0.0f);
		FRotator Rotation(0.0f, 0.0f, 0.0f);
		FActorSpawnParameters SpawnInfo;

		ATP_WeaponBase* ThisWeapon = GetWorld()->SpawnActor<ATP_WeaponBase>(WeaponClass, Location, Rotation, SpawnInfo);
		Weapons.Add(ThisWeapon);
		ThisWeapon->SetOwningCharacter(this);
		//last weapon in list will be equipped after init
		if(CurrentWeapon)
		{
			CurrentWeapon->Unequip();
		}
		CurrentWeapon = ThisWeapon;
		CurrentWeapon->Equip();
		
	}
}


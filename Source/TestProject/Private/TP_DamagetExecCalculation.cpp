// Fill out your copyright notice in the Description page of Project Settings.


#include "TP_DamagetExecCalculation.h"
#include "AbilitySystemComponent.h"
#include "TP_AttributeSet.h"


struct TP_DamageStatics
{
	//The DECLARE_ATTRIBUTE_CAPTUREDEF macro actually only declares two variables. The variable names are dependent on the input, however. Here they will be HealthProperty(which is a UPROPERTY pointer)
	//and HealthDef(which is a FGameplayEffectAttributeCaptureDefinition).
	DECLARE_ATTRIBUTE_CAPTUREDEF(Health);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Armor);

	TP_DamageStatics()
	{
		// We define the values of the variables we declared now. In this example, HealthProperty will point to the Health attribute in the UMyAttributeSet on the receiving target of this execution. The last parameter is a bool, and determines if we snapshot the attribute's value at the time of definition.
		DEFINE_ATTRIBUTE_CAPTUREDEF(UTP_AttributeSet, Health, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UTP_AttributeSet, Armor, Target, false);

	}
};

static const TP_DamageStatics& DamageStatics()
{
	static TP_DamageStatics DStatics;
	return DStatics;
}

UTP_DamagetExecCalculation::UTP_DamagetExecCalculation()
{
	//RelevantAttributesToCapture - ��� ������, ���������� ��� ��������, ������� �� ������ ���������, ��� ����������.
    //��������� �����, ����� � AttemptCalculateCapturedAttributeMagnitude() ������ �� ������ ��������� ������ ���������
	RelevantAttributesToCapture.Add(DamageStatics().HealthDef);
	/*InvalidScopedModifierAttributes.Add(DamageStatics().HealthDef);*/

	RelevantAttributesToCapture.Add(DamageStatics().ArmorDef);
	/*InvalidScopedModifierAttributes.Add(DamageStatics().ArmorDef);*/

}

void UTP_DamagetExecCalculation::Execute_Implementation(const FGameplayEffectCustomExecutionParameters & ExecutionParams, OUT FGameplayEffectCustomExecutionOutput & OutExecutionOutput) const
{
	// �� �������� ���������� AbilitySystemComponents � ��������� ��������������� ����������. �� �����������, �� ��� �������� ��� �� �������� ��� �����.
	UAbilitySystemComponent* TargetAbilitySystemComponent = ExecutionParams.GetTargetAbilitySystemComponent();
	UAbilitySystemComponent* SourceAbilitySystemComponent = ExecutionParams.GetSourceAbilitySystemComponent();
	
	//
	AActor* SourceActor = SourceAbilitySystemComponent ? SourceAbilitySystemComponent->AvatarActor : nullptr;
	AActor* TargetActor = TargetAbilitySystemComponent ? TargetAbilitySystemComponent->AvatarActor : nullptr;

	//�������� ������������ ������� ������� ������� ���� ������
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// Gather the tags from the source and target as that can affect which buffs should be used
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	// �� ���������� ��� ���� ��� �������� ��������� FAggregatorEvaluateParameters,
	//������� ��� ����������� ��� ��������� �������� ����� ����������� ��������� ����� � ���� �������.
	FAggregatorEvaluateParameters EvaluationParameters;
	//EvaluationParameters.SourceTags = SourceTags;
	//EvaluationParameters.TargetTags = TargetTags;

	float Health = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().HealthDef, EvaluationParameters, Health);
	Health = FMath::Max<float>(Health, 0.0f);


	float Armor = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ArmorDef, EvaluationParameters, Armor);
	Armor = FMath::Max<float>(Armor, 0.0f);

	float Damage = 0.0f;
	// Capture optional damage value set on the damage GE as a CalculationModifier under the ExecutionCalculation
	// ������, ����� �� �������� ����������� �������� �������� � ���� �������. Damage ().
	//DamageDef - ��� ����������� ��������, ������� �� ����� ��������, �� ����������
	//EvaluationParameters ���� ���� ���
	//� Damage - ��� ����������, � ������� �� �������� ���������� �������� (���������� Damage, ������� �� ������ ��� ��������)
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DamageDef, EvaluationParameters, Damage);
	Damage += FMath::Max<float>(Spec.GetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage")), false, -1.0f), 0.0f);

	float UnmitigatedDamage = Damage; // Can multiply any damage boosters here
	float OldArmor = Armor;
	Armor = FMath::Clamp (Armor-Damage,0.0f, Armor);
	float MitigatedDamage = UnmitigatedDamage - (OldArmor - Armor) * 0.5f;

	if (MitigatedDamage > 0.f)
	{
		// Set the Target's damage meta attribute
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(DamageStatics().HealthProperty, EGameplayModOp::Additive, -MitigatedDamage));
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(DamageStatics().ArmorProperty, EGameplayModOp::Additive, -UnmitigatedDamage));
	}

}

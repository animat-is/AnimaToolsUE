// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AdjustmentBlendFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class ANIMATOOLS_API UAdjustmentBlendFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable, Category = AnimEditing)
	static void ApplyAdjustmentBlendControlRig(UMovieSceneControlRigParameterTrack* ControlRigTrack);

	UFUNCTION(BlueprintCallable, Category = SequencerEditing)
	static void ApplyAdjustmentBlendToSingleControl(UMovieSceneControlRigParameterTrack* ControlRigTrack, FName ControlName);
};

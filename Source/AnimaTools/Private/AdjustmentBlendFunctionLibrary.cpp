#include "AdjustmentBlendFunctionLibrary.h"
#include "ControlRig.h"
#include "ScopedTransaction.h"
#include "Sequencer/ControlRigSequencerHelpers.h"
#include "Sequencer/MovieSceneControlRigParameterTrack.h"


void UAdjustmentBlendFunctionLibrary::ApplyAdjustmentBlendControlRig(UMovieSceneControlRigParameterTrack* ControlRigTrack)
{
	if(!ControlRigTrack)
	{
		UE_LOG(LogAnimation, Error, TEXT("Invalid control rig track when running adjustment blend"));
		return;
	}
	const UControlRig* ControlRig = ControlRigTrack->GetControlRig();
	TArray<FRigControlElement*> Controls;
	ControlRig->GetControlsInOrder(Controls);
	
	FScopedTransaction Transaction(FText::FromString("Apply Adjustment Blend"));
	ControlRigTrack->Modify(true);
	for (const FRigControlElement* Control : Controls)
	{
		ApplyAdjustmentBlendToSingleControl(ControlRigTrack, Control->GetFName());
	}
}

void UAdjustmentBlendFunctionLibrary::ApplyAdjustmentBlendToSingleControl(UMovieSceneControlRigParameterTrack* ControlRigTrack, FName ControlName)
{
	if(!ControlRigTrack || ControlName.IsNone())
	{
		UE_LOG(LogAnimation, Error, TEXT("Invalid control or rig track when running adjustment blend"));
		return;
	}
	TArray<UMovieSceneSection*> Sections = ControlRigTrack->GetAllSections();
	if (Sections.Num() < 2)
	{
		UE_LOG(LogAnimation, Error, TEXT("Adjustment blend failed, at least 2 layers (base + additive) required."));
		return; // We need at least one base and one additive layer
	}

	const UControlRig* ControlRig = ControlRigTrack->GetControlRig();

	// Identify the Target Additive Section 
	UMovieSceneControlRigParameterSection* TargetAdditiveSection = nullptr;
	int32 LowestAdditiveRow = 0;

	for (UMovieSceneSection* Section : Sections)
	{
		if (Section->IsActive() && Section->GetBlendType() == EMovieSceneBlendType::Additive)
		{
			if (Section->GetRowIndex() > LowestAdditiveRow)
			{
				LowestAdditiveRow = Section->GetRowIndex();
				TargetAdditiveSection = Cast<UMovieSceneControlRigParameterSection>(Section);
			}
		}
	}

	if (!TargetAdditiveSection)
	{
		UE_LOG(LogAnimation, Error, TEXT("Adjustment blend failed: Could not find an active Additive layer."));
		return;
	}

	// SNAPSHOT TARGET FOR UNDO
	TargetAdditiveSection->Modify();

	// Get Timing information
	UMovieScene* MovieScene = ControlRigTrack->GetTypedOuter<UMovieScene>();
	FFrameRate TickResolution = MovieScene->GetTickResolution();
	FFrameRate DisplayRate = MovieScene->GetDisplayRate();
	int32 TickStep = (TickResolution.Numerator * DisplayRate.Denominator) / (TickResolution.Denominator * DisplayRate.Numerator);
	if (TickStep <= 0) TickStep = 1;

	// HELPER LAMBDA: Evaluates the "True Base" by combining all other layers at a specific frame
	auto EvaluateCombinedBaseMotion = [&](FFrameNumber Time, int32 ChannelIndex) -> float
	{
		float AbsoluteValue = 0.0f;
		float AdditiveSum = 0.0f;

		for (UMovieSceneSection* Section : Sections)
		{
			// Skip our target layer, and skip muted layers
			if (Section == TargetAdditiveSection || !Section->IsActive()) continue;

			// Skip if the timeline hasn't reached this section yet
			if (!Section->GetRange().Contains(Time)) continue;

			const UMovieSceneControlRigParameterSection* CRSection = Cast<UMovieSceneControlRigParameterSection>(Section);
			const TArrayView<FMovieSceneFloatChannel*> Channels = FControlRigSequencerHelpers::GetFloatChannels(ControlRig, ControlName, CRSection);
			
			if (Channels.Num() == 9)
			{
				float Val = 0.0f;
				Channels[ChannelIndex]->Evaluate(Time, Val);

				if (Section->GetBlendType() == EMovieSceneBlendType::Absolute)
				{
					AbsoluteValue = Val; // (Note: If there are overlapping absolutes, Sequencer normally weight-blends them. The value is taken here directly for simplicity).
				}
				else if (Section->GetBlendType() == EMovieSceneBlendType::Additive)
				{
					AdditiveSum += Val;
				}
			}
		}
		
		// The true underlying motion is the Absolute Value + any underlying Additive Values
		return AbsoluteValue + AdditiveSum;
	};

	// Process the Target Additive Channels on target channels (every transform's subtrack)
	const TArrayView<FMovieSceneFloatChannel*> TargetFloatChannels = FControlRigSequencerHelpers::GetFloatChannels(ControlRig, ControlName, TargetAdditiveSection);
	if (TargetFloatChannels.Num() == 9)
	{
		for (int32 i = 0; i < 9; ++i)
		{
			FMovieSceneFloatChannel* TargetChannel = TargetFloatChannels[i];
			TMovieSceneChannelData<FMovieSceneFloatValue> TargetData = TargetChannel->GetData();
			TArrayView<const FFrameNumber> TargetTimes = TargetData.GetTimes();
			TArrayView<const FMovieSceneFloatValue> TargetValues = TargetData.GetValues();

			if (TargetTimes.Num() < 2) continue;

			struct FNewKey { FFrameNumber Time; float Value; };
			TArray<FNewKey> KeysToAdd;

			for (int32 KeyIdx = 0; KeyIdx < TargetTimes.Num() - 1; ++KeyIdx)
			{
				const FFrameNumber StartTime = TargetTimes[KeyIdx];
				const FFrameNumber StopTime = TargetTimes[KeyIdx + 1];
				const float StartValue = TargetValues[KeyIdx].Value;
				const float StopValue = TargetValues[KeyIdx + 1].Value;

				if (StartTime == StopTime) continue;

				struct FFrameEval { FFrameNumber Time; float Value; };
				TArray<FFrameEval> SpanValues;

				// Evaluate the COMBINED Base Motion at each frame
				for (FFrameNumber CurrentTime = StartTime; CurrentTime <= StopTime; CurrentTime += TickStep)
				{
					float CombinedBaseVal = EvaluateCombinedBaseMotion(CurrentTime, i);
					SpanValues.Add({ CurrentTime, CombinedBaseVal });
				}
				if (SpanValues.Last().Time < StopTime)
				{
					float CombinedBaseVal = EvaluateCombinedBaseMotion(StopTime, i);
					SpanValues.Add({ StopTime, CombinedBaseVal });
				}

				// Calculate velocity of combined underlying layers
				TArray<float> ChangeValues;
				ChangeValues.Add(0.0f);
				float TotalBaseLayerChange = 0.0f;

				for (int32 SpanIdx = 0; SpanIdx < SpanValues.Num() - 1; ++SpanIdx)
				{
					float Change = FMath::Abs(SpanValues[SpanIdx + 1].Value - SpanValues[SpanIdx].Value);
					ChangeValues.Add(Change);
					TotalBaseLayerChange += Change;
				}

				float TotalPoseLayerChange = FMath::Abs(StopValue - StartValue);
				float PreviousValue = StartValue;

				// Distribute percentages
				for (int32 SpanIdx = 0; SpanIdx < SpanValues.Num(); ++SpanIdx)
				{
					const FFrameNumber CurrentTime = SpanValues[SpanIdx].Time;
					if (CurrentTime == StartTime) continue; 

					const float Percentage = (TotalBaseLayerChange > KINDA_SMALL_NUMBER) 
						? ((100.0f / TotalBaseLayerChange) * ChangeValues[SpanIdx]) 
						: (100.0f / (SpanValues.Num() - 1));

					const float ValueDelta = (TotalPoseLayerChange / 100.0f) * Percentage;
					const float CurrentValue = PreviousValue + ((StopValue > StartValue) ? ValueDelta : -ValueDelta);

					if (CurrentTime < StopTime)
					{
						KeysToAdd.Add({ CurrentTime, CurrentValue });
					}
					PreviousValue = CurrentValue;
				}
			}

			// Inject the baked sub-frame keys back into the Target channel
			for (const FNewKey& NewKey : KeysToAdd)
			{
				FMovieSceneFloatValue NewValue(NewKey.Value);
				NewValue.InterpMode = ERichCurveInterpMode::RCIM_Linear; 
				TargetData.UpdateOrAddKey(NewKey.Time, NewValue);
			}
		}
	}
}
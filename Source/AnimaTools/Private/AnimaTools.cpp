#include "AnimaTools.h"
#include "AdjustmentBlendFunctionLibrary.h"
#include "ToolMenus.h"
#include "Sequencer/MovieSceneControlRigParameterTrack.h"
#include "SequencerToolMenuContext.h"
#include "ISequencer.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "FAnimaToolsModule"

void FAnimaToolsModule::StartupModule()
{
    // Register the toolbar extension
    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAnimaToolsModule::ExtendSequencerToolbar));  
}

void FAnimaToolsModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this); 
}

void FAnimaToolsModule::ExtendSequencerToolbar()
{
    UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("Sequencer.MainToolBar");
    if (!ToolbarMenu) return;

    // Add a UI section to the toolbar
    FToolMenuSection& Section = ToolbarMenu->AddSection("AnimaToolsSection");

    // Add a SubMenu (This renders as a Dropdown Button on toolbars)
    Section.AddSubMenu(
        "AnimaToolsDropdown",
        LOCTEXT("AnimaToolsLabel", "AnimaTools"),
        LOCTEXT("AnimaToolsTooltip", "Custom animation and rigging tools"),
        FNewToolMenuDelegate::CreateLambda([](UToolMenu* SubMenu)
        {
            FToolMenuSection& DropdownSection = SubMenu->AddSection("ControlRigTools", LOCTEXT("CRToolsHeading", "Control Rig Tools"));

        	DropdownSection.AddMenuEntry(
				"ApplyAdjustmentBlend",
				LOCTEXT("ApplyAdjBlendLabel", "Apply Adjustment Blend"),
				LOCTEXT("ApplyAdjBlendTooltip", "Runs Adjustment Blend on the top additive layer of the SELECTED Control Rig track."),
				FSlateIcon(), // TODO: Icon
				FToolUIActionChoice(FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext& InContext)
				{
					const USequencerToolMenuContext* SeqContext = InContext.FindContext<USequencerToolMenuContext>();
					if (!SeqContext) return;

					const TSharedPtr<ISequencer> Sequencer = SeqContext->WeakSequencer.Pin();
					if (!Sequencer.IsValid()) return;

					TArray<UMovieSceneTrack*> SelectedTracks;
					Sequencer->GetSelectedTracks(SelectedTracks);

					if (SelectedTracks.Num() == 0)
					{
						FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoRigTrackSelectedMessage", "Please select at least one control rig track in Sequencer first."));
						return;
					}
					
					for (UMovieSceneTrack* Track : SelectedTracks)
					{
						if (UMovieSceneControlRigParameterTrack* CRTrack = Cast<UMovieSceneControlRigParameterTrack>(Track))
						{
							// Run your script on the selected track
							UAdjustmentBlendFunctionLibrary::ApplyAdjustmentBlendControlRig(CRTrack);
						}
					}
				}))
			);
        }),
        false,
        FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Adjust")
    );
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FAnimaToolsModule, AnimaTools)
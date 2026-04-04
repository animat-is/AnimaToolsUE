using UnrealBuildTool;

public class AnimaTools : ModuleRules
{
    public AnimaTools(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "EditorFramework",
                "IKRig",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Slate",
                "SlateCore",
                "EditorSubsystem",
                "RigVM",
                "UnrealEd",
#if WITH_EDITOR
                "Persona",
#endif
                "ControlRig",
                "ToolMenus",
                "Sequencer",
                "AnimationModifiers",
				"AnimationModifierLibrary", 
				"AnimationBlueprintLibrary",
				"AnimationCore",
                "AnimGraphRuntime",
                "AnimGraph",
                "SignalProcessing",
                "BlueprintGraph",
                "MovieScene",
                "MovieSceneTracks"
            }
        );
    }
}
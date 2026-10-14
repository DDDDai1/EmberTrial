using UnrealBuildTool;

public class EmberTrialEditor : ModuleRules
{
    public EmberTrialEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "DataValidation", "AssetRegistry"
        });
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "AIModule", "UMG", "UMGEditor", "UnrealEd", "EmberTrial",
            "GameplayAbilities", "GameplayTags", "EnhancedInput", "InputCore"
        });
    }
}
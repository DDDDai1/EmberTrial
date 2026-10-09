using UnrealBuildTool;

public class EmberTrial : ModuleRules
{
    public EmberTrial(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
            "GameplayAbilities", "GameplayTags", "AIModule"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "GameplayTasks", "PhysicsCore"
        });

        PrivateIncludePaths.Add(ModuleDirectory);
    }
}
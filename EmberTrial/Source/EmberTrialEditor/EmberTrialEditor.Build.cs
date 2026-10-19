using UnrealBuildTool;

public class EmberTrialEditor : ModuleRules
{
    public EmberTrialEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // 公开Validator基类 / FAssetData签名需要DataValidation与AssetRegistry。
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "DataValidation", "AssetRegistry"
        });
        // 只在Editor目标使用的实现依赖；不向Game目标或Runtime Build.cs传播。
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "AIModule", "UMG", "UMGEditor", "UnrealEd", "EmberTrial",
            "GameplayAbilities", "GameplayTags", "EnhancedInput", "InputCore"
        });
    }
}

using UnrealBuildTool;

public class EmberTrialUI : ModuleRules
{
    public EmberTrialUI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // 原生Widget公开头继承UMG类型；公开依赖供消费该模块头文件的代码使用。
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UMG",
            "Slate",
            "SlateCore",
            "EmberTrial"
        });

        // 公开UI委托使用Core槽位 / 回复类型，Core已列为公开依赖；Core仍不能反向依赖UI。
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "GameplayAbilities",
            "GameplayTags",
            "InputCore"
        });
    }
}

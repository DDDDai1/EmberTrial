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
            "UMG"
        });

        // UI通过私有Core依赖读取玩法接口；Slate / Input只服务实现，Core不能反向依赖UI。
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "EmberTrial",
            "GameplayAbilities",
            "GameplayTags",
            "InputCore",
            "Slate",
            "SlateCore"
        });
    }
}

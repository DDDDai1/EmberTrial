using UnrealBuildTool;

public class EmberTrial : ModuleRules
{
    public EmberTrial(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // 公开头导出ASC / Tag / 输入与AI类型；保持现有Public依赖，不引入UI / Editor。
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
            "GameplayAbilities", "GameplayTags", "AIModule"
        });

        // Gameplay任务 / 物理只在实现中使用；旧Practice头通过ModuleDirectory私有路径解析。
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "GameplayTasks", "PhysicsCore"
        });

        PrivateIncludePaths.Add(ModuleDirectory);
    }
}

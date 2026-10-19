// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

// Runtime目标保留Core / UI，不把Editor校验器或编辑器依赖带入游戏。
public class EmberTrialTarget : TargetRules
{
    public EmberTrialTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        // 固定本项目UE5.5工具链约定，不跟随IDE的CMake配置改变。
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
        ExtraModuleNames.AddRange(new string[] { "EmberTrial", "EmberTrialUI" });
    }
}

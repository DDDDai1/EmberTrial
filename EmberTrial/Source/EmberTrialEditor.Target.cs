// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

// Editor目标才包含工具模块；Game目标只保留两个Runtime模块。
public class EmberTrialEditorTarget : TargetRules
{
    public EmberTrialEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        // 固定本项目UE5.5工具链约定，不跟随IDE的CMake配置改变。
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
        ExtraModuleNames.AddRange(new string[] { "EmberTrial", "EmberTrialUI", "EmberTrialEditor" });
    }
}

# M0-05A 第一段攻击占位验收

日期：2026-09-21。结论：**M0-05A 完成，用户手动用时约 80 分钟。进入 M0-05B，M0-05 总任务及 M0 尚未完成。**

## 用户编辑器验收结果

| 检查 | 结果 |
|---|---|
| 玩家 Mesh / Skeleton | SKM_Quinn_Simple / SK_Mannequin |
| 当前 Anim Class | ABP_Quinn_C |
| Control Rig 手臂控制 | 正常 |
| LS_M0_Attack_01 五帧预览 | 正常；此前已确认五帧 key、两个 FK/IK=False、Linear 与连续播放 |
| AS_M0_Attack_01 独立播放 | 正常 |
| 动画 Skeleton / 时长 | SK_Mannequin，约 1 秒 |
| 固定参考下 root 漂移 | 无 |
| 地图 / Sequence / 动画 | 已保存 |
| 回 ThirdPersonMap 的基础输入 / E / P | 正常 |

用户原文把 `ABP_Quinn_C` 填在“Anim Class / AnimBP Parent”合并字段中。它作为 Anim Class 正确；父类由下方只读资产检查补齐，没有把它误记为父类。

80 分钟记为用户报告的本批手动用时，不把它当作包括助手数值校准、工具运行等在内的总墙钟时间。已有 M0-04 手动记录为 190 分钟，两批合计 270 分钟，仍不代表整个 Demo 总投入。

## 只读资产核对

在独立 UE 5.5.4 命令行编辑器中加载已保存资产，通过 UE API 读取默认角色组件、资产注册信息和动画采样元数据；未修改或保存 UE 资产。

| 项目 | 直接读取结果 |
|---|---|
| BP_ThirdPersonCharacter 默认 Mesh | `/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple` |
| Mesh 的 Skeleton | `/Game/Characters/Mannequins/Meshes/SK_Mannequin` |
| Animation Mode | Animation Blueprint |
| Anim Class | `/Game/Characters/Mannequins/Animations/ABP_Quinn.ABP_Quinn_C` |
| ABP_Quinn 的 ParentClass 资产标记 | `/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C` |
| 攻击动画 Skeleton | 与玩家 Mesh 相同的 SK_Mannequin 资产 |
| LS Display Rate / Playback Range | 30/1 fps；Start=0，End=31 |
| AS 动画采样数 | GetNumFrames=31；GetNumKeys=32 |
| AS 时长 | 1.0333333 秒 |
| Enable Root Motion | False |
| root 变换采样 | 0 至动画结束的 31 个等间隔时间采样中，Location=(0,0,0)、Rotation≈0、Scale=(1,1,1)；不提取 Root Motion |

本地 UE 5.5 源码 `AnimationBlueprintLibrary.cpp` 的 GetNumFrames 返回“采样点数减一”。因此这里分别记录 **31 个帧间隔 / 32 个采样点**，不把用户填写的 Playback Range 直接当成动画采样点数。31/30≈1.033 秒符合当前设置，不要求返工成精确 1.000 秒。

玩家与攻击动画使用同一 Skeleton，后续可以先在当前骨架上测试 Montage 接入。本批没有完成 Montage / Slot、攻击输入、命中或伤害系统；ABP_Quinn 继承 ABP_Manny 的关系将在 C 阶段用于定位实际 AnimGraph。

脚本：[inspect_m0_animation_assets.py](../../Scripts/Editor/inspect_m0_animation_assets.py)。最终检查进程返回 0，日志报告 0 errors / 0 warnings。原始 JSON 与日志在本机 `Docs/Reports/Raw/M0_05A_AssetInspection.*`，按仓库规则不跟踪。

## 落盘与运行日志

| 文件 | 大小（字节） | 本地修改时间 |
|---|---:|---|
| Content/EmberTrial/Maps/L_M0_AnimationLab.umap | 50,785 | 2026-09-20 23:37:52 |
| Content/EmberTrial/Animations/Practice/Sequence/LS_M0_Attack_01.uasset | 173,557 | 2026-09-21 21:26:53 |
| Content/EmberTrial/Animations/Practice/Clips/AS_M0_Attack_01.uasset | 258,461 | 2026-09-21 21:30:28 |

实际素材源目录为单数 `Sequence`，后续继续沿用。

本轮读取的 `EmberTrial/Saved/Logs/EmberTrial.log` 记录了 13:28 UTC 的动画压缩生成、13:30 UTC 的正式保存、13:34 UTC 的再次打开。未匹配 Blueprint Runtime Error、Accessed None、LogScript: Error、LogBlueprint: Error 或 Fatal error。基础输入回归以用户报告为证，没有把素材编辑器日志当作 PIE 功能验证。

日志另有一条 GPU Profiler scratch buffer 空间不足并丢弃该帧分析数据的 LogRHI Error。已记录，不能声称整份编辑器日志零错误；当前用户未报告对应功能异常，本轮未据此改渲染配置。另有预览组件已注册、ApexDestruction 类未找到等消息，未将它们当作攻击动画损坏的证据。

## 后续

继续 [M0-05B：原地闪避／受击占位](../M0_05B_DefensiveAnimations.md)。仍由用户完成基础建资产、填数值和打 key；复杂 Rig / 导出问题由助手接手。B 的参数已完成独立 Rig 求值不等于两段动画已制作或验收。

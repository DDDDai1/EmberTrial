# UE 5.4.4 → 5.5.4 基线校准

迁移执行：2026-09-18；记录更新：2026-09-19。状态：版本配置、解决方案生成与 UE 5.5.4 C++ 构建全部完成，等待用户交互运行复验。

## 已确认

- 用户选择 UE 5.5.4，已保存并关闭当前 UE 与 Visual Studio。
- 原工程仅作为 5.4.4 已验证基线；优先保存源码、配置和二进制资产的本地 Git / LFS 检查点。
- 不删除原工程，不重建用户已创建的角色 / GameMode，不修改玩法实现。

## 计划改动

1. `.uproject` 的 EngineAssociation 从 `5.4` 改为 `5.5`。
2. Game / Editor Target 的 IncludeOrderVersion 改为 Unreal5_5；保留 V5 与原模块名称。
3. 使用 `D:\UE5\UE_5.5` 的工具重新生成解决方案，并构建 EmberTrialEditor / Win64 / Development。
4. 检查构建结果及 receipt，确认输出来自 5.5.4。
5. 用户用 5.5.4 再次验证打开工程和移动 / 转向 / 跳跃；通过后才完成目标版本运行验收。

## 结果

- 迁移前检查点：`3562f61`，Git / LFS；212 个 UE 资产的 LFS 完整性检查通过。
- 配置修改：EngineAssociation 已改为 `5.5`；两个 Target 的 IncludeOrderVersion 已改为 Unreal5_5。DefaultBuildSettings 仍为 V5，玩法代码未改动。
- 5.5.4 解决方案生成：成功，退出码 0，用时约 48 秒；日志在 `EmberTrial/Saved/Logs/M0_GenerateProjectFiles_UE55.log`。
- 5.5.4 C++ 构建：成功，退出码 0；UHT、编译、链接和 receipt 写入完成，共 12 个 action，用时 59.18 秒。本次最多两个并行任务，未修改全局编译配置。
- 实际编译器：MSVC 14.38.33145，可执行工具链位于安装目录 `14.38.33130`；Windows SDK 为 `10.0.22621.0`。
- 产物复核：EmberTrialEditor.target 为 5.5.4 / Win64 / Development / Editor；`.uproject` 关联 5.5，重新生成的 vcxproj 引用 UE_5.5，不再引用 UE_5.4。
- 生成器同步更新 `.vsconfig` 的 Unreal Workspace、ATL 与 Windows 11 SDK 组件建议；没有自动安装或卸载这些组件。
- Gameplay C++ 实现和 Content 资产本次未修改、未批量重保存。
- 5.5.4 交互运行：待用户复验。
- 下一步基础 Actor / 蓝图练习：目标版本复验通过后再下发。

## 日志与复核范围

- 构建日志：`E:\UE_Project\Demo\EmberTrial\Saved\Logs\M0_Build_UE55.log`。
- 生成日志：`E:\UE_Project\Demo\EmberTrial\Saved\Logs\M0_GenerateProjectFiles_UE55.log`。
- 两份日志另存至 `E:\UE_Project\Demo\Artifacts\M0`，避免常规运行记录被覆盖；原始日志目录已被 Git 忽略。
- 本次新增差异通过 `git diff --check`；迁移前模板既有的尾随空白未批量清理。
- Git LFS 完整性已检查；原基线 212 个资源对象均可用。未配置远端或上传资产。
- 仅凭构建成功不能断言蓝图、地图与输入在新版本下全部正常；仍保留下面的手动验收。

## 构建完成后的用户手动验证

1. 双击 `E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject`，确认打开的是 UE 5.5.4；可在“帮助 / Help → 关于 Unreal Editor”查看版本。
2. 等待首次着色器 / 资源处理完成，在 ThirdPersonMap 点击 Play，测试 WASD、鼠标转向和空格跳跃。
3. 停止后再运行一次，确认行为一致，然后保存工程。
4. 回复“5.5.4 复验正常”，或给出异常现象 / 时间，我读取最新日志继续检查。

构建已完成，现在可以打开工程。此后使用 5.5.4；不要用 5.4 直接保存已经由新版本修改的资产。若需要回退，我基于迁移前检查点处理，不通过只改 EngineAssociation 猜测兼容性。

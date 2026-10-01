# VS 中找不到 Tests：目录查看与项目清单同步

日期：2026-09-29。适用本机 VS 2022 **17.14.20**、UE **5.5.4**。本页是手动操作指南；助手未清理缓存、重生成解决方案、移动工程或改动 C++。

## 1. 本次检查结论

目录名是 **Tests**，不是 Texts。磁盘上已有：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Tests\ETPracticeActionTests.cpp
```

当前 `Intermediate/ProjectFiles/EmberTrial.vcxproj` 和 `.vcxproj.filters` 的修改时间仍为 9 月 27 日，均没有旧测试文件的条目。它们生成后才新增了测试，所以 VS 解决方案树没有显示这个目录。不是源码丢失，也没有证据表明缓存损坏。

传统 C++ 解决方案视图根据项目清单和 filters 显示逻辑分组，不保证实时等于磁盘目录。[微软：vcxproj.filters](https://learn.microsoft.com/en-us/cpp/build/reference/vcxproj-filters-files?view=msvc-170)

之前 Lifecycle 已经能够编译并运行，和“树里没有它”并不矛盾：UBT 根据模块 / Target 查找源码，不依赖 VS 清单决定所有编译输入。[Epic：项目文件与 UBT](https://dev.epicgames.com/documentation/en-us/unreal-engine/how-to-generate-unreal-engine-project-files-for-your-ide?application_version=5.5)

## 2. 现在先用文件夹视图查看，不重生成

1. 保持当前 `EmberTrial.sln` 打开。按 **Ctrl+Alt+L** 打开解决方案资源管理器。
2. 清空该面板的搜索框，取消“仅显示打开文件 / 待处理更改”等筛选（若已启用）。
3. 在面板顶部找到 **切换视图 / Switch Views** 按钮或下拉菜单，选择 **文件夹视图 / Folder View**。鼠标悬停看工具提示，不用猜图标。
4. 从解决方案所在目录展开 **Source → EmberTrial → Tests**。如果根节点是 Demo，先展开其中的 EmberTrial，再进入 Source。
5. 打开 `ETPracticeActionTests.cpp`。后续查看外部新增文件也先用这个视图；若目录尚未重绘，折叠再展开，或切换视图后再回来。
6. 继续原有构建操作前，可切回 **解决方案视图 / Solution View**，使用已有 Development Editor / Win64 配置。

微软明确区分这两种视图；`Show All Files` 是另一项显示功能，不等于自动把外部新文件注册进项目清单。[微软：Solution Explorer 工具栏](https://learn.microsoft.com/en-us/visualstudio/ide/use-solution-explorer?view=vs-2022#solution-explorer-toolbar)

本步骤只是切换资源管理器视图，不是执行“文件 → 打开 → 文件夹”来改用原生 `.uproject` 工作区。后者的本机限制见第 4 节。

如果面板里暂时找不到文件夹视图，立即编辑文件的备用入口是 **Ctrl+O**，粘贴上面的完整 `.cpp` 路径打开。这样可以继续当前任务，不用先处理整棵项目树。

## 3. 需要在解决方案视图也显示目录时

仅更新 IDE 项目文件即可，**不必先删除 `.vs`、Binaries、Intermediate、Saved 或 DerivedDataCache**：

1. 保存 VS 中所有修改，关闭解决方案或退出 VS，避免外部重写时冲突。
2. 在 Windows 资源管理器找到：

```text
E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject
```

3. 右键 → Windows 11 必要时点“显示更多选项” → **Generate Visual Studio project files / 生成 Visual Studio 项目文件**。
4. 完成后打开同目录的 `EmberTrial.sln`。在游戏项目的 Source → EmberTrial 下检查 Tests。

这一步重建的是 IDE 项目清单，不是 Clean / Rebuild 游戏，也不是清空编译缓存。它可能更新 IDE 索引；不承诺耗时为零。[Epic：项目生成流程](https://dev.epicgames.com/documentation/en-us/unreal-engine/how-to-generate-unreal-engine-project-files-for-your-ide?application_version=5.5)

如果右键菜单没有生成选项，先用第 2 节的完整路径继续编辑并告诉我，不必重装引擎或清缓存。

日常可按以下规则操作：

| 变化 | 当前建议 |
|---|---|
| 修改已有文件内容 | 保存即可；如提示外部更改，确认后重新加载磁盘版本 |
| 外部新增 `.cpp` / `.h`，只需查看编辑 | 用文件夹视图；新增文件确保位于对应模块 Source 目录 |
| 需要同步解决方案树 / IntelliSense 的项目上下文 | 轻量生成项目文件，不清缓存；一批新增文件结束后统一做一次即可 |
| 新增模块、Target 或更改构建结构 | 刷新项目文件并正常 Build，不能只刷新视图 |

当前 `.sln` 工作方式没有一个普通“刷新”按钮能同时保证补齐所有外部新增文件的项目 / filters 条目。不要把“追踪活动文档”或“自动重新加载已打开文件”设置当作目录自动注册功能，也不需要手改生成的 XML。

## 4. 若希望长期免生成项目文件

VS 17.12 起支持原生 `.uproject` 工作区，直接通过“文件 → 打开 → Unreal Engine Project”打开工程，能够处理外部新增内容而不反复生成 `.sln`。本机 17.14.20 满足版本条件，还需对应 UE 工具组件。[微软：直接打开 UE 工程](https://learn.microsoft.com/en-us/visualstudio/gamedev/unreal/get-started/vs-tools-unreal-uproject)

但微软同页仍列出**引擎和游戏源码需位于同一逻辑驱动器**的限制。本机引擎是 `D:\UE5\UE_5.5`，工程在 `E:\UE_Project\Demo\EmberTrial`。不能仅因为 VS 版本够新就保证直接打开可用。跨盘需要另行安排路径迁移或官方符号链接 / 引擎关联方案；本次不改这些配置。当前建议使用“`.sln` 构建 + 文件夹视图浏览”，不中断 C2-03C。[微软：跨驱动器限制与方案](https://learn.microsoft.com/en-us/visualstudio/gamedev/unreal/get-started/vs-tools-unreal-uproject#game-source-and-engine-source-are-on-different-drives)

## 5. 回到本次练习

现有 Tests 目录不要重建。按 [C2-03C 指南](M0_05C2_03C_DodgeAutomation.md) 第 2–3 节增加友元和新的 `ETPracticeDodgeMovementTests.cpp`；新文件必须保存到真实的 `Source/EmberTrial/Tests`，不能保存在 `Intermediate/ProjectFiles`。本次检查时新测试文件尚未存在。

看到旧文件、成功在正确位置保存新文件后，继续正常 Build 和自动回归。仅解决目录显示问题不表示 C2-03C 已完成。

# ADR-001：复用现有 Character，建立正式 Framework 边界

日期：2026-10-02；交接更新：2026-10-03。状态：**已实施，M1-02B 验收通过**。证据见 [验收报告](../Reports/M1_02B_Verification.md)。

## 背景

现有 AEmberTrialCharacter / BP_ThirdPersonCharacter 已通过移动、动画、交互、暂停与打包验证。蓝图及练习菜单可能持有具体角色类型的引用。为了统一前缀而重命名原生类或复制另一套 Character，会引入重定向 / 引用兼容和重复输入实现的成本。

## 决策

1. 将原计划中的“正式 AETCharacter”落实为现有 **AEmberTrialCharacter**，继续作为唯一玩家身体。保持原生反射类名，公开头文件迁入 Public/Characters，并加 EMBERTRIAL_API。
2. 新建 **AETGameMode** / **AETPlayerController**；Blueprint 选择具体 Controller / Pawn 资产，新地图使用 BP_ETGameMode。角色资产继续使用已验证的 BP_ThirdPersonCharacter。
3. C++ 实现规则与生命周期，Blueprint 装配资源 / 类 / UI Bootstrap。核心模块不包含 UI 类；UI 通过公开契约或 UE 自带的 Pawn 变化事件接入。
4. 旧模板 AEmberTrialGameMode 保留为 M0 对照。当前地图级 Override 建立 M1 入口，项目默认 / Cook 切换在 M1-07 完成。
5. 当前 PracticeAction / 练习 UI 继续作为过渡基线。GAS 接入时分批替换正式入口，**同一角色同一正式动作不得同时运行练习组件与 GAS 两套状态机**；不能把“新 Controller 已存在”记成战斗 / UI 迁移完成。

## 影响与验收

收益是复用已验证行为，避免两套玩家移动 / 动画逻辑。代价是类名保留模板来源，现有原型代码仍需在对应阶段迁移 / 收回。Public 契约不能成为任意读写角色私有字段的入口。

EmberTrial.Build.cs 暂时为本模块的旧根目录头文件增加 PrivateIncludePaths；迁入 Private/Characters 的现有 .cpp 可继续引用练习组件。此路径只服务核心内部，不作为 UI 的公开接口。其余旧 Actor / 组件 / Tests 本批保留原位置，后续有实际迁移需求再整理。

验收已包含 Editor / Game Build、现有蓝图编译、新地图正确使用 GameMode / Controller / Pawn、重复 PIE 清理及旧地图回归。直接日志与用户实机反馈分别记录；UI 生命周期 / ASC 仍在后续阶段，未计入本次实施完成范围。

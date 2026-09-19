# M0-03：C++ Actor / 蓝图 / 实例数据验收

日期：2026-09-19。结论：通过。用户实际用时约 **15 分钟**。

## 1. 用户提交的结果

- Build 成功；第一次运行两个实例各输出一次 Target_A / Target_B。
- 修改 A 的实例标签后，无须重新编译 C++，输出 Target_Changed / Target_B。
- 模板角色移动、转向、跳跃正常；蓝图和场景已保存。
- 原示例写的是 Target_A_Changed，实际使用 Target_Changed 同样证明实例覆盖生效，不要求为字符串差异返工。

## 2. 本地核对与证据边界

检查了 `Source/EmberTrial/ETPracticeActor.h` 和 `.cpp`：

- 继承 AActor；SceneRoot / Mesh 使用 CreateDefaultSubobject 在构造函数创建，组件附着关系正确。
- Tick 关闭；Mesh 的 C++ 默认碰撞为 NoCollision。
- 组件为反射管理的 TObjectPtr；属性暴露与 private / AllowPrivateAccess 配合符合本轮要求。
- PracticeLabel 使用 FText / EditAnywhere / BlueprintReadOnly，默认值通过 NSLOCTEXT 定义。
- PrintPracticeInfo 为 BlueprintCallable，打印当前实例的对象名与标签；没有逐帧日志逻辑。
- 新增类向导给 `.uproject` 的 EmberTrial Runtime 模块加入 `AdditionalDependencies: ["Engine"]`，保留此变更。

`EmberTrial/Saved/Logs/EmberTrial.log` 中确认引擎为 5.5.4；本地时间约 15:00:20、15:00:30、15:01:01 三次 PIE 中，两实例每次各一条记录。前两次为 Target_A / Target_B，最后一次为 Target_Changed / Target_B：

```text
LogTemp: Display: [M0-03] Actor=BP_ETPracticeActor_C_UAID_BCECA00FDA6C390303_2128847031 Label=Target_Changed
LogTemp: Display: [M0-03] Actor=BP_ETPracticeActor_C_UAID_BCECA00FDA6C390303_2136441032 Label=Target_B
```

DLL 和构建 receipt 的时间为 2026-09-19 14:51:30；UHT manifest 含 ETPracticeActor.h。结合成功加载新类的运行记录与用户 Build 成功反馈，接受本轮构建结果，不重复构建。日志筛查未匹配 Blueprint / Script / OutputDevice Error、Fatal error、Assertion failed；这不代表排除了所有 Warning。

新资产已落盘：

```text
Content/EmberTrial/Blueprints/Practice/BP_ETPracticeActor.uasset
Content/__ExternalActors__/ThirdPerson/Maps/ThirdPersonMap/4/1M/IA4JZ2UADF0VS53A8Y4COD.uasset
Content/__ExternalActors__/ThirdPerson/Maps/ThirdPersonMap/C/HZ/E8ZMUQOMX0T2ZZUBGW3NO5.uasset
```

两个场景 Actor 使用外部 Actor 文件保存，因此不能只保存 / 提交蓝图和地图主文件。以上资产一并纳入版本检查点。

我核对源码、文件和日志，没有直接打开二进制蓝图逐节点检查，也未代操作实机画面。基础输入和画面结果来自用户确认；日志支持蓝图调用和实例值已生效的结论。本轮未执行打包，也未测试尚未实现的 UMG。

## 3. 本轮复盘

1. **职责分工**：C++ 建立对象结构和可调用行为，蓝图配置外观并编排调用，场景实例保存具体数据。
2. **默认值层次**：C++ 构造默认值 → 蓝图类默认值 → 场景实例覆盖。本次修改的是最后一层，A 的变化不影响 B，也不需要 C++ 重编译。
3. **两个权限维度**：EditAnywhere 决定 Details 能否编辑；BlueprintReadOnly 决定蓝图图表能否直接 Set。因此“蓝图只读”和“编辑器实例可配置”可以同时成立。
4. **更新时机**：一次性初始化由 BeginPlay 触发，不需要 Tick。下一步把这个原则应用到 UI 的显示与隐藏。

当前未发现需要修改的功能问题；不为排版改动用户源码。单次基础练习的 15 分钟不能直接外推整个 Demo 工期。

## 4. 下一步

M0-03 完成；M0-04 拆成 A / B / C 分批实施：A 是范围提示和候选目标切换，B 是按键交互，C 是暂停菜单与输入恢复。当前仅下发 A，尚未实施或测试。M0 整体仍在进行。

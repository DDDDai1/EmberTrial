# ADR-002：ASC 放在 Character，分开 ActorInfo 与玩法就绪

日期：2026-10-03；实施状态更新2026-10-04。**ASC所有权 / ActorInfo已于A1验收，属性 / 一次初始化 / GameplayReady与真实Effect动态边界已于A2验收，Presenter / 血条View已于B1 / B2验收；B3真实数据源更换待实施。**

## 背景与决策

本 Demo 为单人，角色死亡后通过快照重建。当前没有跨 Pawn 保留能力 / 效果或网络预测需求，因此 ASC 由现有 AEmberTrialCharacter 持有，OwnerActor 和 AvatarActor 都是该 Character。此决策落实 Introduction 的既定路线，不创建第二个玩家 Character 或 PlayerState ASC。

1. 先使用原生 UAbilitySystemComponent 原生子对象，Character 实现 IAbilitySystemInterface；真正需要自定义 ASC 行为时再派生类型。
2. ActorInfo 记录 Owner、Avatar、Controller、动画等运行时关联；BeginPlay 检查绑定，重新 Possess / UnPossess 时刷新关联，不重置生命或再次授予能力。
3. UE 5.5.4 的 UAbilitySystemComponent::InitializeComponent 已执行默认 InitAbilityActorInfo(Owner, Owner)。项目检查正确绑定时只 Refresh；关联不正确时才 Init。日志的 Mode=Refresh 是正常结果，不能以项目日志次数推断引擎从未初始化。
4. 结束前停止刷新，取消能力 / 清理 Cue、ClearActorInfo；结束过程中发生的 UnPossess 不能再次建立关联。组件 / Actor 存活与 World 生命周期分开处理。
5. M1-03A1 只有 ActorInfo 基础接入。A2-01 增加 AttributeSet / 基础属性与显式 GameplayReady，采用 OnGameplayReadyChanged(Character,bReady) + IsGameplayReady 查询；通知前提交状态，EndPlay撤销。初始化属性 / 授予能力的幂等保护独立实现，不能把 ActorInfo 有效当成生命 / 能力 / 装备已经就绪。首次仅表示最小属性链完成；之后能力 / 装备 / 存档接入要扩展就绪前置条件。
6. B1 UI Presenter已监听Pawn变化，先解绑旧数据，再检查GameplayReady；订阅就绪事件后立即读就绪状态，处理事件先发生的情况。展示数据来自ASC；B2 Widget已通过DTO事件 / 立即快照 / 显式解绑，不复制规则数据。

## 收益、代价与边界

收益是所有权简单，身体 / ASC 一同创建，便于单人敌人与角色共用规则。代价是死亡 / 重生时属性、能力、装备效果与 UI 都需要显式重建；未来若需要跨 Pawn 保留状态或多人，须重新评估 PlayerState ASC 与复制 / 预测，不能声称当前已支持。

M0 PracticeAction 目前继续负责练习输入。M1-03A1 不授予动作能力，因此不会出现两套动作同时执行；M1-04 接入正式攻击时收回相应练习入口，其余动作在对应迁移批次处理。

本项目的 Weak 引用 / 解绑原则沿用已验收的 M1-02；C2 Probe 是裸 Pawn，不需要 ASC，不代表正式重生身体。

## 验证安排

- A1：已通过，单个ASC / 接口、归属 / 控制刷新、结束保护与双目标构建，见 [验收](../Reports/M1_03A1_Verification.md)。
- A2-01：已通过，Health / MaxHealth、配置范围、一次初始化与GameplayReady，见 [验收](../Reports/M1_03A2_01_Verification.md)。
- A2-02：已通过，五个Instant GE、Current / Base动态范围及属性修改后Ensure不重置，见 [A2总验收](../Reports/M1_03A2_Verification.md)。
- B1：[验收通过](../Reports/M1_03B1_Verification.md)，Bootstrap持有 / 弱引用数据源、属性事件 / 快照 / 重订阅 / 清理；原生观察早于Ready直接验证，结束保护补项在B2关闭。
- B2：[验收通过](../Reports/M1_03B2_Verification.md)，晚加入快照 / 无Pawn / Waiting / 重建显式解绑，先订阅后Ready有直接日志。
- B3：[纯蓝图方案](../M1_03B3_DataSourceRebinding.md)已交接，真实新身体 / 非玩法Pawn / 旧属性修改与销毁隔离、Ready先于Possess的快照，待实施；未新增生产初始化开关。

官方 [InitAbilityActorInfo](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/GameplayAbilities/UAbilitySystemComponent/InitAbilityActorInfo) 说明 Owner / Avatar 的含义；具体实现依据本机 UE 5.5.4 的 AbilitySystemComponent_Abilities.cpp、GameplayAbilityTypes.cpp。官方网页当前默认版本可能比本机新，不据此升级项目版本。

# M1-04A：正式单段攻击验收

日期：2026-10-06。用户约15:38报告全项通过，**M1-04A验收完成**。本轮助手读取源码、构建产物、索引、资产和日志，更新文档；没有代写C++、修改UE资产或执行Build / PIE。

## 1. 直接核对

- Source由33增至37份：新增四份Tag / Ability文件；修改核心Build.cs、Character两份与PracticeAction.cpp，其他29份保持M1-03C基线。四份新文件去除空白后与A指南代码块一致。
- GameplayTags公开依赖、一次授予 / Spec Handle、正式活动门控、Montage Task回调、ScopeLock结束保护与Character结束清理已读取；Practice检查已改为GetCurrentActiveMontage。没有直接Include UI或新增角色Tick。
- Editor receipt为15:11:40，Game receipt为15:13:37，均UE5.5.4 / Win64 / Development、相应Editor / Game目标。CLion compile_commands于15:12:14更新，包含18份工程cpp与两份新实现。构建成功和导航实际体验同时采用用户反馈，不把receipt等同于本轮重新编译。
- GA_ET_LightAttack、IA_ET_LightAttack、IMC_Default分别15:16:12、15:17:42、15:18:10落盘；玩家Blueprint于15:27:22保存。原清单只有玩家Blueprint哈希变化，其余28项不变。父类、具体默认值及内部连线采用用户反馈，文件存在 / 哈希不能独立证明这些配置。

新增能力与输入各一项；把一直使用但尚未单列的模板共享IMC_Default也纳入来源清单，故清单由29增至**32项**。这不是新增三份资产，实际新资产为两份。保存完整37份源码与32项登记资产的[下一批基线](Raw/M1_04A_Baseline.json)。

## 2. 日志证据

原日志EmberTrial/Saved/Logs/EmberTrial.log的15:35–15:37相关片段已保存为[验收日志](Raw/M1_04A_Acceptance_2026-10-06.log)。UE时间戳为UTC，换算本地加8小时。

- 两轮PIE各一条Granted，Handle分别1 / 2，能力类GA_ET_LightAttack_C。
- 13条Begin对应13条End：11次正常完成、2次取消，全部End的AttackingTag=0。忙碌请求被拒绝，未观察到额外并发Begin。
- 第二轮两次取消后均接Practice受击；原2→3有HitInterrupt样本。15:36:30攻击开始，15:36:30.859暂停、15:36:36.439恢复，15:36:37.139正常结束，支持暂停期间Montage未继续完成。
- 15:37:38旧EmberTrial.M0.PracticeAction.Lifecycle结果Success，BeginEvents / EndEvents之间没有测试失败事件。
- 没有匹配到本轮游戏 / 蓝图运行错误、Accessed None或Fatal error。全日志存在Epic遥测HTTP SSL连接Warning，来源为LogHttp；不归为能力逻辑失败，也不称全日志零Warning。

日志不能独立证明具体鼠标长按、菜单边界、移动拒绝和视觉效果；这些与用户实机结果共同构成验收。

## 3. 用户实机结果与范围

Editor Build / 索引 / Game Build，GA父类 / Montage / Character能力Class / 左键映射 / Compile / Save，正常攻击、长按 / 连按、动作门控、数字3取消、暂停鼠标边界和重开均通过。Lifecycle一次Success；最后E / P / 移动跳跃与HUD100/100正常，无待修复现象。

本批完成正式攻击授予 / 输入 / 激活 / Montage完成和取消、M0迁移互斥与相关生命周期。**尚无命中检测、真实伤害或正式受击 / 死亡**；当前HUD100/100符合A范围。当前Development Game构建不等于已Cook / 打包，旧M0包仍不能代表现阶段。

## 4. 工时与下一批

用户手动50分钟：代码15 + 蓝图资产30 + 检查5；构建等待2分钟另计。A原初估85–130分钟，实际首次顺利完成较快；不能因此按固定比例缩减涉及碰撞 / 时序的新任务预算。

项目累计手动**1248分钟（20小时48分钟）**，等待**75分钟**。继续选定的B分层验收：M1-04B拆成B1命中窗口 / 扫掠 / 去重与B2真实GameplayEffect伤害；复用攻击输入和保留训练目标，旧HUD / 换身体 / 闪避矩阵不重搭。下一批手动步骤见[B1指南](../M1_04B1_AttackTrace.md)。

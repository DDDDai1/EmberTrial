# M2-06B1：锁定屏幕标记验收

2026-10-24，UE5.5.4。用户13:39反馈全项完成，无失败或布局问题；手动代码 / 配置80、检查10，合计90分钟，构建等待5。**B1验收通过，无阻塞补项；进入B2镜头跟随。**本轮只读核对并写文档，未改实际Source / UE资产、执行Build / 自动化 / PIE / Cook或操作Git。

## 1. 实现、构建与文件位置

实际新增六UI文件（3h / 3cpp）、零旧Source变化；按既有clang-format归一后六份均与下发候选一致。A的147旧Source / 全部Config保持；新测试实际位于**EmberTrialUI/Private/Tests/ETTargetMarkerRulesTests.cpp**。前次C1083源于误建Core/Tests，已按用户当次授权原样移动，内容hash不变，后续双构建及专项通过；不用加反向模块依赖或公开Private头。

实际**153 Source / 83cpp，Core57 / UI23 / Editor3；21项自动化（Core17 / UI4）**。最后Source13:20:12；UI / Editor DLL13:24:58、Core DLL / Editor receipt13:24:59；Game exe / receipt13:27:38覆盖最终源码，用户确认正常双构建与三模块索引。时间统一为Asia/Shanghai，不把UTC日志05点误写成本地05点。

Controller独立UI组件观察Core同一目标，按资格开启PostUpdateWork投影、无暂停Tick / 候选扫描 / GAS写入。Widget只写直接Canvas子级LockMarker的位置与可见性，无Actor引用、Tick、Binding；离屏只隐藏子控件，根仍可更新以重新出现。重建只换View、不改变Core身份，组件结束关闭更新并解绑。

## 2. 资产与规则专项

新WBP_ETTargetLockMarker最后保存13:35:23，原BP_ETPlayerController保存13:36:29；其余291份旧UE资产与A基线一致。本地实际**292份UE资产 / 来源73行**。原生Widget父类、单LockMarker Border / 直接Canvas子级、16×16 / 45°、无Bind / Never、单Marker组件 / 正确Class / 24-24 / AutoActivate按用户反馈与正常运行接受；助手未打开UE遍历全部Details。

13:30:12–27曾有LockMarker必需绑定错误；后续13:35:24.111与13:35:31.436均有该正式WBP的本项目**[M1-07C1] Valid**，保存后真实View创建和投影正常，故早期错误关闭，不因完整日志仍保留历史Error要求重新验收。

**13:36:47.580，EmberTrial.UI.TargetLock.MarkerRules：Success。**纯规则覆盖投影资格、有限坐标 / 尺寸、边界和安全内边距；不创建Viewport、不证明真实DPI / 渲染 / 输入。旧20项本轮沿用，没有当次全组Success声明。

## 3. 实机证据与覆盖口径

13:37:15.245正式View创建；13:37:17.960 Core锁定真实Enemy、投影Active=1，13:37:21.908 PawnChanged后Active=0 / View移除。13:37:34–45再一轮相同初始化 / 释放，说明前一轮实例未继续投影。

13:37:56.966第三轮创建，13:37:59.331锁定。**13:38:05.483–484 C_0移除 / C_1创建，13:38:11.422 C_1移除 / C_2创建**，两次重建后立即恢复投影，中间无Core重新Locked / Cleared。实例尾号递增是新的View身份，符合重建语义；Core目标仍同一份。

13:39:05.532暂停、13:39:06.097继续；13:39:07.798再次暂停，13:39:08.814返回菜单，随后投影关闭、PawnChanged清Core、旧C_2移除，13:39:09.926菜单世界结束。日志支持Pause / 切图 / Stop清理。

Tab单标记、目标 / 相机移动、离屏 / 背后隐藏再出现、窗口尺寸调整、Deactivate→Activate false时Core保持、敌人死亡、Failure / Retry / Continue以及能力 / 五HUD短基线按用户完成反馈接受。当前日志13:38:26–27是主动Tab解除 / 重锁，**不把它冒充组件失活的独立日志证据**。用户未附具体PIE方式或分辨率，记录为“未注明”；窗口缩放结果通过，不宣传指定分辨率、Windows缩放倍率、分屏或完整DPI矩阵已测，不追加重复验收阻塞本章。

早期绑定错误之外，正式运行段未见Fatal / Accessed None或新的Blueprint绑定错误；不宣称全日志零Warning。本轮未测新性能、未Cook；旧Cleanup/Attempt01仍不含后来次数HUD / Poise / F / 冷却HUD / 锁定，不能用于证明B1独立包。

## 4. 工时、下一批与提交状态

B1手动90 / 等待5；M2-06累计**180 / 10**，M2累计**1250 / 114**；项目累计**3261分钟（54小时21分）/ 明确等待299分钟（4小时59分）**。仅累加用户报告，不追加助手设计 / 排障或未单独报告的学习时间。模块当前按已用180 + B2 80–130 + C 60–100 = **320–410分钟**估计，只有180为实耗。

B2下发三新Core / 两旧Controller、沿原UpdateRotation接入平滑镜头；无UI修改 / 新美术 / 临时Controller图。候选和纯规则静态检查不等同用户已Build或实机完成。

最新HEAD为用户提交c902dcc，完整六新UI / 两资产已纳入，实物与HEAD一致，测试仅正确UI/Private/Tests一份。误放Core路径 / AM骨架暂存问题已关闭；助手未代Stage或修改历史提交。当前B1报告与最新文档修订待提交，不影响玩法验收。

## 5. 当前 Commit / Push（固定放在最后）

B1完整实现已由用户提交`c902dcc`；正确UI测试路径、六新UI与两资产均已纳入，实际Source / 资产与HEAD一致，无需重复Stage，旧误放 / AM暂存问题已关闭。本轮Stage新B1报告、指南收尾、来源73、状态 / 架构 / 路线及B2指南最新修订。建议Commit（英文）：`docs: verify the target marker and prepare camera following`。中文译文：`docs: 记录锁定标记验收并准备镜头跟随`。B2三新Core / 两旧Controller尚未实施，完成后再按[B2清单](../M2_06B2_LockCamera.md#9-本章-commit--push固定放在最后)提交。Raw / 日志 / 生成物 / 包体留本地，Git由用户操作；助手未改索引、Commit、Push或联网核验远端。

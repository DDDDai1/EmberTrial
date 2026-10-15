# M2-01B2：体力HUD验收

更新2026-10-15，UE **5.5.4**。用户14:12报告全项完成、无问题，手动30 / 构建等待5分钟。**B2验收通过，无待补项；M2-01资源基础收口。**依据实际Source / 资产哈希、双目标receipt / 产物、三CodeModel、当次日志和用户画面反馈；本轮没有重跑Build、测试或游戏。

## 1. 实现与资产

当前 **87份Source / 44cpp / 49项来源清单**。四个新UI文件全文、Bootstrap两文件的指定接入，忽略空白后均与指南一致；其余81份旧Source与B1哈希保持，包括Core、恢复 / 两资源测试、旧Health UI、Editor校验器与Build依赖。

StaminaPresenter复用LocalPlayer当前Pawn观察、Ready与两属性事件；清理时递增观察序号，拒绝旧ASC / 旧轮回调，快照去重后广播副本。Bootstrap强持有Presenter / 当前Widget，View弱引用Presenter；结束先取消View订阅并移除，再Shutdown Presenter。Widget沿用DisableNativeTick / Never，无资源写入或UI轮询。

新资产为`/Game/EmberTrial/ui/HUD/WBP_ETStaminaHUD`，由用户复制项目生命HUD后换父类、改名和布局；实际文件最后修改14:08:13。唯一旧资产差异为BP_ETPlayerController的Stamina Widget Class配置，原WBP_ETHealthHUD和其他47份旧资产保持B1哈希。AssetManifest新增一行体力HUD、更新Controller来源 / 用途 / 哈希，总49行；字体沿用旧来源，未引入外部资源。

共享Config / uproject、冻结HealthPresenter指南与C3 Attempt02七个包指纹不变。没有新Cook，旧包不含M2体力功能。

## 2. 构建与编辑器

- Editor receipt **13:59:34**，UI DLL13:59:33，晚于本批Source；5.5.4 / Win64 / Development / Editor。
- Game receipt **14:01:36**，exe14:01:35；UBT记录9个动作，包含新Presenter / Widget与Bootstrap及生成代码，链接和WriteMetadata完成，总40.17秒。
- 最新Editor日志是up-to-date / 0.46秒，不冒充第二次全量编译。三CodeModel共44cpp，新增两cpp均在EmberTrialUICodeModel。
- 新Widget在 **14:08:14、14:09:14** 两次项目Valid；原通用BindWidget校验器直接覆盖，没有新增Editor规则或负例Probe。

14:03:45–14:04:43共31条Missing BindWidget Error，发生在新副本Reparent后逐项改名期间，与指南预告的中间状态相符。最终控件完成、Save / Valid后，运行及后续记录无Error / Fatal、Accessed None或Ensure失败；保留历史错误，不将它们当作仍未解决的问题，也不说全日志从未有Error。

## 3. 运行与证据边界

实际有四份游戏世界HUD实例的订阅 / 取消订阅、创建 / 移除，以及四次Presenter停止，均闭合；世界切换中的对象尾号变化正常。14:09:34开发注入-25，真实100→75 / Timer1，14:09:35恢复开始；14:09:49查询已回100 / Timer0。体力数字、绿色条与生命100隔离按用户画面反馈接受；连续Snapshot / View使用Verbose，默认Display没有每个数值事件属于预期。

14:10:16注入-100，19秒Pause，24秒Resume；冻结与继续后的画面按用户反馈。14:10:31真实死亡并出现Failure，14:10:35 `Intent=Retry`，旧体力View退订 / 移除、Presenter停止后，新世界创建下一份体力View。14:10:46 `Intent=MainMenu` 再次清理游戏UI；菜单隔离按用户反馈。

用户确认Continue返回单面板，但现存日志没有单独的Continue请求；后续两轮是新PIE Session，**不把新Session记作Continue直接证据**。该项按用户实机反馈接受，不为缺少独立日志重做已验证的切图矩阵。14:11:11未满体力后Stop，旧View / Presenter结束；随后新PIE创建新面板与新100。E激活M1_Target_A有日志；左键25、原2不耗体力、移动 / 转向 / 跳跃与最终保存按用户反馈。

B2没有修改Core或B1测试，未要求重跑两项资源自动化；最近一次两项Success仍为B1的13:23:19。NoPawn / Unsupported / Waiting、同世界独立View重建与旧ASC拒绝本次只审查代码，未新增逐项实机测试；不把这些写成全面动态覆盖。正式成本 / Dodging / 无敌仍待M2-02。

## 4. 工时与Git

本批30 / 等待5；累计用户手动 **2141分钟（35小时41分钟）**、已明确等待 **200分钟（3小时20分钟）**。M2-01三批合计手动130 / 等待15，单独检查耗时未登记；不据总时长推算检查占比或未来节省率。

本地HEAD与缓存origin/main均 **8b725e5**，包含B1 / B2实现；旧误建Effects / Resource路径已不在当前索引，本轮没有联网核验远端 / LFS传输。此前AD问题已随用户提交解决，不再当作当前待办。

本地证据：`Raw/M2_01B2_2026-10-15/Acceptance.json`、AcceptanceExcerpt与构建摘录。[M2-01总验收](M2_01_Verification.md)汇总三批，不另设旧功能验收门槛。

## 5. 本次 Commit / Push（固定放在最后）

实现已在8b725e5，没有新差异的Source / uasset不重复提交。本轮补充本报告、M2-01总验收、AssetManifest两行、B2完成状态与路线 / Progress / Backlog / Architecture / VersionControl / AcceptanceWorkflow / Introduction实际收尾；下一批方案仍只是设计，不能记作已实现。

Raw / 日志 / receipt / CodeModel / Binaries / 历史包留本地。用户在CLion刷新Changes → 重新Stage最新文档 / 清单 → 检查Staged Diff → Commit / Push；可用 `docs: verify stamina HUD and close resource foundation`。助手未操作Git索引或远端。

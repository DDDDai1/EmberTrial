# M2-02B：方向位移与正式输入验收

2026-10-15，UE **5.5.4**。用户16:48反馈全部完成、无问题，手动30 / 构建等待5分钟。**B批验收通过，无待补项。**本次读取源码、构建产物 / 收据、索引、资产哈希和现存日志，没有修改工程实现、重跑Build / 测试或操作游戏。

## 1. 实现与资产

实际 **93份Source / 48cpp / 51项来源清单**。三新Task / 测试文件、五旧Dodge / Character / Validator文件，忽略空白后均与指南一致；其余85份旧Source保持A哈希，包括原CostContract、资源实现 / 测试、旧Practice及全部UI Source。

Character直接读取当前MoveAction并转换世界方向，无输入按身体前方；激活时固定方向。Ability仍拥有Montage / 成本 / Dodging，Task独占本次RMS，活动期间Tick；自然位移完成不结束动作，取消 / 死亡 / EndPlay经OnDestroy释放自身Source。Montage与Source准备成功后一次提交成本，Runtime与Editor共用移动配置检查。

唯一两份旧资产差异为 **GA_ET_Dodge、BP_ThirdPersonCharacter**；其余49份清单资产保持，包括IA_PracticeDodge、IMC、Cost GE、原Montage、正式地图。没有新资产或外部美术。玩家Started只到RequestDodge(Self)、旧执行线断开按用户反馈接受；日志没有旧M0 DodgeMove，支持当前请求已走正式链路。清单仅更新上述两行来源 / 用途 / 哈希。

共享Config / uproject、冻结M1-03B1指南与M1 C3 Attempt02七个包指纹不变。没有新Cook；旧包不包含M2正式闪避。当前有正式方向移动和成本，**尚无无敌窗口**；占位仍为一段侧闪动画，不宣称八方向专用动画已完成。

## 2. 构建与资产校验

- Editor receipt **16:36:41**；Core DLL16:36:26、Editor DLL16:36:30，均晚于本批相关Source。
- Game receipt **16:39:23**，exe16:39:21；UBT记录12个构建动作、链接与Metadata完成，总78.27秒。
- 最新Editor记录为up-to-date / 0.49秒，单独保留，不冒充第二次全编译。三CodeModel为Core32 / UI13 / Editor3，共48cpp；两新cpp均在Core目标。
- GA **16:41:57、16:42:01**两次项目Valid；玩家 **16:43:59 Valid**，均Issues0。全日志无Error / Fatal、Accessed None或Ensure失败，也无LogET玩法Warning；图标 / EOS / 退出遥测Warning不当作本批玩法错误。

## 3. 自动化与实机证据

**16:44:35 CostContract、16:44:36 Movement各一次Success**，本次审查没有重跑。Movement在临时世界真正激活原生GAS能力、播放已有Montage、应用真实成本GE并运行CharacterMovement；覆盖方向转换、一次支付 / 忙碌拒绝、位移先结束而动作仍保持、恢复阻断、非法配置 / 外部Source拒绝、墙阻挡、取消不退款、暂停冻结 / 恢复、真实死亡和活动中EndPlay。

| 固定模拟步长 | Travel2D | SourceTime |
| --- | --- | --- |
| 1/30秒 | 150.05cm | 0.3001秒 |
| 1/60秒 | 150.05cm | 0.3001秒 |
| 1/120秒 | 150.05cm | 0.3001秒 |

这三行是测试世界步长，不是硬件实际FPS测量。键盘连线、相机关系和坡面表现由用户实机补充，不以方向数学断言替代输入端到端。未变资源 / Practice不重复运行旧矩阵。

正式Blueprint身体有 **13组Move begin / end及一次25成本**：一组为16:42:17迁移前ke验证，其余十二组在16:45之后，不将十三组全部称为2键按压。十二次正常WindowEnded、一次取消，全部动作End为Dodging0。正常空地约150.05cm / SourceTime0.3001；墙边样本6.28cm、斜向受阻129.73cm，符合碰撞截短。不同世界方向有日志；16:46:25位移0.1000秒 / 50cm时由诊断3受击取消，先Move end再进入HitReact，无第二次支付。

16:46:32 E激活M1_Target_A，16:46:34有Pause / Resume。这次菜单日志在已记录的受击取消之后，**不单独作为活动位移暂停的直接证据**；该规则由新自动化及用户实机反馈共同支持，不为缺少一条Display样本增加补测。长按 / 忙碌无补播、四向斜向 / 镜头90度 / 松键前方 / 方向固定、坡面上下、最终Stop重开双100、短基线与slomo1 / 临时Cube恢复按用户反馈接受。地图哈希保持，未留下临时场景改动。

离地中止、底层ApplyRootMotionSource失败和同步结束极端路径有代码保护，本次不宣称每种异常均动态测过。UI旧矩阵沿用既有证据。无待补项。

## 4. 工时与下一批

本批手动 **30分钟**、等待 **5分钟**；累计手动 **2221分钟（37小时1分钟）**，已明确等待 **210分钟（3小时30分钟）**。M2累计手动210 / 等待25；M2-02 A+B合计80 / 等待10。独立检查耗时未拆报，不由总录入时长推算检查比例，也不据此线性压缩后续动画 / 美术预算。

后续 **[M2-02C短无敌与集成](M2_02C_Verification.md)已验收**，默认0.15世界秒、真实伤害门控和按Handle清理成立，计时补修后单项Success。本B报告的证据与工时保持原批次；[M2-02整章报告](M2_02_Verification.md)已关闭，不需重复实施B。

本地证据：`Raw/M2_02B_2026-10-15/Acceptance.json`、AcceptanceExcerpt与构建摘录；Preparation保留为指南下发时的历史记录。

## 5. 本次 Commit / Push（固定放在最后）

核对期间用户已提交本批实现，本地HEAD与缓存origin/main均 **c010520**，Source / Content / Config无未提交差异。本轮未联网核验远端 / LFS，不替用户操作Git。

本轮补充本报告、AssetManifest玩家 / GA两行更新、B指南已验收状态，以及Progress / Backlog / M2路线 / 闪避路线 / Architecture / VersionControl / AcceptanceWorkflow / Introduction收尾。无差异Source / uasset无需重复提交。CLion刷新Changes → 重新Stage最新文档 / 清单 → 检查Staged Diff → Commit / Push，可用`docs: verify directional GAS dodge movement`。Raw / 日志 / receipt / CodeModel / 生成文件 / 旧包留本地；C尚无实现，不把整个M2-02标记完成。

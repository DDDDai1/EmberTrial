# M3-05C1：音量偏好与本机保存验收记录

2026-11-06 18:55完成。**验收通过，无待补，进入[M3-05C2实际声音路由](../M3_05C2_AudioRouting.md)。** 本章数据 / 保存合同成立；RuntimeAudioApplied0符合本批范围，尚无实际游戏混音或正式音乐完成结论。

## 1. 代码、构建与专项

- 三新 / 两旧Core与本批候选去空白后全部一致；本地99b8146恰好包含这五个源码，复核起点工作区干净。助手仅整理文档 / 候选与本地证据，不操作Git提交 / 远端。
- 实际**225 Source / 128cpp（Core87 / UI38 / Editor3）、40自动化、302 Content / 来源83**。资产和共享Config本批未改，83行来源SHA全部与实物匹配。
- Editor / Game receipt时间18:45:33 / 18:47:09；双Build与三索引用户确认通过。18:49:26.782，EmberTrial.M3.Settings.AudioPreferences Result=Success。
- 18:49:43查询PIE基线1 / 1 / 1，18:49:47 StoreAccepted0，18:49:52查询仍1 / 1 / 1。原P与移动转向短查按用户实机反馈通过，不新增蓝图脚手架或跑旧全组。

## 2. 两轮独立进程证据

原证据存于E:/UE_Project/Demo/Artifacts/M3_05C1/Acceptance，含Editor.log、Audio01 / Audio02.log、AudioUser.ini与Summary.json，均留本地。下表时间为Asia/Shanghai，UE原日志UTC已加8小时。

| 检查 | 实际结果 |
| --- | --- |
| 启动隔离 | 两轮均-game，命令行使用同一专用AudioUser.ini与独立abslog |
| 合法保存 | Audio01 18:51:17.420 Accepted1 / SaveRequested1，0.5 / 0.4 / 0.2，数学组合0.2 / 0.1 |
| 非法拒绝 | 18:51:22.792、18:51:27.508两次Accepted0，快照保持合法组 |
| 总音量0 | 18:51:37.048 Accepted1；Master0、子通道仍0.4 / 0.2，组合0 / 0 |
| 最终提交 | 18:51:45.392保存0.6 / 0.3 / 0.8，组合0.18 / 0.48；18:51:50再次查询相同 |
| 全新进程只读 | Audio02 18:53:39.574查询同组0.6 / 0.3 / 0.8；本进程此前没有store / 保存，证明真正读回 |
| 正常结束 | 两轮退出请求约18:52:10 / 18:53:42；无Fatal / Assertion / Ensure / Accessed None；用户确认正常退出及专用配置保持 |

RuntimeAudioApplied始终0。上述组合值只是数学诊断，没有声音播放 / 音量生效测试。原P与移动转向为用户反馈，日志不替代完整键鼠观感；本批不重测M2 / 遗物矩阵或Cook。旧M2_07A/Attempt01不含M3。

## 3. 工时与下一步

本章代码40 + 阅读10 + 检查10 = **60分钟**，构建启动等待**5分钟**。M3累计**1828分钟（30小时28分）/ 等待121**；项目累计**5239分钟（87小时19分）/ 明确等待445分钟（7小时25分）**。助手工作不计用户实耗，M2仍1400 / 等待139。

C2由一个原生游戏GI服务管理专用SoundMix与三SoundClass，切图精确释放 / 重应用；保留GameUserSettings唯一偏好来源，迁移C1数据专用入口到实际apply。三新 / 两旧Core、共享Config及六UAsset / 两原创WAV当前只有指南 / 候选 / 生成方法，尚未写入正式工程，不预填资产来源或实际规模。C3再做共享音量界面。

## 4. Commit / Push（固定放在最后）

C1五源码已在本地99b8146。现在Stage本报告、C1指南收尾、C2新指南与实际修改的Progress / Backlog / M3与M2路线 / Architecture / AcceptanceWorkflow / VersionControl、Introduction / README。AssetManifest与生产源码 / 资产本轮不改。

- 英文：`docs: verify audio preferences and prepare runtime audio routing`
- 专业中文：`docs: 完成音量偏好验收并准备运行时声音路由`

C2未来五源码 / Config、六UAsset / 两WAV的准确清单见新指南第9节；未实施文件不提前Stage。Artifacts证据 / 候选 / 个人ini与生成缓存留本地，Stage后继续修改需重新Stage → Review → Commit / Push由用户操作，助手未核验远端。

# M7-07 Win64 Shipping 发布候选验收

**通过。** 用户于 2026-11-19 完成三轮独立实机，反馈无明显问题。用户记录手动 **10 分钟**、等待 **2 分钟**。[验收步骤](../M7_07_ShippingCandidate.md)

## 构建与实机结果

| 检查 | 结果 |
| --- | --- |
| 构建与自动化 | 配置校验 0 错误、0 警告。Editor 全量 **60 项通过、0 失败、0 未运行**。UAT 完成 Build、Cook、Stage、Package 和 Archive，退出码 **0**，实际用时约 **2 分 52 秒**。 |
| 同包同档 | 三份启动 JSON 使用 `Artifacts/Builds/M7_07/Attempt01`。内层 Shipping exe 与 IoStore TOC 的 SHA-256 都与当前包相符。三轮进程 ID 各异，共用独立设置文件及验收用户目录。 |
| Run01 | 23:50 启动，请求 **1920×1080**。用户确认大窗口、设置预览回滚、短刃听感及路由正常，A/B 检查点、双槽与治疗次数正常。 |
| Run02 | 23:51 启动，请求 **1280×720**。用户确认跨进程 Continue B、小窗、只读遗物与 Retry 正常，清场 **3** 后显示胜利页，返回后 Continue 禁用。 |
| Run03 | 23:53 启动，请求 **1280×720**。用户确认重启后继续禁用，新局从 Entry 开始并恢复默认状态。 |
| 画面 | 独立冒烟记录小窗下暂停面板局部覆盖背景生命 HUD 标题。用户三轮实机未报告明显问题。 |

候选包启动入口为 `Artifacts/Builds/M7_07/Attempt01/Windows/EmberTrial.exe`。三轮启动记录位于 `Artifacts/M7_07/Runtime`，其分辨率字段记录启动请求值。玩法检查以用户实机反馈为证。

两份运行存档位于 `Artifacts/M7_07/Runtime/Attempt01/User/Saved/SaveGames`。收尾时两份文件的 ETS1 封装、CRC32 和内容哈希均已核对，A/B 的 SHA-256 一致。构建与自动化原件分别为 Attempt01 下的 `UAT_Console.log` 和 `Artifacts/M7_07/Preflight/AutomationReport/index.json`。最新量化性能基线见 [M6-06 验收](M6_06_Verification.md)。

## 本次文档 Commit / Push

提交本报告、M7-07 指南、进度与路线入口。包、存档、日志和启动 JSON 保存在本地 `Artifacts/`。

- 英文 `docs(release): record M7 Shipping candidate acceptance`
- 中文 `docs(release): 记录 M7 Shipping 候选包验收`

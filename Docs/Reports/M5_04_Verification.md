# M5-04 独立包存档验收

**通过。** 2026-11-13 22:19（北京时间），手动 40 分钟，Build 与 Cook 等待 5 分钟。用户反馈无明显问题。

| 验收项 | 结果与证据 |
| --- | --- |
| 配置与正式回归 | 配置检查 PASS，0 错误、0 警告。打包前 60 项自动化通过，0 失败，其中 1 项伴随 EOS 后台请求警告。 |
| 打包 | `M5_04/Attempt01` 完成 Build、Cook、Stage、Package、Archive，UAT 最终退出码 0。归档包含实际游戏 exe、Pak 与 IoStore 容器。 |
| Run01 从 Entry 到 B | 首屏 Continue=0，Entry 出生。22:15:53 保存 A，v2 序号 1、清场 1、治疗 3。22:16:25 保存 B，v2 序号 2、清场 2、槽 A 生息余烬、槽 B 裂势印、治疗 2。 |
| Run02 跨进程继续 | 首屏 Continue=1，读取 B、v2 序号 2。22:16:55 实际恢复 B，日志确认前两场恢复清场、两槽遗物、120/120 生命与 2/3 治疗。 |
| Run02 Retry 与通关 | 22:17:22 死亡 Retry 确认从 B 出生，生命 120/120、治疗回到 3/3。新守关者 180/180。22:17:47 清场 3，写入结束标记序号 3。返回菜单 Continue=0。 |
| Run03 通关后重启 | 22:18:07 首屏 Continue=0。22:18:16 新试炼从 Entry 出生，双空槽、生命和体力 100、治疗 3/3。 |
| 运行目录、退出与双槽 | 三份启动 JSON 对应三个进程、同一启动 exe 指纹和同一用户目录。三轮退出码均为 0。两份存档文件都在 22:17:47 更新，最终内容 SHA-256 一致。 |

UAT 日志在 `Artifacts/Builds/M5_04/Attempt01/UAT_Console.log`。实际游戏二进制为归档下的 `Windows/EmberTrial/Binaries/Win64/EmberTrial.exe`，SHA-256 为 `2FF1D678B8D4D1118A07F79BDDBC3EAECA306A35C1B4EC3DAE729A343D5AD4D3`。三轮日志在 `Artifacts/M5_04/Runtime`，双槽在该目录下的 `Attempt01/User/Saved/SaveGames`。

[M6-01 性能采集](../M6_01_TrialPerformanceBaseline.md)

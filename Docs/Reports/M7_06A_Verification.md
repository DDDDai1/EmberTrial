# M7-06A 灰烬短刃命中音效验收

**通过。** 用户确认实机试听、实际命中与连击时序、独立游戏音乐和音效静音路由通过，测试后恢复原音量。反馈工时 **15 分钟**，无待修问题。此次反馈未记录实际地图名和独立游戏启动路径。[操作指南](../M7_06A_CinderBladeHitAudio.md)

| 检查 | 结果 |
| --- | --- |
| 用户试听 | 空挥无命中声，真实命中和连击无重复声，时序与听感正常。改变音乐或音效音量时，路由按预期响应。 |
| PIE 开发复验 | `L_ET_VerticalSlice` 中空挥保留训练目标 100 点生命。命中从 100 降至 75，`Serial=2` 下记录了一次 `SW_ET_CinderBladeHit` 播放请求。 |
| 双目标构建 | Editor 和 Win64 Development 构建均通过。 |
| 资产 | 新进程读回玩家 GA、Sound Wave 和距离衰减，**3 Valid、0 Invalid、0 Warnings**。 |
| 自动化 | 连段、闪避取消、音量配置和路由共 **4 项通过**。发布前另跑全量 **60 项通过、0 失败**。 |
| 版本 | 实现提交 `2baa408` 已进入当前 `origin/main` 的祖先历史。轻击 GA、两份 UE 音频资产、源 WAV 与同提交的三份 UI 资产均通过本机 Git LFS OID 核对。 |

开发记录在 `Artifacts/M7_06/SmokeHitAudio.log` 与 `CombatAudioRegression.log`。继续进入 [M7-07 Win64 Shipping 集成验证](../M7_07_ShippingCandidate.md)。

## 本次文档 Commit / Push

与发布候选指南、进度记录及运行脚本一并提交。候选包、日志和验收存档保存在 `Artifacts/`。

- 英文 `chore(release): verify hit audio and prepare Shipping QA`
- 中文 `chore(release): 验收命中音效并准备 Shipping 验证`

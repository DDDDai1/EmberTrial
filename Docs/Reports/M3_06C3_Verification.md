# M3-06C3 六件遗物集成包验收

2026-11-09通过。实际交付验证包为Win64 Development的`M3_06C3/Attempt01`。

## 构建与运行证据

- UAT的Build、Cook、Stage、Package与Archive均完成，返回0。UAT自身计时128.10秒。包内`.pak`、`.ucas`、`.utoc`均存在，游戏exe与本机Game Development构建产物的SHA-256一致。
- 打包前43项自动化全部Success，配置校验Errors 0／Warnings 0。当前HEAD `115bdd0`，C2测试完整源码与已验收版本一致，资产来源清单96条指纹全部匹配。
- 两轮由同一包的外层引导程序启动，进程不同，均从菜单正常退出。两轮请求1280×720，共用专用`M3PackageUser.ini`。日志未见Fatal error、Ensure condition failed或Accessed None。

| 本地时间 | 流程 | 证据 |
| --- | --- | --- |
| Run01 22:37:59–22:38:32 | 新会话Continue禁用，应用1.25倍灵敏度并保持Y轴不反转，从Entry出生，E进入祭坛 | 菜单、应用、出生与祭坛日志 |
| Run01 22:38:52–22:39:09 | 槽A生息余烬，槽B裂势印。F实际80点，训练目标架势100→20 | 两次装备提交成功与同一次F伤害日志 |
| Run01 22:39:16–22:39:55 | Checkpoint_A激活。真实死亡后Retry从该点出生，返回菜单后Continue也从该点出生 | 两次均恢复双槽，生命HUD均120/120 |
| Run02 22:40:28–22:40:55 | 新进程Continue禁用，六件空槽，从Entry出生。只读回1.25倍、不反转。F实际60点，目标架势100→40 | 独立进程记录、输入查询、Inventory快照和F日志 |
| 两轮实机 | 六件与详情滚动通过 | 用户实机反馈，运行日志佐证六件及页面打开，视觉布局按实机反馈验收 |

## 本地归档

- Attempt目录 `E:/UE_Project/Demo/Artifacts/Builds/M3_06C3/Attempt01`，含`UAT_Console.log`。
- 游戏exe `Windows/EmberTrial/Binaries/Win64/EmberTrial.exe`。脚本启动的外层引导程序为`Windows/EmberTrial.exe`。
- Runtime目录 `E:/UE_Project/Demo/Artifacts/M3_06C3/Runtime`。两组证据名为`Run01_20261109_223754_207`和`Run02_20261109_224025_303`，各含`.log`和`.json`。原始日志为UTC，表格已换算到北京时间。Run02于22:41:11退出。
- 同阶段`Acceptance/Summary.json`保留双exe指纹、进程与配置路径。`Acceptance/RuntimeExcerpt.log`保留关键原始行。

本批手动**30分钟**，构建等待**10分钟**。M3累计2878分钟（47小时58分），等待211分钟（3小时31分）。项目累计6289分钟（104小时49分），等待535分钟（8小时55分）。

M3按当前交付范围完成。后续进入关卡内容阶段，先做远程守卫的投射物飞行基础。[M4-01A操作指南](../M4_01A_ProjectileFlightFoundation.md)

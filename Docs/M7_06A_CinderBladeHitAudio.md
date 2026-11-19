# M7-06A 灰烬短刃命中音效

本批先校准玩家轻击的命中反馈。三段轻击各沿原有命中窗口和 Gameplay Effect 结算。命中实际扣血后，在接触点播放一声短打击。原音频采用金属瞬态和低频撞击两层合成，以短促、能听清实际命中为目标。

## 1. 工作区配置

| 对象 | 配置 |
| --- | --- |
| `GA_ET_LightAttack` | `Confirmed Hit Sound` 指向 `SW_ET_CinderBladeHit` |
| `SW_ET_CinderBladeHit` | 原始合成 WAV，单声道，48 kHz，16 位，205 ms。使用现有 `SC_ET_Effects` 音效类 |
| `SA_ET_CinderBladeHit` | 球形衰减，内半径 160 cm，外衰减段 900 cm |
| `ETGA_LightAttack` | 在现有伤害分支确认 Gameplay Effect 成功应用且生命实际下降时播放。一次命中窗口继续使用原目标去重。AI 两份轻击配置保持空音效引用 |

生成器和源 WAV 随项目保留。[来源清单](AssetManifest.csv)

开发复验通过 Editor、Win64 Development、三项资产校验和四项自动化。PIE 中空挥保留训练目标 100 点生命，有效轻击使其从 100 降至 75，命中与播放请求同为 `Serial=2`，只记录一次。实际听感及独立游戏音量路由按第 2 节验收。

## 2. 实机试听

1. 在 UE 播放 `L_ET_VerticalSlice`。开启 **输出日志**，需要配对时在控制台键入 `Log LogETLightAttack Verbose`。
2. 离开训练目标挥一刀。再靠近目标面向它，等上一段结束后单击。空挥应无短促命中声。有效命中应只有一声，日志显示 `Actual=25.0` 和同一 `Serial` 的 `Confirmed hit sound`。若装备提高伤害，记录实际数值。
3. 在正式路线实战做一次连续轻击。确认各段对应实际命中、音量不会盖过画面动作节奏，尤其检查连续命中是否感觉刺耳或过响。
4. 用独立游戏检查音量路由。菜单的 **设置 → 音量** 先记下当前值。在主音量正常时将音乐设为 0，确认有效命中仍有声。再恢复音乐并将音效设为 0，确认命中仍结算且音效静音。测试结束将主音量、音乐、音效恢复到原值。UE 的内嵌 PIE 用于定位命中，设置页的持久应用按项目原设计在独立游戏复查。
5. 若听感不符，把实际描述和日志行一起记录。建议描述瞬态是钝、尖、偏轻或拖尾，以及它相对动作和扣血的时序。

## 3. 反馈

```text
M7-06A 完成
测试用地图与启动方式：
空挥、真实命中和是否有重复声：
连击时的时序和最明显的听感：
独立游戏音乐／音效静音路由：
测试后原音量恢复：
问题或修改／工时：
```

## 4. Commit / Push

M7-05B-2 的菜单导航源码缺口按界面报告先补齐。本批包含 `ETGA_LightAttack.h` 和 `.cpp`、`GA_ET_LightAttack.uasset`、`Audio/Combat` 下的 Sound Wave 与 Sound Attenuation、源 WAV、生成器及批次文档。采样与开发复验日志位于本地 `Artifacts/`。

- 英文 `feat(audio): add confirmed Cinder Blade hit feedback`
- 中文 `feat(audio): 接入灰烬短刃的真实命中音效`

# M0-05C2-03A：手动新增闪避位移配置

日期：2026-09-28。引擎：UE 5.5.4。预计手动 **10–20 分钟**，含一次正常 C++ Build。

**C2-02 已验收通过，实际约 15 分钟。按你的要求，本批由你新增两个 C++ 可配置属性并检查编辑器显示；助手本轮没有修改工程源码。** 出现编译错误或属性异常时，将结果发来，我接手排查。

## 1. 本批结果与后续分工

完成本批后，已有 PracticeAction 组件的 Details 中会出现：

| 属性 | C++ 名称 | 默认值 | 含义 |
|---|---|---|---|
| Dodge Distance | DodgeDistance | **150.0** | 无障碍平地上的目标闪避距离，单位 cm，即 1.5 m |
| Dodge Duration | DodgeDuration | **0.30** | 位移持续时间，单位秒；不是整个闪避动画时长 |

**这一批只是配置准备：按 2 仍在原地播放，不产生位移，属于预期。** 两个属性暂时没有代码读取，改数值也不会改变动作表现。不要为了看到位移自行增加 Timeline / SetActorLocation / LaunchCharacter。

下一批 C2-03B 由助手实现较复杂的 CharacterMovement 位移、碰撞、暂停和中断清理，然后由你实机验证。C2-03A 通过不代表整个 C2-03 已完成。

### 下一批将采用的规则

- 当前动画是向角色自身右侧倾身，因此先验证**角色自身右侧**闪避；不是屏幕右侧，也不跟随镜头转向。方向在开始时固定。正式方向闪避留到后续战斗阶段扩展。
- 初始目标 150 cm / 0.30 秒；闪避 Montage 仍约 0.70 秒。位移先结束，动作状态继续保持到 Montage 结束，避免位移结束就提前放开输入。
- 位移交给 CharacterMovement 的程序化 Root Motion Source；现有动画的 Enable Root Motion 继续为 False。它与从动画根骨提取位移是两种来源。
- 墙壁、坡面会影响实际轨迹 / 距离，不强行穿墙到达 150 cm。暂停应冻结位移，受击打断、取消或 EndPlay 应移除本动作的位移并处理残留速度。

这些是下一批的实现与验收目标，尚未写入或验证。API 选择参考 [Epic UE 5.5 FRootMotionSource_ConstantForce](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/GameFramework/FRootMotionSource_ConstantForce?application_version=5.5)，实现时以本机 5.5.4 源码及测试为准。本轮不用添加 GAS 插件或额外模块。

## 2. 编辑前保存与关闭 UE

1. 停止 PIE，保存当前蓝图与地图。
2. 关闭 **EmberTrial 的 UE 编辑器**。Visual Studio 可以保留打开。
3. 等编辑器窗口完全退出，再开始改下面的头文件。

这次增加 UPROPERTY，会改变反射属性和类布局，所以仍使用正常 Build，不用 Live Coding。上次关闭编辑器的确认不代替这一次保存 / 关闭。

## 3. 在头文件里新增两个属性

### 3.1 找到准确位置

在 Visual Studio 打开：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\ETPracticeActionComponent.h
```

搜索 **AttackMaxHorizontalSpeed**。它位于 `private:` 区域，当前附近应是：

```cpp
    UPROPERTY(EditDefaultsOnly, Category = "M0|Practice Action", meta = (ClampMin = "0.0"))
    float AttackMaxHorizontalSpeed = 3.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient,
        Category = "M0|Practice Action", meta = (AllowPrivateAccess = "true"))
    EETPracticeActionState CurrentState = EETPracticeActionState::Idle;
```

在 `float AttackMaxHorizontalSpeed = 3.0f;` **后面**、CurrentState 的整组 `UPROPERTY` **前面**插入新属性。不要将代码插在 `UPROPERTY(...)` 与它原来修饰的成员之间。

### 3.2 需要你手动输入的完整新增代码

```cpp
    // Target travel distance on unobstructed, level ground, in centimeters.
    UPROPERTY(EditDefaultsOnly, Category = "M0|Practice Action|Dodge",
        meta = (ClampMin = "1.0", ClampMax = "300.0", Units = "cm"))
    float DodgeDistance = 150.0f;

    // Movement duration in seconds, separate from the montage duration.
    UPROPERTY(EditDefaultsOnly, Category = "M0|Practice Action|Dodge",
        meta = (ClampMin = "0.05", ClampMax = "0.50", Units = "s"))
    float DodgeDuration = 0.30f;
```

只新增上面两组属性。保留当前文件的其余内容，包括三个 Montage、CurrentState、回调与测试友元声明。不要用 C2-01 的旧整份示例覆盖当前文件，也不用改 `.cpp`、Character 或 Build.cs。

### 3.3 各部分的作用

| 写法 | 本项目中的含义 |
|---|---|
| `private:` | 其他 C++ 类不能直接随意改参数；组件内部使用 |
| `UPROPERTY` | 让 UE 反射系统识别字段，提供编辑器配置能力 |
| `EditDefaultsOnly` | 在蓝图默认配置中编辑；本批到角色蓝图里选中组件设置，不在场景实例上找 |
| `Category` | 将两个新属性放在 M0 → Practice Action → Dodge 分组 |
| `ClampMin / ClampMax` | 本轮编辑器可输入范围：距离 1–300 cm，时间 0.05–0.50 秒；这是 M0 调参范围，不是通用闪避规则 |
| `Units` | 为编辑器显示单位；代码中仍按 cm / 秒计算 |
| `150.0f / 0.30f` | C++ 默认值；末尾 `f` 表示 float 常量 |

这里不加 `Transient`，因为需要保存蓝图中的配置。也不加 `BlueprintReadWrite`：蓝图只负责配置，后续由 C++ 使用，无需额外开放运行时 Set 节点。编辑器边界不能代替运行时防御检查，下一批实现会处理无效值。

## 4. 保存并正常 Build

1. 保存头文件，确认 UE 编辑器已经关闭。
2. Visual Studio 顶部配置为 **Development Editor**，平台为 **Win64**。
3. 在 Solution Explorer 中找到 **EmberTrial 项目**，右键 → **Build**。不需要 Clean / Rebuild 整个引擎。
4. 等待完成，查看 **Output → Build** 的最终结果：成功、无编译错误。

若 VS 的文件树没有展开，使用解决方案 `E:\UE_Project\Demo\EmberTrial\EmberTrial.sln`。本轮只改现有头文件，不需要重新生成解决方案。

**如果失败，停在这里，把第一条编译错误及上下文发给我。** 不要删除 Binaries / Intermediate，不用反复创建类。错误列表可能混有 IntelliSense 提示，优先发 Output → Build 的实际文本。

## 5. 在角色蓝图中确认新属性

Build 成功后：

1. 重新打开 `E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject`。
2. 打开 `/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter`。
3. 在 Components 列表选择已有 **PracticeAction**。不要再添加组件，也不要选 Self 或 CharacterMovement。
4. Details 搜索 `Dodge`。除了原有 Dodge Montage，应看到 **Dodge Distance** 与 **Dodge Duration**。
5. 确认 Distance=**150.0 cm**、Duration=**0.30 s**。界面也可能省略尾随零，显示 150 / 0.3，同样正确。
6. 查看原来的三个 Montage 引用仍正确，Auto Activate=True。
7. Compile、Save 角色蓝图。

不需要新增两个同名蓝图变量；它们必须来自 C++ 组件。不要为了让它们出现而移除 / 重加组件。若看不到，发组件 Details 截图和 Build 结果，我核对反射和加载情况。

本批保留默认值 **150 / 0.30**，后续位移接入后再结合实机效果调参。

## 6. 简短回归

这次没有接入位移，确认添加属性没有破坏已有功能即可，不必重复 C2-02 全部次数：

1. 站定按 1/2/3：正常播放并结束，2 仍原地。
2. 按 1 后迅速按 3：受击仍能打断，结束后恢复移动 / 跳跃。
3. 动作期间 E 被拒绝，结束后正常交互；P 暂停 / 恢复正常。
4. 基础移动 / 转向 / 跳跃正常，Output Log 未出现新错误。
5. 确认参数恢复为 150 / 0.30，资产和头文件已保存。

当前日志仍为 C2-02 的 Begin / Completed 等。**本批不应期待新位移日志、实际距离或撞墙闪避结果**，这些在 C2-03B 接入后测试。

## 7. 反馈模板

```text
M0-05C2-03A 完成 / 卡在第 __ 节
新增属性所在文件：ETPracticeActionComponent.h
两个属性已放在 private 区域，原代码保留：是 / 实际情况
关闭 UE 后正常 C++ Build：成功 / 完整错误
PracticeAction 可见 Dodge Distance / Dodge Duration：是 / 实际情况
最终配置：150 cm / 0.30 s（或填写实际值）
三个 Montage / Auto Activate 保持正确：是
角色蓝图 Compile：通过 / 错误原文
1/2/3 原地播放、受击打断与结束恢复：通过 / 实际结果
E / P / 移动 / 转向 / 跳跃：正常 / 实际结果
头文件与蓝图已保存：是
实际用时：约 __ 分钟
```

编译错误优先发 **文字**：第一条 `error` 及前后约 15–30 行，包含文件名 / 行号；参数不可见或位置不确定，发 **截图**：组件选中状态、Details 和相关代码上下文。动画 / 时序异常再补按键顺序和短视频。不需要先判断根因，告诉我卡在哪一步即可。

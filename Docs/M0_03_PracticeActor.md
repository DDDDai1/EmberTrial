# M0-03：手动创建 C++ Actor 与蓝图子类

日期：2026-09-19。引擎：UE 5.5.4。状态：任务已下发，尚未实现或验收。

预计 45–90 分钟，按实际用时记录。你负责创建类、实现基础代码、配置蓝图和场景；我负责审查与排错。下方代码只放在本指南中，没有代写到工程 Source，也没有提前生成蓝图资产。

## 1. 本轮要做出的效果

在现有 ThirdPersonMap 的出生点附近放两个蓝图物体。它们共用一个 C++ 父类和同一个蓝图资产，但拥有不同的实例标签。

点击 Play 后，两个物体各通过一次蓝图函数调用，在 Output Log 输出自己的对象名和标签。角色原有移动、转向、跳跃仍正常。

```text
C++ AETPracticeActor
    定义组件、可配置标签、可调用函数
        ↓
BP_ETPracticeActor
    配置 Cube 外观、默认标签，在 BeginPlay 调用 C++ 函数
        ↓
场景实例 A / B
    分别覆盖标签为 Target_A / Target_B
        ↓
Play → 各输出一条带 [M0-03] 前缀的日志
```

这次只验证 C++ / Blueprint 的配合方式。它是练习 Actor，不是最终敌人、伤害或交互框架；后续正式系统仍按计划设计。

## 2. 创建 C++ 类

1. 在 UE 5.5.4 中保存当前工程。
2. 使用“工具 / Tools → 新建 C++ 类 / New C++ Class”。也可以在内容浏览器的 C++ Classes 区域使用新建 C++ 类入口。
3. 父类选择 **Actor**。
4. 名称填 **ETPracticeActor**，不要手动加 `A` 前缀；生成的 C++ 类型应为 `AETPracticeActor`。
5. 模块选择 **EmberTrial**。本轮沿用模板的平铺目录，预期文件为：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\ETPracticeActor.h
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\ETPracticeActor.cpp
```

如果向导已自动生成 Public / Private 子目录，不要自行移动文件，把实际路径告诉我即可核对。关键是文件在 EmberTrial 模块中。

6. 若向导自动触发编译，先等待它结束。然后保存并关闭 UE 编辑器，再在 Visual Studio 中编辑新类。原因是本轮会新增反射属性与函数，使用关闭编辑器后的正常编译，避免旧类布局留在编辑器中。

若创建类后出现报错，保留生成文件和错误原文，交给我检查，不需要删除类重建。

## 3. 你实现的 C++ 设计契约

| 成员 | 类型 / 要求 | 用途 |
|---|---|---|
| SceneRoot | `USceneComponent`，构造函数中创建并设为根组件 | 定义 Actor 的组件层级 |
| Mesh | `UStaticMeshComponent`，附着到 SceneRoot | 模型由蓝图配置；练习中关闭碰撞 |
| PracticeLabel | `FText`；`EditAnywhere` + `BlueprintReadOnly` | 可改蓝图默认值和场景实例值；蓝图节点不能直接 Set |
| PrintPracticeInfo | `UFUNCTION(BlueprintCallable)`，无参数、无返回值 | 打印当前对象名与 PracticeLabel |
| Tick | 关闭 `PrimaryActorTick.bCanEverTick` | 本轮没有逐帧更新需求 |

构造函数创建默认组件，模型资产在蓝图中选择；日志函数由蓝图 BeginPlay 明确调用一次，不放进 Tick。函数有输出日志的副作用，使用带执行线的 BlueprintCallable，不做成 Pure 节点。

下面给出完整的参考结构，便于你手写后对照。也可以先按契约自行实现，再检查差异。若保留向导生成的注释没有影响；本参考类不覆盖 BeginPlay / Tick，不要仅删除它们的声明却保留旧定义，或反过来。

### 3.1 ETPracticeActor.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ETPracticeActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class EMBERTRIAL_API AETPracticeActor : public AActor
{
    GENERATED_BODY()

public:
    AETPracticeActor();

    UFUNCTION(BlueprintCallable, Category = "Practice")
    void PrintPracticeInfo();

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Practice",
        meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Practice",
        meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Practice",
        meta = (AllowPrivateAccess = "true"))
    FText PracticeLabel;
};
```

`.generated.h` 必须是最后一个 `#include`。`EMBERTRIAL_API` 对应当前模块名；类名、构造函数名与 cpp 定义必须一致。

### 3.2 ETPracticeActor.cpp

```cpp
#include "ETPracticeActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

AETPracticeActor::AETPracticeActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot.Get());

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(SceneRoot.Get());
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    PracticeLabel = NSLOCTEXT("EmberTrial", "PracticeLabel_Default", "Practice Actor");
}

void AETPracticeActor::PrintPracticeInfo()
{
    UE_LOG(LogTemp, Display, TEXT("[M0-03] Actor=%s Label=%s"),
        *GetName(), *PracticeLabel.ToString());
}
```

本轮用 LogTemp 和固定前缀便于检索；正式系统会按模块建立日志类别。现有 EmberTrial.Build.cs 已依赖 Engine，此类不需要新增模块依赖。

属性的编辑权限和 Blueprint 读写权限是两个维度：`EditAnywhere` 允许在编辑器里调整值；`BlueprintReadOnly` 限制蓝图图表直接写值，两者并不冲突。`AllowPrivateAccess` 让标记为 private 的成员按声明暴露给蓝图，不是把它改为 C++ public。反射和蓝图接口说明参见 [Epic UE 5.5 文档](https://dev.epicgames.com/documentation/en-us/unreal-engine/exposing-gameplay-elements-to-blueprints-visual-scripting-in-unreal-engine?application_version=5.5)。

### 3.3 正常编译

1. 确保 UE 编辑器已关闭，保存 h / cpp。
2. 在 Visual Studio 打开 `E:\UE_Project\Demo\EmberTrial\EmberTrial.sln`。
3. 选择 **Development Editor / Win64**，右键游戏项目 **EmberTrial → 生成 / Build**。
4. 输出显示成功后，重新打开现有 `.uproject`。

如果构建失败，发“输出 → 生成”中的首个实际错误及上下文；我检查新文件即可，不必把整份代码复制进聊天。

## 4. 创建并配置蓝图子类

1. 在内容浏览器中找到 C++ Classes → EmberTrial → ETPracticeActor。未显示 C++ 类时，在浏览器设置中启用显示 C++ Classes。
2. 右键该类，选择基于它创建蓝图；也可新建 Blueprint Class，在 All Classes 中搜索 ETPracticeActor 并选为父类。
3. 蓝图命名 **BP_ETPracticeActor**，保存到：

```text
/Game/EmberTrial/Blueprints/Practice/BP_ETPracticeActor

磁盘预期位置：
E:\UE_Project\Demo\EmberTrial\Content\EmberTrial\Blueprints\Practice\BP_ETPracticeActor.uasset
```

4. 打开蓝图，在 Components 中选中继承来的 **Mesh**，把 Static Mesh 设为引擎基础 **Cube**。资源路径为 `/Engine/BasicShapes/Cube.Cube`；若列表找不到，在资源选择器的设置中启用显示 Engine Content，再搜索 Cube。
5. 确认 Mesh 的 Collision Enabled 为 No Collision，Simulate Physics 关闭；在视口中应能看见一个立方体。模型尺寸先用默认值即可。
6. 点击 **Class Defaults / 类默认值**，找到 Practice → Practice Label，设为 **Default Target**。
7. 打开 Event Graph，只连接下面这条执行链：

```text
Event BeginPlay ──白色执行线──> Print Practice Info
                                      Target = Self
```

`Print Practice Info` 是 C++ 中 PrintPracticeInfo 的友好显示名。从 BeginPlay 执行引脚拖线搜索该函数即可。保持 Target 为 Self，这样每个场景实例处理自己的标签。

8. 编译并保存蓝图。此时只创建 **一个** BP_ETPracticeActor 资产，下面在场景中放它的两个实例。

## 5. 放置实例并验证

1. 回到现有 ThirdPersonMap，将 BP_ETPracticeActor 拖入场景两次，放在出生点附近容易看到的位置，底部贴近地面。
2. 在 World Outliner 分别命名为 **Practice_A** 和 **Practice_B**，方便查找；这个编辑器标签与 C++ GetName 输出的运行时对象名不是同一概念。
3. 分别选中两个实例，在 Details → Practice 中将 Practice Label 设为 **Target_A** 和 **Target_B**。这一步修改场景实例，不再修改蓝图 Class Defaults。
4. 保存关卡与蓝图。
5. 打开 Output Log，在搜索框输入 `[M0-03]`。可使用底部 Output Log 入口或“窗口 / Window → 开发者工具 / Developer Tools → Output Log”。确保 Display 日志没有被过滤。
6. 先清空当前日志显示，再点击 Play。预期出现两条如下形式的消息：

```text
LogTemp: Display: [M0-03] Actor=<实例A运行时对象名> Label=Target_A
LogTemp: Display: [M0-03] Actor=<实例B运行时对象名> Label=Target_B
```

输出顺序及运行时对象名不作要求；每个实例在这一次 Play 中各一条，标签必须正确。停留几秒不应持续刷同样的日志。

7. 验证模板角色移动 / 转向 / 跳跃仍正常；两个练习物体能看见，但不会挡路。
8. 停止 Play，把 A 的 Practice Label 改为 **Target_A_Changed**，保存，清空日志显示并再次 Play。应输出新标签；**修改标签不需要重新编译 C++**。
9. 停止运行并保存，保留场景和蓝图供我核对。

## 6. 常见卡点与定位入口

| 现象 | 优先检查 / 发给我的材料 |
|---|---|
| 找不到新 C++ 类 | 是否 Build 成功、是否重启 UE、模块是否为 EmberTrial；给实际 h / cpp 路径 |
| 编译提示类 / 函数未声明或重复定义 | 生成模板的旧声明 / 定义是否残留，类名是否一致；给首个错误原文 |
| 蓝图没有 Print Practice Info | UFUNCTION 声明、正常重编译与重启、蓝图父类是否正确；发 Class Settings 或节点搜索截图 |
| private 属性相关 UHT 错误 | UPROPERTY 是否带 AllowPrivateAccess，meta 语法和括号是否正确 |
| 看不到立方体 | 实际放的是蓝图子类还是裸 C++ 类；Mesh 是否选了 Cube，Actor 是否在可见位置 |
| 没有日志 | BeginPlay 执行线、Self 目标、是否在当前地图放置实例、Output Log 过滤条件 |
| 两条日志标签相同 | 是否只修改 Class Defaults，没有分别覆盖场景实例；是否选中了正确对象 |
| 每帧持续输出 | 是否误接 Event Tick，或还有其他 PrintPracticeInfo 调用 |

你不需要逐项自行排完才报告。简单描述卡在哪一步，我就能读取本地代码与日志；蓝图连线问题附一张相关节点截图更直观。

## 7. 验收与复盘

- [ ] 新类由你创建，C++ 生成通过；不改模板角色 / GameMode 行为。
- [ ] 组件在 C++ 构造函数创建，Static Mesh 在蓝图选择，Tick 关闭。
- [ ] BlueprintCallable 函数由蓝图 BeginPlay 调用，两个实例各输出一次。
- [ ] Target_A / Target_B 的实例数据不混淆。
- [ ] 修改 A 的标签后，不重新编译 C++ 就能看到新输出。
- [ ] 场景和蓝图已保存，模板角色基础操作正常。

完成后我们用这次代码复盘：为何用 Actor 而非 Character；组件指针的 VisibleAnywhere 和标签的 EditAnywhere 分别允许编辑什么；为什么 BlueprintReadOnly 仍能在 Details 修改；C++ 默认值、蓝图默认值和场景实例覆盖是什么关系；为何这次无需 Tick。

回复格式：

```text
M0-03 完成
Build：成功 / 具体错误
第一次日志：Target_A 和 Target_B，各一条 / 实际结果
修改实例标签后：输出 Target_A_Changed，不需要重编译 / 实际结果
移动 / 转向 / 跳跃：正常 / 具体异常
蓝图和场景：已保存
实际用时：约 __ 分钟
```

这一步通过后再进入 M0-04 的最小交互 / UMG 原型。当前不要求你提前实现后续系统。

#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogETEditorModule, Log, All);

// 模块生命周期属于进程级加载；这里不创建World、Pawn、Presenter或Widget。
class FEmberTrialEditorModule final : public FDefaultModuleImpl
{
public:
    // 原生Validator由引擎发现，不能在此重复手动注册。
    virtual void StartupModule() override
    {
        UE_LOG(LogETEditorModule, Display, TEXT("[M1-07C1] Editor validation module ready"));
    }
};

IMPLEMENT_MODULE(FEmberTrialEditorModule, EmberTrialEditor)

#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogETEditorModule, Log, All);

class FEmberTrialEditorModule final : public FDefaultModuleImpl
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogETEditorModule, Display,
            TEXT("[M1-07C1] Editor validation module ready"));
    }
};

IMPLEMENT_MODULE(FEmberTrialEditorModule, EmberTrialEditor)
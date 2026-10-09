#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogETUIModule, Log, All);

class FEmberTrialUIModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogETUIModule, Display, TEXT("[M1-02A] EmberTrialUI module ready"));
    }

    virtual void ShutdownModule() override
    {
        UE_LOG(LogETUIModule, Display, TEXT("[M1-02A] EmberTrialUI module shutdown"));
    }
};

IMPLEMENT_MODULE(FEmberTrialUIModule, EmberTrialUI)
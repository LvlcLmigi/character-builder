#include "Outfits.h"
#include "UI.h"
#include "Wardrobe.h"

namespace
{
    void SetupLog()
    {
        auto logsFolder = SKSE::log::log_directory();
        if (!logsFolder) {
            SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
        }
        auto logPath = *logsFolder / "CharacterStudio.log";
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath.string(), true);
        auto log = std::make_shared<spdlog::logger>("log", std::move(sink));
#ifndef NDEBUG
        log->set_level(spdlog::level::trace);
        log->flush_on(spdlog::level::trace);
#else
        log->set_level(spdlog::level::info);
        log->flush_on(spdlog::level::info);
#endif
        spdlog::set_default_logger(std::move(log));
    }

    void OnSKSEMessage(SKSE::MessagingInterface::Message* a_message)
    {
        switch (a_message->type) {
        case SKSE::MessagingInterface::kPostLoad:
            // Every SKSE plugin (including SKSE Menu Framework) is loaded by now.
            UI::Register();
            break;
        case SKSE::MessagingInterface::kDataLoaded:
            // All plugins (.esp/.esm/.esl) are loaded, so every armor form exists.
            Wardrobe::BuildCatalog();
            Outfits::LoadAll();
            break;
        default:
            break;
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
    SKSE::Init(a_skse);
    SetupLog();

    const auto* plugin = SKSE::PluginDeclaration::GetSingleton();
    logger::info("{} {} loading", plugin->GetName(), plugin->GetVersion().string());

    if (!SKSE::GetMessagingInterface()->RegisterListener(OnSKSEMessage)) {
        logger::critical("Could not register for SKSE messages");
        return false;
    }
    return true;
}

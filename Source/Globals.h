// Copyright Nodos AS. All Rights Reserved.
#include <Nodos/PluginAPI.h>
#include <nosSysSettings/nosSettingsSubsystem.h>
#include <nosSysSettings/EditorEvents_generated.h>
#include <unordered_map>
#include <Nodos/Plugin.hpp>

namespace nos::sys::settings
{
struct EntryManager;
struct SettingsEditorManager;
extern std::unique_ptr<EntryManager> GSettingsEntryManager;
extern std::unordered_map<uint32_t, std::unique_ptr<nosSettingsSubsystem>> GExportedAPIVersions;
static constexpr char DEFAULT_SETTINGS_ENTRY_NAME[] = "default";
}
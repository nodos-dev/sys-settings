/*
 * Copyright MediaZ Teknoloji A.S. All Rights Reserved.
 */

#ifndef NOS_SETTINGS_SUBSYSTEM_H_INCLUDED
#define NOS_SETTINGS_SUBSYSTEM_H_INCLUDED
#include "Nodos/Types.h"

// NOS_RESULT_SUCCESS: Item value will be serialized to the WriteDirectories
// NOS_RESULT_FAIL: Default item value will be serialized to the WriteDirectories
typedef nosResult(*nosPfnSettingsEntryUpdate)(const char* entryName, nosBuffer itemValue);
typedef uint64_t nosSettingsEntryId;

#if NOS_HAS_CPLUSPLUS_20
typedef nos::fb::Visualizer* nosSettingsEditorVisualizer;
#else
typedef void* nosSettingsEditorVisualizer;
#endif

typedef struct nosSettingsEntryParams {
	nosName TypeName;
	// Default value of the buffer
	// If nosPfnSettingsEntryUpdate callback returns NOS_SETTINGS_ENTRY_NON_COMPATIBLE, this will be saved
	// Don't forget to free this buffer
	nosBuffer DefaultValueBuffer;
	const char* EntryName; // If NULL, "default" will be used
	nosPfnSettingsEntryUpdate UpdateCallback; // Callback to update the entry value
	nosSettingsEditorVisualizer Visualizer; // Visualizer for the entry in editor
	const char* DisplayName;
	// Target module name, editor can decide where to place this entry
	// For example, if this is "nos.sys.device", editor can place this entry under "Devices" section
	const char* UiTargetName;
} nosSettingsEntryParams;

typedef struct nosSettingsSubsystem
{
	nosResult(NOSAPI_CALL *RegisterEntry)(const nosSettingsEntryParams* parameters);
	nosResult(NOSAPI_CALL* UpdateEntryValue)(const char* entryName, nosBuffer value);
	// An entry can only be unregistered by the plugin it's registered from
	void(NOSAPI_CALL* UnregisterEntry)(const char* entryName);
} nosSettingsSubsystem;

#pragma region Helper Declarations & Macros
// Make sure these are same with nossys file.
#define NOS_SETTINGS_SUBSYSTEM_NAME "nos.sys.settings"

#define NOS_SETTINGS_SUBSYSTEM_VERSION_MAJOR 2
#define NOS_SETTINGS_SUBSYSTEM_VERSION_MINOR 0

extern struct nosPluginInfo nosSettingsModuleInfo;
extern nosSettingsSubsystem* nosSettings;

#define NOS_SETTINGS_INIT()                                                                                              \
	nosPluginInfo nosSettingsModuleInfo;                                                                                 \
	nosSettingsSubsystem* nosSettings = nullptr;

#define NOS_SETTINGS_IMPORT() NOS_IMPORT_DEP(NOS_SETTINGS_SUBSYSTEM_NAME, nosSettingsModuleInfo, nosSettings)

#if NOS_HAS_CPLUSPLUS_20
#include "Types_generated.h"
#include <Nodos/Plugin.hpp>

namespace nos::sys::settings {
	inline nosResult RegisterEntry(std::string const& entryName, std::string const& typeName, nosPfnSettingsEntryUpdate updateCallback, std::optional<nosBuffer> defaultVal = std::nullopt, std::optional<std::string> displayName = std::nullopt, std::optional<std::string> targetName = std::nullopt, std::optional<nos::fb::TVisualizer> visualizer = std::nullopt) {
		nosSettingsEntryParams params{};
		params.EntryName = entryName.empty() ? NULL : entryName.c_str();
		params.TypeName = nos::Name(typeName);
		params.UpdateCallback = updateCallback;

		if (displayName.has_value() && !displayName->empty())
			params.DisplayName = displayName->c_str();
		else 
			params.DisplayName = params.EntryName;
		
		if (defaultVal.has_value())
			params.DefaultValueBuffer = *defaultVal;

		if (targetName.has_value() && !targetName->empty())
			params.UiTargetName = targetName->c_str();
		
		nos::Buffer visualizerBuf;
		if (visualizer.has_value()) {
			visualizerBuf = nos::Buffer::From(*visualizer);
			params.Visualizer = visualizerBuf.As<nos::fb::Visualizer>();
		}

		return nosSettings->RegisterEntry(&params);
	}

	inline void UnregisterEntry(std::string const& entryName) {
		nosSettings->UnregisterEntry(entryName.c_str());
	}
}
#endif // NOS_HAS_CPLUSPLUS_20

#pragma endregion


#endif // NOS_SETTINGS_SUBSYSTEM_H_INCLUDED
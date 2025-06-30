/*
 * Copyright MediaZ Teknoloji A.S. All Rights Reserved.
 */

#ifndef NOS_SETTINGS_SUBSYSTEM_H_INCLUDED
#define NOS_SETTINGS_SUBSYSTEM_H_INCLUDED
#include "Nodos/Types.h"

// NOS_RESULT_SUCCESS: Item value will be serialized to the WriteDirectories
// NOS_RESULT_FAIL: Default item value will be serialized to the WriteDirectories
typedef nosResult(*nosPfnSettingsEntryUpdate)(nosName entryName, nosBuffer itemValue);
typedef uint64_t nosSettingsEntryId;

#if NOS_HAS_CPLUSPLUS_20
typedef nos::fb::Visualizer* nosSettingsEditorVisualizer;
#else
typedef void* nosSettingsEditorVisualizer;
#endif

typedef enum nosSettingsFileDirectory {
	NOS_SETTINGS_FILE_DIRECTORY_LOCAL = 1 << 0, // Module's root folder
	NOS_SETTINGS_FILE_DIRECTORY_WORKSPACE = 1 << 1, // Engine's config folder
	NOS_SETTINGS_FILE_DIRECTORY_GLOBAL = 1 << 2	// AppData folder
} nosSettingsFileDirectory;

typedef uint32_t nosSettingsFileDirectoryFlag;

typedef struct nosSettingsEntryParams {
	nosName TypeName;
	// Default value of the buffer
	// If nosPfnSettingsEntryUpdate callback returns NOS_SETTINGS_ENTRY_NON_COMPATIBLE, this will be saved
	// Don't forget to free this buffer
	nosBuffer DefaultValueBuffer;
	nosName EntryName; // If NULL, "default" will be used
	nosSettingsFileDirectoryFlag WriteDirectories; // Directory flags to determine where this entry will be stored
	nosPfnSettingsEntryUpdate UpdateCallback; // Callback to update the entry value
	bool IsEditableFromEditor; // If true, this entry will be editable from editor
	nosSettingsEditorVisualizer Visualizer; // Visualizer for the entry in editor
	nosName DisplayName;
	// Target module name, editor can decide where to place this entry
	// For example, if this is "nos.sys.device", editor can place this entry under "Devices" section
	nosName TargetName;
} nosSettingsEntryParams;

typedef struct nosSettingsSubsystem
{
	nosResult(NOSAPI_CALL *RegisterEntry)(nosSettingsEntryParams* parameters);
	nosResult(NOSAPI_CALL* UpdateEntryValue)(nosName entryName, nosBuffer value);
	// An entry can only be unregistered by the plugin it's registered from
	void(NOSAPI_CALL* UnregisterEntry)(nosName entryName);
} nosSettingsSubsystem;

#pragma region Helper Declarations & Macros
// Make sure these are same with nossys file.
#define NOS_SETTINGS_SUBSYSTEM_NAME "nos.sys.settings"

#define NOS_SETTINGS_SUBSYSTEM_VERSION_MAJOR 1
#define NOS_SETTINGS_SUBSYSTEM_VERSION_MINOR 0

extern struct nosPluginInfo nosSettingsModuleInfo;
extern nosSettingsSubsystem* nosSettings;

#define NOS_SETTINGS_INIT()                                                                                              \
	nosPluginInfo nosSettingsModuleInfo;                                                                                 \
	nosSettingsSubsystem* nosSettings = nullptr;

#define NOS_SETTINGS_IMPORT() NOS_IMPORT_DEP(NOS_SETTINGS_SUBSYSTEM_NAME, nosSettingsModuleInfo, nosSettings)

#if NOS_HAS_CPLUSPLUS_20
#include "Types_generated.h"
#include <Nodos/Name.hpp>

namespace nos::sys::settings {
	inline nosResult RegisterEntry(std::string const& entryName, std::string const& typeName, nosPfnSettingsEntryUpdate updateCallback, std::optional<nosBuffer> defaultVal = std::nullopt, nosSettingsFileDirectoryFlag writeDirectories = NOS_SETTINGS_FILE_DIRECTORY_WORKSPACE, bool editableFromEditor = true, std::optional<std::string> displayName = std::nullopt, std::optional<std::string> targetName = std::nullopt, std::optional<nos::fb::TVisualizer> visualizer = std::nullopt) {
		nosSettingsEntryParams params{};
		params.EntryName = nos::Name(entryName);
		params.TypeName = nos::Name(typeName);
		params.UpdateCallback = updateCallback;

		if (displayName.has_value())
			params.DisplayName = nos::Name(*displayName);
		else 
			params.DisplayName = params.EntryName;
		
		if (defaultVal.has_value())
			params.DefaultValueBuffer = *defaultVal;
		
		params.WriteDirectories = writeDirectories;
		params.IsEditableFromEditor = editableFromEditor;

		if (targetName.has_value())
			params.TargetName = nos::Name(*targetName);
		
		nos::Buffer visualizerBuf;
		if (visualizer.has_value()) {
			visualizerBuf = nos::Buffer::From(*visualizer);
			params.Visualizer = visualizerBuf.As<nos::fb::Visualizer>();
		}

		return nosSettings->RegisterEntry(&params);
	}

	inline void UnregisterEntry(std::string const& entryName) {
		nosSettings->UnregisterEntry(nos::Name(entryName));
	}
}
#endif // NOS_HAS_CPLUSPLUS_20

#pragma endregion


#endif // NOS_SETTINGS_SUBSYSTEM_H_INCLUDED
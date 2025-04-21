// Copyright Nodos AS. All Rights Reserved.
#include <Nodos/PluginAPI.h>
#include <nosSettingsSubsystem/nosSettingsSubsystem.h>
#include <nosSettingsSubsystem/Types_generated.h>
#include <unordered_map>
#include <Nodos/Helpers.hpp>

namespace nos::sys::settings
{
struct SettingsFileManager
{
	struct SettingsFile
	{
		// Version -> EntryName -> {TypeName, Buffer}
		std::unordered_map<nosName, std::unordered_map<std::string, std::pair<std::string, nos::Buffer>>> Entries;
		nosSettingsFileDirectory Directory;
	};
	// Each module has a settings file
	// ModuleName -> [SettingsFile]
	std::unordered_map<std::string, std::vector<SettingsFile>> SettingsFiles;
	std::mutex SettingsFilesMutex;
	SettingsFile& FindOrCreateSettingsFile(nosPluginInfo const& module, nosSettingsFileDirectory dir);
	std::filesystem::path GetSettingsFilePath(nosSettingsFileDirectory dir, const nosPluginInfo& moduleInfo) const;
	nosResult ReadSettingsFile(std::filesystem::path filePath, SettingsFile& file);
	nosResult ReadSettings(nosSettingsEntryParams* params);
	nosResult WriteSettings(const nosSettingsEntryParams* params);
	nosResult WriteSettings(const nosSettingsEntryParams* params, const nosPluginInfo& callingModule);
	nosResult WriteSettingsFile(const SettingsFile& settingsFile, const nosPluginInfo& info) const;
};
}
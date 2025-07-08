// Copyright Nodos AS. All Rights Reserved.
#include <Nodos/PluginAPI.h>
#include <nosSettingsSubsystem/nosSettingsSubsystem.h>
#include <nosSettingsSubsystem/Types_generated.h>
#include <nosSettingsSubsystem/EditorEvents_generated.h>
#include <unordered_map>
#include <Nodos/Helpers.hpp>
#include "EntryManager.h"
#include "Globals.h"

namespace nos::sys::settings {
NOS_REGISTER_NAME_SPACED(SETTINGS_ENTRY_TYPENAME, nos::sys::settings::SettingsEntry::GetFullyQualifiedName());
NOS_REGISTER_NAME_SPACED(SETTINGS_LIST_TYPENAME, nos::sys::settings::SettingsList::GetFullyQualifiedName());

std::filesystem::path GetFilePathFromEntry(nos::Name pluginName, util::SemVer pluginVer, editor::SettingsEntryFileDirectory dir) {
	auto fileName = pluginName.AsString() + "-" + std::string(pluginVer) + ".json";
	switch (dir)
	{
	case editor::SettingsEntryFileDirectory::LOCAL:
	{
		return std::filesystem::path(GSettingsEntryManager->PluginVersions[pluginName].second.RootFolderPath) / fileName;
	}
	case editor::SettingsEntryFileDirectory::WORKSPACE:
	{
		return std::filesystem::path(nosEngine.GetEnginePath(NOS_ENGINE_PATH_CONFIG)) / fileName;
	}
	case editor::SettingsEntryFileDirectory::GLOBAL:
	{
		return std::filesystem::path(nosEngine.GetEnginePath(NOS_ENGINE_PATH_APPDATA)) / fileName;
	}
	default:
		nosEngine.LogE("The file location requested is not supported by nosSettingsSubsystem.");
		return std::filesystem::path();
	}
}

std::string GetPluginSettingsFileName(nosPluginInfo const& Plugin)
{
	return std::string(nosEngine.GetString(Plugin.Id.Name)) + "-" + nosEngine.GetString(Plugin.Id.Version);
}


std::vector<std::string> Split(const std::string& str, const std::string& delim)
{
	std::vector<std::string> result;
	size_t start = 0;
	size_t end = 0;
	while ((end = str.find(delim, start)) != std::string::npos)
	{
		result.push_back(str.substr(start, end - start));
		start = end + delim.length();
	}
	result.push_back(str.substr(start));
	return std::move(result);
}

std::vector<std::string> Split(const std::string& str, char delim) {
	return Split(str, std::string(1, delim));
}

nos::util::SemVer GetPluginVersionFromName(std::string const& pluginName)
{
	auto const& parts = Split(pluginName, '-');
	if (parts.size() < 2) {
		nosEngine.LogE("Invalid plugin name format: %s", pluginName.c_str());
		return nos::util::SemVer(0, 0, 0); // Return a default version
	}
	return nos::util::SemVer::ParseFrom(parts[1]);
}

nosResult EntryManager::ReadSettingsFile(std::filesystem::path filePath, ReadEntryList& readEntries)
{
	char* json = nullptr;
	// Read to JSON
	{
		std::ifstream file(filePath);
		if (!file || !file.is_open()) {
			return NOS_RESULT_NOT_FOUND;
		}

		std::stringstream buffer;
		buffer << file.rdbuf();  // Read the entire file into the buffer
		file.close();

		std::string content = buffer.str();  // Convert buffer to string
		json = (char*)malloc(content.size() + 1); // Allocate memory (+1 for null terminator)
		std::memcpy(json, content.c_str(), content.size() + 1); // Copy content to json
	}

	auto data = GenerateBufferFromJson(NSN_SETTINGS_LIST_TYPENAME, json);
	if (!data) {
		nosEngine.LogDE("Possible reasons;\n1) File has entries with types that are either changed or not registered yet.\n2) File is corrupted", "Failed to generate buffer from JSON for nosSettingsSubsystem");
		return NOS_RESULT_FAILED;
	}
	free(json);

	const auto& rootTable = data->As<nos::sys::settings::SettingsList>();
	flatbuffers::Verifier verifier((uint8_t*)data->Data(), data->Size());
	if (!rootTable->Verify(verifier)) {
		nosEngine.LogW("Failed to verify the settings file: %s", filePath.c_str());
		return NOS_RESULT_FAILED;
	}

	std::unique_lock lock(readEntries.FileMutex);
	readEntries.Entries.clear();
	if (!rootTable->settings())
		return NOS_RESULT_SUCCESS;

	for (auto it : *rootTable->settings()) {
		if (!it)
			continue;

		auto ver = GetPluginVersionFromName(filePath.filename().string());
		readEntries.Entries[it->entry_name()->str()].push_back({});
		auto& entry = readEntries.Entries[it->entry_name()->str()].back();
		entry = { ver , {nos::Name(it->type_name()->c_str()), nos::Buffer(it->data())} };
	}

	return NOS_RESULT_SUCCESS;
}

nosResult EntryManager::WriteSettingsFile(nos::Name pluginName, util::SemVer pluginVer, editor::SettingsEntryFileDirectory dir, const ReadEntryList& entries) const
{
	std::ofstream file(GetFilePathFromEntry(pluginName, pluginVer, dir));
	flatbuffers::FlatBufferBuilder builder;

	if (!file || !file.is_open()) {
		nosEngine.LogE("Settings subsystem couldn't open file for writing");
		return NOS_RESULT_FAILED;
	}

	TSettingsList settingsList;
	for (auto const& [entryName, pluginVerAndEntryVal] : entries.Entries) {
		for(auto const& [entryVer, entryVal] : pluginVerAndEntryVal) {
			if (entryVer != pluginVer)
				continue;
			settingsList.settings.push_back(std::make_unique<nos::sys::settings::TSettingsEntry>());
			auto& tEntry = settingsList.settings.back();
			tEntry->entry_name = entryName;
			tEntry->type_name = entryVal.first.AsString();
			tEntry->data = entryVal.second;
		}
	}

	auto data = nos::Buffer::From(settingsList);

	auto json = GenerateJsonFromBuffer(NSN_SETTINGS_LIST_TYPENAME, data);
	if (!json) {
		nosEngine.LogE("Failed to generate JSON from buffer for nosSettingsSubsystem");
		return NOS_RESULT_FAILED;
	}

	file << json->AsCStr();
	file.close();
	return NOS_RESULT_SUCCESS;
};

nosResult EntryManager::TryGetOrCreateFromClosestValidEntry(nos::Name pluginName, std::string entryName, RegisteredEntry& entry) {
	auto const& requestedPluginVer = PluginVersions[pluginName].first;
	nos::util::SemVer entryVer;
	EntryTypeNameBufferPair entryVal;
	auto searchDirectory = [&](ReadEntryList& entryList) {
		std::shared_lock<std::shared_mutex> lock(entryList.FileMutex); // Lock the mutex to ensure thread safety
		if (auto const& entriesFromDifVersions = entryList.Entries.find(entryName); entriesFromDifVersions != entryList.Entries.end()) {
			nosResult result = NOS_RESULT_NOT_FOUND;
			for (auto const& [pluginVer, entryValPair] : entriesFromDifVersions->second | std::views::reverse) {
				if (pluginVer > requestedPluginVer)
					continue;

				if (pluginVer <= requestedPluginVer) {
					result = entry.UpdateCallback(entryName.c_str(), entryValPair.second);
					entryVer = pluginVer;
					entryVal = entryValPair;
					entry.ReadEntryPluginVer = entryVer;
					break;
				}
			}
			if (result == NOS_RESULT_SUCCESS) {
				return UpdateEntry(pluginName, entryVer, entry.SaveDir, entryName, entryVal);
			}
		}
		return NOS_RESULT_FAILED;
	};

	if (auto localFile = LocalEntries.find(pluginName); localFile != LocalEntries.end()) {
		if(searchDirectory(localFile->second) == NOS_RESULT_SUCCESS)
			return NOS_RESULT_SUCCESS;
	}
	if (auto workspaceFile = WorkspaceEntries.find(pluginName); workspaceFile != WorkspaceEntries.end()) {
		if (searchDirectory(workspaceFile->second) == NOS_RESULT_SUCCESS)
			return NOS_RESULT_SUCCESS;
	}
	if (auto globalFile = GlobalEntries.find(pluginName); globalFile != GlobalEntries.end()) {
		if (searchDirectory(globalFile->second) == NOS_RESULT_SUCCESS)
			return NOS_RESULT_SUCCESS;
	}
	return NOS_RESULT_FAILED;
}


nosResult EntryManager::UpdateEntry(nos::Name pluginName, nos::util::SemVer pluginVersion, editor::SettingsEntryFileDirectory dir, std::string const& entryName, EntryTypeNameBufferPair entryVal) {
	auto addOrUpdateEntry = [&](ReadEntryList& entryList, editor::SettingsEntryFileDirectory dir) {
		auto updateEntry = [&](nos::util::SemVer entrySemVer) -> nosResult {
			auto res = WriteSettingsFile(pluginName, entrySemVer, dir, entryList);
			if(res != NOS_RESULT_SUCCESS) {
				nosEngine.LogE("Failed to write settings file for plugin %s with version %s", pluginName.AsString().c_str(), std::string(pluginVersion).c_str());
				return res;
			}
			// Update editor if the entry is registered
			if (auto pluginEntries = GSettingsEntryManager->RegisteredEntries.find(pluginName); pluginEntries != GSettingsEntryManager->RegisteredEntries.end())
				if (auto entry = pluginEntries->second.find(entryName); entry != pluginEntries->second.end())
					entry->second.LastValue = entryVal.second;
					return UpdateEditorEntriesForPlugin(pluginName);
			};

		auto& entriesFromDifVersions = entryList.Entries[entryName];

		for (size_t i = 0; i < entriesFromDifVersions.size(); i++) {
			auto const& entryVer = entriesFromDifVersions[i].first;
			if(entryVer == pluginVersion) {
				entriesFromDifVersions[i].second = entryVal;
				return updateEntry(entriesFromDifVersions[i].first);
			}
			if(entryVer > pluginVersion) {
				// If the version is greater, we can insert before it
				entriesFromDifVersions.insert(entriesFromDifVersions.begin() + i, {pluginVersion, entryVal});
				return updateEntry(entriesFromDifVersions[i].first);
			}
		}

		entriesFromDifVersions.push_back({ pluginVersion, entryVal }); // If we reach here, we can just append
		return updateEntry(entriesFromDifVersions.back().first);
	};

	std::unordered_map<nos::Name, ReadEntryList>& entriesMap = (dir == editor::SettingsEntryFileDirectory::LOCAL) ? LocalEntries :
		(dir == editor::SettingsEntryFileDirectory::GLOBAL) ? GlobalEntries : WorkspaceEntries;
	if (entriesMap.find(pluginName) == entriesMap.end()) {
		nosEngine.LogE("Plugin %s is not registered for editor settings, but called UpdateEntry().", pluginName.AsCStr());
	}
	return addOrUpdateEntry(entriesMap[pluginName], dir);
}
}
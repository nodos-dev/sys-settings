// Copyright MediaZ Teknoloji A.S. All Rights Reserved.

#include "nosUtil/VersionUtils.h"

namespace nos::util
{

bool SemVer::IsCompatible(int againstMajor) const
{
	if (Major == -1)
		return true;
	return Major == againstMajor;
}

bool SemVer::IsCompatible(SemVer const& user) const
{
	if (Major == -1)
		return true;
	if (Major != user.Major)
		return false;
	if (Minor < user.Minor)
		return false;
	return true;
}

bool SemVer::Supports(SemVer const& user) const
{
	if (Major == -1 || user.Major == -1)
		return true;
	if (Major != user.Major)
		return false;
	if (Minor < user.Minor)
		return false;
	else if (Minor == user.Minor && Patch < user.Patch)
		return false;
	return true;
}

SemVer::operator std::string() const
{
	if (Major == -1)
		return "N/A";
	std::stringstream ss;
	ss << Major;
	if (Minor != -1)
		ss << "." << Minor;
	if (Patch != -1)
		ss << "." << Patch;
	if (Build != -1)
		ss << ".b" << Build;
	return ss.str();
}

SemVer SemVer::ParseFrom(const std::string& string)
{
	SemVer ver;
	util::ParseVersion(string, ver.Major, ver.Minor, ver.Patch, ver.Build);
	return ver;
}

bool IsVersionNewer(const std::string& version, const std::string& other)
{
	// Returns true if version1 is greater than version2. False otherwise.

	std::istringstream iss1(version);
	std::istringstream iss2(other);
	std::string token1, token2;
	std::vector<std::string> parts1, parts2;

	while (std::getline(iss1, token1, '.'))
		parts1.push_back(token1);

	while (std::getline(iss2, token2, '.'))
		parts2.push_back(token2);

	// Compare corresponding parts
	for (size_t i = 0; i < std::max(parts1.size(), parts2.size()); ++i)
	{
		std::string part1 = (i < parts1.size()) ? parts1[i] : "";
		std::string part2 = (i < parts2.size()) ? parts2[i] : "";

		// If the part contain 'b' prefix, then it is a build number and should be omitted, e.g. 1.2.3.b45 -> 1.2.3.45
		if (part1.starts_with('b') && util::AllDigits(part1.substr(1)))
			part1 = part1.substr(1);
		if (part2.starts_with('b') && util::AllDigits(part2.substr(1)))
			part2 = part2.substr(1);

		if (util::AllDigits(part1) && util::AllDigits(part2))
		{
			int num1 = std::stoi(part1);
			int num2 = std::stoi(part2);

			if (num1 < num2)
				return false; // version1 is considered smaller
			if (num1 > num2)
				return true; // version2 is considered smaller
		}
		else if (util::AllDigits(part1))
		{
			return false; // version1 is considered smaller
		}
		else if (util::AllDigits(part2))
		{
			return true; // version2 is considered smaller
		}
		else if (!part1.empty() && !part2.empty())
		{
			if (part1 < part2)
				return false; // version1 is considered smaller
			if (part1 > part2)
				return true; // version2 is considered smaller
		}
	}

	return false; // Both version strings are equal
}

void ParseVersion(const std::string& version, int& major, int& minor, int& patch, int& build)
{
	std::istringstream iss(version);
	std::string token;
	std::vector<std::string> parts;

	while (std::getline(iss, token, '.'))
		parts.push_back(token);

	major = -1;
	minor = -1;
	patch = -1;
	build = -1;
	
	if (parts.size() >= 1)
		major = std::stoi(parts[0]);
	if (parts.size() >= 2)
		minor = std::stoi(parts[1]);
	if (parts.size() >= 3) {
		if (parts[2].starts_with('b') && util::AllDigits(parts[2].substr(1)))
		{
			build = std::stoi(parts[2].substr(1));
			patch = 0;
		}
		else if (util::AllDigits(parts[2]))
			patch = std::stoi(parts[2]);
	}
	if (parts.size() >= 4)
	{
		if (parts[3].starts_with('b') && util::AllDigits(parts[3].substr(1)))
			build = std::stoi(parts[3].substr(1));
		else if (util::AllDigits(parts[3]))
			build = std::stoi(parts[3]);
	}

}
}

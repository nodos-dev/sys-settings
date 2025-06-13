/*
 * Copyright MediaZ Teknoloji A.S. All Rights Reserved.
 */

#pragma once

// nosUtil
#include <nosCppUtilities.hpp>

// std
#include <string>

namespace nos::util
{
struct SemVer
{
	int Major = -1, Minor = -1, Patch = -1, Build = -1;
	static SemVer From(int major, int minor = 0, int patch = 0) { return {.Major = major, .Minor = minor, .Patch = patch}; }
	bool IsCompatible(int againstMajor) const;
	operator std::string() const;
	static SemVer ParseFrom(const std::string& string);
	bool IsCompatible(SemVer const& user) const;
	bool Supports(SemVer const& user) const;
	bool IsValid() const { return Major != -1 && Minor != -1; }
	bool operator==(const SemVer& other) const
	{
		return Major == other.Major && Minor == other.Minor && Patch == other.Patch && Build == other.Build;
	}
	bool operator!=(const SemVer& other) const { return !(*this == other); }
	bool operator<(const SemVer& other) const
	{
		if (Major != other.Major)
			return Major < other.Major;
		if (Minor != other.Minor)
			return Minor < other.Minor;
		if (Patch != other.Patch)
			return Patch < other.Patch;
		return Build < other.Build;
	}
	bool operator<=(const SemVer& other) const { return (*this < other) || (*this == other); }
	bool operator>(const SemVer& other) const { return !(*this <= other); }
	bool operator>=(const SemVer& other) const { return !(*this < other); }
};

bool IsVersionNewer(const std::string& version, const std::string& other);
void ParseVersion(const std::string& version, int& major, int& minor, int& patch, int& build);
}

template<>
struct std::hash<nos::util::SemVer>
{
	std::size_t operator()(const nos::util::SemVer& s) const noexcept {
		size_t h = 0;
		nos::hash_combine(h, s.Major, s.Minor, s.Patch, s.Build);
		return h;
	}
};

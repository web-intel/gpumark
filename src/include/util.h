#ifdef NDEBUG
#define ASSERT(x) do { (void)sizeof(x);} while (0)
#else
#include <assert.h>
#define ASSERT(x) assert(x)
#endif

#ifndef UNICODE
  typedef std::string String;
#else
  typedef std::wstring String;
#endif

inline std::wstring GetFullPath(std::wstring relativePath)
{
	WCHAR dir[FILENAME_MAX];
	DWORD size = GetModuleFileName(nullptr, dir, FILENAME_MAX);
	if (size == 0)
	{
		throw;
	}

	WCHAR* lastSlash = wcsrchr(dir, L'\\');
	if (lastSlash)
	{
		*(lastSlash + 1) = L'\0';
	}
	std::wstring wdir = dir;
	wdir.append(L"../../src/");
	return wdir.append(relativePath);
}

/*
std::wstring s2ws(const std::string& s)
{
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
    std::wstring r(len, L'\0');
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, &r[0], len);
    return r;
}

std::string ws2s(const std::wstring& s)
{
    int len;
    int slength = (int)s.length() + 1;
    len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0);
    std::string r(len, '\0');
    WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, &r[0], len, 0, 0);
    return r;
}
*/
/*

inline std::wstring GetFullPath(LPCWSTR relativePath)
{
	WCHAR dir[FILENAME_MAX];
	DWORD size = GetModuleFileName(nullptr, dir, FILENAME_MAX);
	//GetCurrentDirectory(FILENAME_MAX, dir);
	WCHAR* lastSlash = wcsrchr(dir, L'\\');
	if (lastSlash)
	{
		*(lastSlash + 1) = L'\0';
	}

	return dir.append(relativePath);
}
*/
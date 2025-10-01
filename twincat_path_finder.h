// twincat_path_finder.h
// External path finding functions for TwinCAT projects

#ifndef TWINCAT_PATH_FINDER_H
#define TWINCAT_PATH_FINDER_H

#include <windows.h>

// Find path to TwinCAT project file
BOOL FindTwinCATProjectPath(const char* filename, char* result_path, DWORD max_path_len);

// Try multiple methods to find the project file
BOOL FindProjectByCommonPaths(const char* filename, char* result_path);
BOOL FindProjectByRegistry(const char* filename, char* result_path);
BOOL FindProjectByMemorySearch(HANDLE hProcess, const char* filename, char* result_path);

#endif // TWINCAT_PATH_FINDER_H
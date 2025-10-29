#ifndef TWINCAT_CACHE_H
#define TWINCAT_CACHE_H

#include <windows.h>
#include <stdbool.h>

#define MAX_CACHE_ITEMS 10000

typedef struct {
    int index;
    char text[256];
    int level;
    char path[1024];
    int hasChildren;
    DWORD flags;
} CachedItem;

// Pomocne funkce
void GetProjectName(HWND listbox, HANDLE hProcess, char* projectName, int maxLen);
void ClickListBoxItem(HWND listbox, int itemIndex);

// Expandovani a nacteni stromu
int ExpandAllFoldersWrapper(HWND listbox, HANDLE hProcess);
int LoadFullTree(HWND listbox, HANDLE hProcess, CachedItem* cache, int maxItems);
void CollapseAllFoldersFromCache(HWND listbox, HANDLE hProcess, CachedItem* cache, int count);

// JSON operace
bool SaveCacheToFile(const char* filename, CachedItem* cache, int count, const char* projectName);
int LoadCacheFromFile(const char* filename, CachedItem* cache, int maxItems);

// Hledani v cache
int FindInCache(CachedItem* cache, int count, const char* searchText);

#endif // TWINCAT_CACHE_H

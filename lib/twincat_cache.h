/**
 * twincat_cache.h - Rozhraní pro cache systém TwinCAT projektu
 * 
 * Cache systém:
 * - První spuštění: ExpandAllFoldersWrapper() expanduje celý strom
 * - LoadFullTree() načte všechny položky do CachedItem[] pole
 * - SaveCacheToFile() uloží do JSON souboru (twincat_tree_cache.json)
 * - CollapseAllFoldersFromCache() zavře všechny složky (od nejvyššího levelu)
 * 
 * Další spuštění:
 * - LoadCacheFromFile() načte cache z JSON (rychlé, bez expandování)
 * - FindInCache() vyhledá položku podle textu
 * 
 * Struktura CachedItem:
 * - index: Index v ListBoxu v okamžiku uložení cache
 * - text: Název položky (např. "MAIN", "ST_Funkce")
 * - level: Úroveň odsazení (0=root, 1=POUs, 2=MAIN)
 * - path: Plná cesta k položce (např. "POUs/MAIN/ST_Funkce")
 * - hasChildren: 1 = složka, 0 = list
 * - flags: Typ položky (FLAG_FOLDER, FLAG_FILE, atd.)
 */

#ifndef TWINCAT_CACHE_H
#define TWINCAT_CACHE_H

#include <windows.h>
#include <stdbool.h>

#define MAX_CACHE_ITEMS 10000

typedef struct {
    int index;           // Index v ListBoxu (v okamžiku uložení cache)
    char text[256];      // Název položky (např. "MAIN", "POUs", "ST_Funkce")
    int level;           // Úroveň odsazení (0=root, 1=POUs, 2=MAIN, atd.)
    char path[1024];     // Plná cesta (např. "POUs/MAIN/ST_Funkce")
    int hasChildren;     // 1 = složka (expandovatelná), 0 = list (POU, GVL)
    DWORD flags;         // Typ položky (FLAG_FOLDER, FLAG_FILE, atd.)
} CachedItem;

// Pomocne funkce
void GetProjectName(HWND listbox, HANDLE hProcess, char* projectName, int maxLen); // Získá název projektu z první položky
void ClickListBoxItem(HWND listbox, int itemIndex); // Fyzické kliknutí myší na položku (SetCursorPos + mouse_event)

// Expandovani a nacteni stromu
int ExpandAllFoldersWrapper(HWND listbox, HANDLE hProcess); // Expanduje všechny složky (iterativně dokud nejsou všechny otevřené)
int LoadFullTree(HWND listbox, HANDLE hProcess, CachedItem* cache, int maxItems); // Načte všechny položky do cache pole
void CollapseAllFoldersFromCache(HWND listbox, HANDLE hProcess, CachedItem* cache, int count); // Zavře všechny složky (od nejvyššího levelu)

// JSON operace
bool SaveCacheToFile(const char* filename, CachedItem* cache, int count, const char* projectName); // Uloží cache do JSON souboru
int LoadCacheFromFile(const char* filename, CachedItem* cache, int maxItems); // Načte cache z JSON (ruční parser, ne cJSON knihovna)

// Hledani v cache
int FindInCache(CachedItem* cache, int count, const char* searchText); // Najde položku podle textu (case-insensitive)

#endif // TWINCAT_CACHE_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../lib/twincat_navigator.h"
#include "../lib/twincat_cache.h"

/**
 * Test: Cache celého stromu TwinCAT projektu
 * 
 * 1. Zkontroluje, jestli existuje cache soubor
 * 2. Pokud ne:
 *    - Expandne všechny složky
 *    - Načte celý strom
 *    - Uloží do JSON souboru
 *    - Zavře všechny složky zpět
 * 3. Načte cache a najde položku podle cesty
 * 4. Otevře pouze složky v cestě k položce
 */

#define CACHE_FILE "twincat_tree_cache.json"

// Globalni cache
CachedItem g_cache[MAX_CACHE_ITEMS];
int g_cacheSize = 0;

int main() {
    printf("===================================================\n");
    printf("  TEST: TREE CACHE (Expand All -> Save -> Search)\n");
    printf("===================================================\n");
    
    HWND twincatWindow = FindTwinCatWindow();
    if (!twincatWindow) {
        printf("[X] TwinCAT okno nenalezeno!\n");
        return 1;
    }
    
    HWND listbox = FindProjectListBox(twincatWindow);
    if (!listbox) {
        printf("[X] ListBox nenalezen!\n");
        return 1;
    }
    
    HANDLE hProcess = OpenTwinCatProcess(listbox);
    if (!hProcess) {
        printf("[X] Nelze otevrit proces!\n");
        return 1;
    }
    
    char projectName[256];
    GetProjectName(listbox, hProcess, projectName, sizeof(projectName));
    printf("Projekt: %s\n", projectName);
    
    // KROK 1: Zkus nacist cache
    g_cacheSize = LoadCacheFromFile(CACHE_FILE, g_cache, MAX_CACHE_ITEMS);
    
    if (g_cacheSize == 0) {
        // Cache neexistuje -> vytvorime ho
        printf("\n=== VYTVARENI CACHE ===\n");
        
        // Expandni vsechny slozky
        int expandedCount = ExpandAllFoldersWrapper(listbox, hProcess);
        
        // Nacti cely strom
        g_cacheSize = LoadFullTree(listbox, hProcess, g_cache, MAX_CACHE_ITEMS);
        
        // Uloz cache
        SaveCacheToFile(CACHE_FILE, g_cache, g_cacheSize, projectName);
        
        // Zavri vsechny slozky (optimalizovane!)
        printf("\n=== KROK 3: ZAVIRANI SLOZEK (cache-based) ===\n");
        CollapseAllFoldersFromCache(listbox, hProcess, g_cache, g_cacheSize);
        
        printf("\n[OK] Cache vytvoren, ulozen a slozky zavreny!\n");
    }
    
    // Test odznaceni - LB_SETTOPINDEX
    printf("\n=== TEST: Pokus o odznaceni ===\n");
    printf("Nastavuji LB_SETTOPINDEX na 0...\n");
    SendMessage(listbox, LB_SETTOPINDEX, 2, 0);
    Sleep(50);
    printf("Nastavuji LB_SETCURSEL na -1...\n");
    SendMessage(listbox, LB_SETCURSEL, (WPARAM)-1, 0);
    Sleep(50);
    printf("[OK] Prikazy odeslany - zkontroluj vizualne!\n");
    
    CloseHandle(hProcess);
    
    printf("\n===================================================\n");
    printf("HOTOVO!\n");
    printf("Cache soubor: %s\n", CACHE_FILE);
    printf("Celkem polozek v cache: %d\n", g_cacheSize);
    printf("===================================================\n");
    
    return 0;
}

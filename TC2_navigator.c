/**
 * TC2 Navigator - TwinCAT 2 Navigator with Global Hotkey
 * 
 * Finální aplikace kombinující:
 * - Globální keyboard hook (test_hook_simple.c)
 * - TwinCAT Navigator library (lib/twincat_*.c)
 * 
 * Funkce:
 * - Běží na pozadí s minimální konzolí
 * - Zachytává globální hotkey (Ctrl+Alt+Space)
 * - Automaticky naviguje v TwinCAT Project Explorer
 * - Cache systém pro rychlé vyhledávání
 * 
 * Kompilace:
 *   gcc -o TC2_navigator.exe TC2_navigator.c lib/twincat_navigator.c lib/twincat_tree.c lib/twincat_cache.c lib/twincat_search.c -luser32 -lpsapi -lcomctl32
 * 
 * Autostart:
 *   Po přidání do Registry (HKCU\Software\Microsoft\Windows\CurrentVersion\Run)
 *   nebo StartUp složky se aplikace spustí automaticky po přihlášení uživatele
 */

#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include "lib/twincat_navigator.h"
#include "lib/twincat_tree.h"
#include "lib/twincat_cache.h"
#include "lib/twincat_search.h"
#include "lib/twincat_search.h"

// Globální proměnné
HHOOK g_Hook = NULL;
bool g_Running = true;

#define CACHE_FILE "twincat_tree_cache.json"

// Globalni cache
CachedItem g_cache[MAX_CACHE_ITEMS];
int g_cacheSize = 0;

/**
 * Keyboard hook callback
 * Zachytává Ctrl+Alt+Space a spouští TwinCAT navigaci
 */
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT *kb = (KBDLLHOOKSTRUCT *)lParam;
        
        printf("Key pressed: VK=0x%02X\n", kb->vkCode);
        
        // Test: Ctrl+Alt+Space
        if (kb->vkCode == VK_SPACE) {
            bool ctrl = (GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0;
            bool alt = (GetAsyncKeyState(VK_LMENU) & 0x8000) != 0;
            
            //printf("  SPACE! Ctrl=%d, Alt=%d\n", ctrl, alt);
            
            
                printf("  >>> HOTKEY DETECTED! <<<\n");
                MessageBeep(MB_OK);
                
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
                    ClickListBoxItem(listbox, 0);
                    Sleep(50);
                    
                    ClickListBoxItem(listbox, 0);
                    printf("\n===================================================\n");
                    printf("HOTOVO!\n");
                    printf("Cache soubor: %s\n", CACHE_FILE);
                    printf("Celkem polozek v cache: %d\n", g_cacheSize);
                    printf("===================================================\n");
                                
                    printf("[TODO] TwinCAT navigation will be implemented here\n");
                    
                    
                    CloseHandle(hProcess);
                return 1; // Blokuj další zpracování
            
        }
        
        // ESC pro ukončení
        if (kb->vkCode == VK_ESCAPE) {
            bool ctrl = (GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0;
            bool alt = (GetAsyncKeyState(VK_LMENU) & 0x8000) != 0;
            
            if (ctrl && alt) {
                printf("\n[EXIT] Ctrl+Alt+ESC - Shutting down...\n");
                g_Running = false;
                PostQuitMessage(0);
                return 1;
            }
        }
    }
    
    return CallNextHookEx(g_Hook, nCode, wParam, lParam);
}

/**
 * Hlavní vstupní bod
 */
int main() {
    printf("========================================\n");
    printf("  TC2 Navigator v1.0\n");
    printf("========================================\n");
    printf("Hotkeys:\n");
    printf("  Ctrl+Alt+Space  - Activate navigator\n");
    printf("  Ctrl+Alt+ESC    - Exit\n");
    printf("========================================\n\n");
    
    // Instalace keyboard hooku
    g_Hook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    
    if (!g_Hook) {
        printf("[ERROR] Failed to install keyboard hook! Error=%lu\n", GetLastError());
        return 1;
    }
    
    printf("[OK] Keyboard hook installed. Handle=%p\n\n", g_Hook);
    
    // Message loop - exactly as in test_hook_simple.c
    MSG msg;
    while (true) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        if (!g_Running) {
            printf("\nExiting...\n");
            break;
        }
        
        Sleep(10);
    }
    
    // Cleanup
    UnhookWindowsHookEx(g_Hook);
    printf("Hook uninstalled\n");
    
    return 0;
}

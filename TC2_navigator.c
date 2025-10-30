/**
 * TC2 Navigator - Globální navigátor pro TwinCAT 2 Project Explorer
 * 
 * Popis:
 * Aplikace běží na pozadí a zachytává globální klávesovou zkratku Ctrl+Shift+A.
 * Po stisku zkratky extrahuje název POU/funkce z titulku aktivního TwinCAT okna
 * a automaticky naviguje na tuto položku v Project Explorer stromu.
 * 
 * Hlavní funkce:
 * - Globální keyboard hook pro zachycení Ctrl+Shift+A (funguje v jakékoliv aplikaci)
 * - Automatická extrakce názvu POU z titulku okna (např. "MAIN (PRG) - TwinCAT PLC Control")
 * - Vyhledání položky v cache stromu projektu (JSON soubor)
 * - Automatické rozbalení cesty ke hledané položce
 * - Fyzické kliknutí a focus na nalezenou položku v ListBoxu
 * 
 * Cache systém:
 * - První spuštění: expanduje celý strom, uloží do JSON, zavře složky
 * - Další spuštění: načte cache z JSON (rychlé vyhledávání)
 * - Cache obsahuje: index, text, level, path, hasChildren, flags
 * 
 * Použité knihovny:
 * - lib/twincat_navigator.c - Hledání TwinCAT okna a ListBoxu, čtení paměti
 * - lib/twincat_tree.c      - Práce se stromovou strukturou, expandování cest
 * - lib/twincat_cache.c     - Generování a načítání cache, JSON operace
 * - lib/twincat_search.c    - Extrakce názvu z titulku okna
 * 
 * Kompilace:
 *   gcc -o TC2_navigator.exe TC2_navigator.c ^
 *       lib/twincat_navigator.c lib/twincat_tree.c ^
 *       lib/twincat_cache.c lib/twincat_search.c ^
 *       -luser32 -lpsapi -lcomctl32
 * 
 * Klávesové zkratky:
 *   Ctrl+Shift+A   - Aktivovat navigaci (extrahuje název a naviguje)
 *   Ctrl+Alt+ESC   - Ukončit aplikaci
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
 * Keyboard hook callback - zachytává globální klávesové zkratky
 * 
 * Zpracovává:
 * - Ctrl+Shift+A: Spustí navigaci v TwinCAT Project Explorer
 *   1. Najde TwinCAT okno a ListBox
 *   2. Načte/vytvoří cache stromu projektu
 *   3. Extrahuje název POU z titulku okna
 *   4. Vyhledá položku v cache
 *   5. Expanduje cestu a klikne na položku
 * 
 * - Ctrl+Alt+ESC: Ukončí aplikaci
 * 
 * @param nCode  Hook kód (HC_ACTION = zpracuj událost)
 * @param wParam Typ události (WM_KEYDOWN)
 * @param lParam Ukazatel na KBDLLHOOKSTRUCT s informacemi o klávese
 * @return 1 = blokuj další zpracování, CallNextHookEx = předej dál
 */
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT *kb = (KBDLLHOOKSTRUCT *)lParam;
        
        //printf("Key pressed: VK=0x%02X\n", kb->vkCode);
        
        // Test: Ctrl+Alt+Space
        if (kb->vkCode == 'A') {
            // Použij GetKeyState místo GetAsyncKeyState pro spolehlivější detekci
            SHORT ctrlState = GetKeyState(VK_CONTROL);
            SHORT shiftState = GetKeyState(VK_SHIFT);
            
            //printf("  SPACE! Ctrl=%d, Alt=%d\n", ctrl, alt);
            bool ctrl = (ctrlState & 0x8000) != 0;
            bool shift = (shiftState & 0x8000) != 0;

            if (ctrl && shift) {
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
                
                printf("\n=== TEST: Pokus o odznaceni ===\n");
                printf("Nastavuji LB_SETTOPINDEX na 0...\n");
                SendMessage(listbox, LB_SETTOPINDEX, 2, 0);
                Sleep(50);
                printf("Nastavuji LB_SETCURSEL na -1...\n");
                SendMessage(listbox, LB_SETCURSEL, (WPARAM)-1, 0);
                Sleep(50);
                printf("[OK] Prikazy odeslany - zkontroluj vizualne!\n");
                
                printf("\n===================================================\n");
                printf("HOTOVO!\n");
                printf("Cache soubor: %s\n", CACHE_FILE);
                printf("Celkem polozek v cache: %d\n", g_cacheSize);
                printf("===================================================\n");
                
                printf("[TODO] TwinCAT navigation will be implemented here\n");
                
                char windowTitle[512];
                GetWindowText(twincatWindow, windowTitle, sizeof(windowTitle));
                
                printf("Nalezeno TwinCAT okno:\n");
                printf("Titulek: '%s'\n\n", windowTitle);
                
                char target[256];
                if (ExtractTargetFromTitle(windowTitle, target, sizeof(target))) {
                    printf("✓ Extrahovaný název: '%s'\n", target);
                    // TODO: Implementace navigace podle extrahovaného názvu
                    printf("\nHledam polozku: %s\n", target);
                    
                    // Zavolej funkci hledani
                    int foundIndex = FindAndExpandPath(listbox, hProcess, target);
                    
                    if (foundIndex >= 0) {
                        printf("\n========================================\n");
                        printf("VYSLEDEK: Polozka nalezena na indexu %d\n", foundIndex);
                        printf("========================================\n");
                        
                        // FOCUSUJ a OZNAC nalezenou polozku v ListBoxu
                        printf("\n[AKCE] Focusuji polozku v TwinCAT okne...\n");
                        
                        // 1. Aktivuj TwinCAT okno
                        SetForegroundWindow(twincatWindow);
                        Sleep(100);
                        
                        // 2. Zamer na ListBox
                        SetFocus(listbox);
                        Sleep(100);
                        
                        // 3. Vyber polozku
                        LRESULT result = SendMessage(listbox, LB_SETCURSEL, foundIndex, 0);
                        Sleep(100);
                        
                        if (result == LB_ERR) {
                            printf("[X] Nelze vybrat polozku (LB_SETCURSEL vratilo LB_ERR)\n");
                        } else {
                            printf("[OK] Polozka [%d] vybrana\n", foundIndex);
                        }
                        
                        // 4. KLIKNI na polozku (double-click pro otevreni)
                        printf("[AKCE] Klikam na polozku...\n");
                        PostMessage(listbox, WM_LBUTTONDBLCLK, 0, MAKELPARAM(10, foundIndex * 16));
                        Sleep(20);
                        printf("[OK] Polozka otevrena!\n");
                        
                        // Zobraz okolni polozky pro kontext
                        printf("\nKONTEXT (okolni polozky):\n");
                        TreeItem items[10];
                        int startIdx = (foundIndex > 2) ? foundIndex - 2 : 0;
                        
                        for (int i = 0; i < 5 && (startIdx + i) < GetListBoxItemCount(listbox); i++) {
                            TreeItem item;
                            if (ExtractTreeItem(hProcess, listbox, startIdx + i, &item)) {
                                if (startIdx + i == foundIndex) {
                                    printf(">>> [%2d] L%d %s  <<<< TADY!\n", 
                                           startIdx + i, item.position, item.text);
                                } else {
                                    printf("    [%2d] L%d %s\n", 
                                           startIdx + i, item.position, item.text);
                                }
                            }
                        }
                    } else {
                        printf("\n========================================\n");
                        printf("VYSLEDEK: Polozka '%s' nebyla nalezena\n", target);
                        printf("========================================\n");
                    }
                    
                } else {
                    printf("✗ Nepodařilo se extrahovat název z titulku\n");
                }
                
                CloseHandle(hProcess);
                return 1; // Blokuj další zpracování
            }
        }
        
        // ESC pro ukončení
        if (kb->vkCode == 'Q') {

            // Použij GetKeyState místo GetAsyncKeyState pro spolehlivější detekci
            SHORT ctrlState = GetKeyState(VK_CONTROL);
            SHORT shiftState = GetKeyState(VK_SHIFT);
        
            bool ctrl = (ctrlState & 0x8000) != 0;
            bool shift = (shiftState & 0x8000) != 0;

            if (ctrl && shift) {

                printf("\n[EXIT] Ctrl+Shift+Q - Shutting down...\n");
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
    printf("  Ctrl+Shift+A  - Activate navigator\n");
    printf("  Ctrl+Shift+Q    - Exit\n");
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

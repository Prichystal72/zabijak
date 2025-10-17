#include <windows.h>
#include <stdio.h>
#include "../lib/twincat_navigator.h"
#include "../lib/twincat_tree.h"

/**
 * Test: Hledani konkretni polozky s automatickym expandovanim cesty
 */

int main() {
    printf("===================================================\n");
    printf("  TEST HLEDANI POLOZKY S AUTO-EXPAND\n");
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

    // Hledana polozka
    const char* searchText = "ST03_Init_AbholPos";
    
    printf("\nHledam polozku: %s\n", searchText);
    
    // Zavolej funkci hledani
    int foundIndex = FindAndExpandPath(listbox, hProcess, searchText);
    
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
        printf("VYSLEDEK: Polozka '%s' nebyla nalezena\n", searchText);
        printf("========================================\n");
    }

    CloseHandle(hProcess);
    return 0;
}

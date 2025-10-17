#include <windows.h>
#include <stdio.h>
#include "../lib/twincat_navigator.h"

/**
 * Test nekonecne smycky: Neustale prepina polozku na indexu 6
 * Ukonci pomoci Ctrl+C
 */

int main() {
    printf("=== TEST: Nekonecna smycka prepinan polozky 6 ===\n\n");
    
    HWND twincatWindow = FindTwinCatWindow();
    if (!twincatWindow) {
        printf("[X] TwinCAT okno nenalezeno!\n");
        return 1;
    }
    
    HWND listbox = FindProjectListBox(twincatWindow);
    if (!listbox) {
        printf("[X] Project ListBox nenalezen!\n");
        return 1;
    }
    
    DWORD processId;
    GetWindowThreadProcessId(listbox, &processId);
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) {
        printf("[X] Nelze otevrit proces!\n");
        return 1;
    }
    
    int count = SendMessage(listbox, LB_GETCOUNT, 0, 0);
    printf("ListBox obsahuje %d polozek\n", count);
    
    if (count < 7) {
        printf("[!] ListBox ma mene nez 7 polozek. Index 6 neexistuje.\n");
        CloseHandle(hProcess);
        return 1;
    }
    
    // Ziskej text polozky 6
    char text[256] = {0};
    SendMessage(listbox, LB_GETTEXT, 6, (LPARAM)text);
    printf("Polozka 6: '%s'\n", text);
    
    // Zjisti pocatecni stav
    bool isExpanded = IsItemExpanded(listbox, hProcess, 6);
    printf("Pocatecni stav: %s\n\n", isExpanded ? "OTEVRENA" : "ZAVRENA");
    
    int delta;
    
    printf("\n=== NEKONECNA SMYCKA KLIKANI (Ctrl+C pro ukonceni) ===\n\n");
    
    int cycleCount = 0;
    while (1) {
        cycleCount++;
        printf("[Cyklus %d] Prepinam polozku 6...\n", cycleCount);
        
        delta = ToggleListBoxItem(listbox, 6);
        if (delta > 0) {
            printf("  -> Otevreno! Pribylo %d polozek.\n", delta);
        } else if (delta < 0) {
            printf("  -> Zavreno! Ubylo %d polozek.\n", -delta);
        } else {
            printf("  -> Zadna zmena.\n");
        }
        
        isExpanded = IsItemExpanded(listbox, hProcess, 6);
        printf("  -> Stav: %s\n\n", isExpanded ? "OTEVRENA" : "ZAVRENA");
        
        printf("Pauza 2 sekundy...\n\n");
        Sleep(2000);
    }
    
    CloseHandle(hProcess);
    return 0;
}

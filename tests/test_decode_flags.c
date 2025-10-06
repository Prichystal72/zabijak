#include <windows.h>
#include <stdio.h>
#include "../lib/twincat_navigator.h"

/**
 * Test: Dekodovani flags - zjistime co znamenaji
 */

int main() {
    printf("===================================================\n");
    printf("  DEKODOVANI FLAGS\n");
    printf("===================================================\n\n");
    
    HWND twincatWindow = FindTwinCatWindow();
    if (!twincatWindow) return 1;
    
    HWND listbox = FindProjectListBox(twincatWindow);
    if (!listbox) return 1;
    
    HANDLE hProcess = OpenTwinCatProcess(listbox);
    if (!hProcess) return 1;
    
    printf("PUVODNI STAV:\n");
    printf("===================================================\n");
    
    int itemCount = GetListBoxItemCount(listbox);
    for (int i = 0; i < itemCount; i++) {
        TreeItem item;
        if (ExtractTreeItem(hProcess, listbox, i, &item)) {
            printf("[%d] %-20s | Flags: 0x%06X | Level: %d\n", 
                   i, item.text, item.flags, item.position);
        }
    }
    
    printf("\n===================================================\n");
    printf("OTEVREME POLOZKU 1 (EPT Lib):\n");
    printf("===================================================\n");
    
    int delta = ToggleListBoxItem(listbox, 1);
    printf("Zmena: %+d polozek\n\n", delta);
    
    Sleep(200);
    
    itemCount = GetListBoxItemCount(listbox);
    for (int i = 0; i < itemCount; i++) {
        TreeItem item;
        if (ExtractTreeItem(hProcess, listbox, i, &item)) {
            printf("[%d] %-20s | Flags: 0x%06X | Level: %d\n", 
                   i, item.text, item.flags, item.position);
        }
    }
    
    printf("\n===================================================\n");
    printf("ZAVREME POLOZKU 1 (EPT Lib):\n");
    printf("===================================================\n");
    
    delta = ToggleListBoxItem(listbox, 1);
    printf("Zmena: %+d polozek\n\n", delta);
    
    Sleep(200);
    
    itemCount = GetListBoxItemCount(listbox);
    for (int i = 0; i < itemCount; i++) {
        TreeItem item;
        if (ExtractTreeItem(hProcess, listbox, i, &item)) {
            printf("[%d] %-20s | Flags: 0x%06X | Level: %d\n", 
                   i, item.text, item.flags, item.position);
        }
    }
    
    printf("\n===================================================\n");
    printf("ANALYZA FLAGS:\n");
    printf("===================================================\n");
    printf("0x0205F5 (POUs)          : Binary = ");
    for (int i = 23; i >= 0; i--) {
        printf("%d", (0x0205F5 >> i) & 1);
        if (i % 4 == 0) printf(" ");
    }
    printf("\n");
    
    printf("0x0205F7 (zavrene slozky): Binary = ");
    for (int i = 23; i >= 0; i--) {
        printf("%d", (0x0205F7 >> i) & 1);
        if (i % 4 == 0) printf(" ");
    }
    printf("\n");
    
    printf("\nRozdil:\n");
    printf("0x0205F5 XOR 0x0205F7 = 0x%06X\n", 0x0205F5 ^ 0x0205F7);
    printf("Rozdil je v bitu: %d (bit cislo %d)\n", 0x0205F7 - 0x0205F5, 1);
    
    CloseHandle(hProcess);
    return 0;
}

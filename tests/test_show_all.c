#include <windows.h>
#include <stdio.h>
#include "../lib/twincat_navigator.h"

/**
 * Test: Zobrazeni VSECH polozek v ListBoxu s detaily
 */

int main() {
    printf("===================================================\n");
    printf("  ZOBRAZENI VSECH POLOZEK V LISTBOXU\n");
    printf("===================================================\n\n");
    
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
    
    int itemCount = GetListBoxItemCount(listbox);
    printf("Celkem polozek v ListBoxu: %d\n\n", itemCount);
    
    // Zobraz vsechny polozky
    printf("===================================================\n");
    printf("IDX | LEVEL | FLAGS    | TEXT\n");
    printf("===================================================\n");
    
    for (int i = 0; i < itemCount; i++) {
        TreeItem item;
        if (ExtractTreeItem(hProcess, listbox, i, &item)) {
            // Zjisti skutecny level (z position)
            int realLevel = item.position;
            
            printf("[%2d] | L%d    | 0x%06X | %s\n", 
                   i, realLevel, item.flags, item.text);
        } else {
            printf("[%2d] | ???   | ???????? | <nelze nacist>\n", i);
        }
    }
    
    printf("===================================================\n\n");
    
    // Statistika podle levelu
    printf("STATISTIKA PODLE LEVELU:\n");
    int levelCount[10] = {0};
    
    for (int i = 0; i < itemCount; i++) {
        TreeItem item;
        if (ExtractTreeItem(hProcess, listbox, i, &item)) {
            if (item.position < 10) {
                levelCount[item.position]++;
            }
        }
    }
    
    for (int i = 0; i < 10; i++) {
        if (levelCount[i] > 0) {
            printf("  Level %d: %d polozek\n", i, levelCount[i]);
        }
    }
    
    CloseHandle(hProcess);
    return 0;
}

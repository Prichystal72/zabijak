#include <windows.h>
#include <stdio.h>
#include "../lib/twincat_navigator.h"
#include "../lib/twincat_tree.h"

/**
 * Test: Zobrazeni VSECH polozek v ListBoxu s detaily
 * Pouziva knihovnu twincat_tree.c pro praci se seznamem polozek
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

    // Ziskej vsechny polozky pomoci nove knihovny
    TreeItem items[1000];
    int count = GetAllVisibleItems(listbox, hProcess, items, 1000);
    
    printf("Celkem polozek v ListBoxu: %d\n\n", count);

    // Zobraz vsechny polozky
    PrintTreeStructure(items, count);
    
    printf("\n");
    
    // Statistika podle levelu
    printf("STATISTIKA PODLE LEVELU:\n");
    int levelCounts[10];
    GetLevelStatistics(items, count, levelCounts);
    
    for (int i = 0; i < 10; i++) {
        if (levelCounts[i] > 0) {
            printf("  Level %d: %d polozek\n", i, levelCounts[i]);
        }
    }

    CloseHandle(hProcess);
    return 0;
}

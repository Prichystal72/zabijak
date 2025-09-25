#include "lib/twincat_navigator.h"
#include <stdlib.h>

int main() {
    printf("=== TWINCAT PROJECT NAVIGATOR ===\n\n");
    
    // Najdi TwinCAT okno
    HWND hTwinCAT = FindTwinCatWindow();
    if (!hTwinCAT) {
        printf("❌ TwinCAT okno nenalezeno!\n");
        printf("Ujisti se, že TwinCAT PLC Control je spuštěn.\n");
        return 1;
    }
    
    char windowTitle[512];
    GetWindowText(hTwinCAT, windowTitle, sizeof(windowTitle));
    printf("✅ TwinCAT nalezen: %s\n\n", windowTitle);
    
    // Najdi project ListBox
    HWND hListBox = FindProjectListBox(hTwinCAT);
    if (!hListBox) {
        printf("❌ Project ListBox nenalezen!\n");
        return 1;
    }
    
    // Otevři proces
    HANDLE hProcess = OpenTwinCatProcess(hListBox);
    if (!hProcess) {
        printf("❌ Nelze otevřít TwinCAT proces!\n");
        return 1;
    }
    
    // Získej počet položek
    int itemCount = GetListBoxItemCount(hListBox);
    printf("📊 ListBox obsahuje %d položek\n\n", itemCount);
    
    if (itemCount <= 0) {
        printf("❌ ListBox je prázdný nebo nedostupný!\n");
        CloseHandle(hProcess);
        return 1;
    }
    
    // Alokuj paměť pro položky
    TreeItem* items = malloc(itemCount * sizeof(TreeItem));
    if (!items) {
        printf("❌ Nedostatek paměti!\n");
        CloseHandle(hProcess);
        return 1;
    }
    
    // Extrahuj všechny položky
    int extractedCount = 0;
    printf("🔍 Extrahuji položky stromu...\n");
    
    for (int i = 0; i < itemCount; i++) {
        if (ExtractTreeItem(hProcess, hListBox, i, &items[extractedCount])) {
            extractedCount++;
        }
    }
    
    printf("✅ Extrahováno %d položek\n\n", extractedCount);
    
    // Zobraz strukturu
    PrintTreeStructure(items, extractedCount);
    
    // Zaměř se na 25. položku (index 24)
    if (extractedCount > 24) {
        printf("\n🎯 Zaměřuji se na 25. položku...\n");
        if (FocusOnItem(hListBox, 24)) {
            printf("✅ Focus nastaven na položku [24]: %s\n", items[24].text);
        } else {
            printf("❌ Nepodařilo se nastavit focus!\n");
        }
    }
    
    // Cleanup
    free(items);
    CloseHandle(hProcess);
    
    printf("\n=== HOTOVO ===\n");
    printf("Stiskněte Enter...\n");
    getchar();
    
    return 0;
}
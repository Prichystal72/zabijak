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
    
    // Zobraz aktuální stav
    printf("=== VÝCHOZÍ STAV STROMU ===\n");
    PrintTreeStructure(items, extractedCount);
    
    // INTELIGENTNÍ HLEDÁNÍ A OTEVÍRÁNÍ CESTY
    printf("\n=== INTELIGENTNÍ VYHLEDÁVÁNÍ ===\n");
    
    // Extrahuj cílový text z titulku okna
    char targetText[256];
    if (ExtractTargetFromTitle(windowTitle, targetText, sizeof(targetText))) {
        printf("🎯 Cílový text z titulku: '%s'\n", targetText);
        
        // Použij inteligentní vyhledávání
        int targetIndex = FindAndOpenPath(hListBox, hProcess, targetText);
        
        if (targetIndex >= 0) {
            printf("🎯 Zaměřuji se na nalezený cíl [%d]...\n", targetIndex);
            
            if (FocusOnItem(hListBox, targetIndex)) {
                printf("✅ Focus úspěšně nastaven!\n");
                
                // Ověř že focus je správně nastaven
                int currentSel = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
                printf("✓ Aktuální selection: %d\n", currentSel);
                
                // Zobraz finální stav stromu
                printf("\n=== FINÁLNÍ STAV (JEN POTŘEBNÁ CESTA OTEVŘENÁ) ===\n");
                free(items);
                
                itemCount = GetListBoxItemCount(hListBox);
                items = malloc(itemCount * sizeof(TreeItem));
                if (items) {
                    extractedCount = 0;
                    for (int i = 0; i < itemCount; i++) {
                        if (ExtractTreeItem(hProcess, hListBox, i, &items[extractedCount])) {
                            extractedCount++;
                        }
                    }
                    
                    // Najdi a zobraz zaměřenou položku
                    for (int i = 0; i < extractedCount; i++) {
                        if (items[i].index == currentSel) {
                            printf("✓ Zaměřeno na: %s\n", items[i].text);
                            break;
                        }
                    }
                    
                    PrintTreeStructure(items, extractedCount);
                }
            } else {
                printf("❌ Nepodařilo se nastavit focus!\n");
            }
        } else {
            printf("❌ Cíl '%s' nebyl nalezen!\n", targetText);
        }
    } else {
        printf("❌ Nelze extrahovat cílový text z titulku\n");
    }
    
    // Cleanup
    free(items);
    CloseHandle(hProcess);
    
    printf("\n=== HOTOVO ===\n");
    printf("Stiskněte Enter...\n");
    getchar();
    
    return 0;
}
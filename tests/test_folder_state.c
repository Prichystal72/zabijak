#include "../lib/twincat_navigator.h"

int main() {
    printf("=== TEST DETEKCE STAVU SLOZKY ===\n\n");
    
    // Najdi TwinCAT okno
    HWND twincatWindow = FindTwinCatWindow();
    if (!twincatWindow) {
        printf("[CHYBA] TwinCAT okno nenalezeno!\n");
        return 1;
    }
    printf("[OK] TwinCAT okno: 0x%p\n", twincatWindow);
    
    // Najdi ListBox
    HWND listbox = FindProjectListBox(twincatWindow);
    if (!listbox) {
        printf("[CHYBA] ListBox nenalezen!\n");
        return 1;
    }
    printf("[OK] ListBox: 0x%p\n", listbox);
    
    // Otevri proces
    HANDLE hProcess = OpenTwinCatProcess(listbox);
    if (!hProcess) {
        printf("[CHYBA] Nelze otevrit proces!\n");
        return 1;
    }
    printf("[OK] Proces otevren\n\n");
    
    // Ziskej vsechny polozky
    int itemCount = GetListBoxItemCount(listbox);
    printf("Celkem polozek: %d\n\n", itemCount);
    
    printf("POROVNANI METOD DETEKCE STAVU:\n");
    printf("Index | Text                    | IsItemExpanded | GetFolderState\n");
    printf("------+-------------------------+----------------+---------------\n");
    
    // Porovnej obě metody pro každou položku
    for (int i = 0; i < itemCount; i++) {
        TreeItem item;
        if (ExtractTreeItem(hProcess, listbox, i, &item)) {
            bool isExpanded = IsItemExpanded(listbox, hProcess, i);
            int folderState = GetFolderState(listbox, hProcess, i);
            
            printf("[%02d]  | %-23s | %-14s | %d (%s)\n", 
                   i, 
                   item.text,
                   isExpanded ? "TRUE (open)" : "FALSE (closed)",
                   folderState,
                   folderState == 1 ? "open" : (folderState == 0 ? "closed" : "error"));
        }
    }
    
    printf("\n=== TEST OTEVIRANI/ZAVIRANI ===\n\n");
    
    // Test na EPT Lib (index 1)
    int testIndex = 1;
    TreeItem testItem;
    if (ExtractTreeItem(hProcess, listbox, testIndex, &testItem)) {
        printf("Testuji slozku: %s (index %d)\n\n", testItem.text, testIndex);
        
        // Stav před
        bool expandedBefore = IsItemExpanded(listbox, hProcess, testIndex);
        int stateBefore = GetFolderState(listbox, hProcess, testIndex);
        printf("PRED akcí:\n");
        printf("  IsItemExpanded: %s\n", expandedBefore ? "TRUE" : "FALSE");
        printf("  GetFolderState: %d (%s)\n\n", stateBefore, stateBefore == 1 ? "open" : "closed");
        
        // Toggle
        printf("Prepinani slozky...\n");
        int delta = ToggleListBoxItem(listbox, testIndex);
        printf("  Zmena poctu polozek: %d\n\n", delta);
        
        // Stav po
        bool expandedAfter = IsItemExpanded(listbox, hProcess, testIndex);
        int stateAfter = GetFolderState(listbox, hProcess, testIndex);
        printf("PO akci:\n");
        printf("  IsItemExpanded: %s\n", expandedAfter ? "TRUE" : "FALSE");
        printf("  GetFolderState: %d (%s)\n\n", stateAfter, stateAfter == 1 ? "open" : "closed");
        
        // Vyhodnocení
        printf("VYHODNOCENI:\n");
        if (expandedBefore != expandedAfter) {
            printf("  [OK] IsItemExpanded detekovalo zmenu\n");
        } else {
            printf("  [FAIL] IsItemExpanded NEdetekovala zmenu!\n");
        }
        
        if (stateBefore != stateAfter) {
            printf("  [OK] GetFolderState detekovalo zmenu\n");
        } else {
            printf("  [FAIL] GetFolderState NEdetekovala zmenu!\n");
        }
        
        // Kontrola konzistence
        if ((expandedAfter && stateAfter == 1) || (!expandedAfter && stateAfter == 0)) {
            printf("  [OK] Obe metody jsou v souladu\n");
        } else {
            printf("  [WARNING] Metody se lisi!\n");
        }
    }
    
    CloseHandle(hProcess);
    
    printf("\n=== TEST DOKONCEN ===\n");
    return 0;
}

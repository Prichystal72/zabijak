#include "twincat_tree.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Ziska vsechny viditelne polozky z ListBoxu
int GetAllVisibleItems(HWND listbox, HANDLE hProcess, TreeItem* items, int maxItems) {
    if (!listbox || !hProcess || !items || maxItems <= 0) {
        return 0;
    }
    
    int itemCount = SendMessage(listbox, LB_GETCOUNT, 0, 0);
    if (itemCount == LB_ERR || itemCount == 0) {
        return 0;
    }
    
    int extractedCount = 0;
    for (int i = 0; i < itemCount && extractedCount < maxItems; i++) {
        if (ExtractTreeItem(hProcess, listbox, i, &items[extractedCount])) {
            extractedCount++;
        }
    }
    
    return extractedCount;
}

// Najde polozku podle textu (case-insensitive)
int FindItemByText(TreeItem* items, int count, const char* text) {
    if (!items || !text || count <= 0) {
        return -1;
    }
    
    for (int i = 0; i < count; i++) {
        // Case-insensitive porovnani
        if (_stricmp(items[i].text, text) == 0) {
            return i;
        }
    }
    
    return -1;
}

// Najde vsechny polozky na danem levelu
int FindItemsByLevel(TreeItem* items, int count, int level, int* foundIndices, int maxFound) {
    if (!items || !foundIndices || count <= 0 || maxFound <= 0) {
        return 0;
    }
    
    int foundCount = 0;
    for (int i = 0; i < count && foundCount < maxFound; i++) {
        if (items[i].position == level) {
            foundIndices[foundCount++] = i;
        }
    }
    
    return foundCount;
}

// Vytiskne stromovou strukturu
void PrintTreeStructure(TreeItem* items, int count) {
    if (!items || count <= 0) {
        printf("[X] Zadne polozky k zobrazeni\n");
        return;
    }
    
    printf("===================================================\n");
    printf("IDX | LEVEL | FLAGS    | TEXT\n");
    printf("===================================================\n");
    
    for (int i = 0; i < count; i++) {
        printf("[%2d] | L%d    | 0x%06X | %s\n", 
               items[i].index, 
               items[i].position, 
               items[i].flags, 
               items[i].text);
    }
    
    printf("===================================================\n");
}

// Ziska statistiku podle levelu
void GetLevelStatistics(TreeItem* items, int count, int* levelCounts) {
    if (!items || !levelCounts || count <= 0) {
        return;
    }
    
    // Vynuluj pole
    for (int i = 0; i < 10; i++) {
        levelCounts[i] = 0;
    }
    
    // Spocitej vyskyty
    for (int i = 0; i < count; i++) {
        if (items[i].position >= 0 && items[i].position < 10) {
            levelCounts[items[i].position]++;
        }
    }
}

// Najde polozku v ListBoxu s automatickym expandovanim cesty
int FindAndExpandPath(HWND listbox, HANDLE hProcess, const char* searchText) {
    if (!listbox || !hProcess || !searchText) {
        printf("[X] Neplatne parametry pro FindAndExpandPath\n");
        return -1;
    }
    
    printf("\n[HLEDANI] Hledam polozku: '%s'\n", searchText);
    printf("========================================\n");
    
    // Zasobnik pro indexy otevrenych slozek (ktere jsme sami otevreli)
    int openedStack[100];
    int stackSize = 0;
    int previousLevel = -1;
    
    int maxIterations = 1000;  // Ochrana proti nekonecne smycce
    int iteration = 0;
    
    while (iteration < maxIterations) {
        int itemCount = GetListBoxItemCount(listbox);
        if (itemCount <= 0) {
            printf("[X] Prazdny ListBox!\n");
            return -1;
        }
        
        printf("\n[Iterace %d] Celkem polozek: %d, Stack size: %d\n", 
               iteration, itemCount, stackSize);
        
        bool foundNewFolder = false;
        
        for (int i = 0; i < itemCount; i++) {
            TreeItem item;
            if (!ExtractTreeItem(hProcess, listbox, i, &item)) {
                continue;
            }
            
            // Debug output - pouze pro CLOSED a OPEN, ne LEAF
            int folderState = GetFolderState(listbox, hProcess, i);
            
            // Detekce poklesu levelu -> zavri stack
            if (previousLevel >= 0 && item.position < previousLevel) {
                // Zavri slozky ze stacku, ktere patri k vyssimu levelu
                while (stackSize > 0) {
                    int topIndex = openedStack[stackSize - 1];
                    TreeItem topItem;
                    if (ExtractTreeItem(hProcess, listbox, topIndex, &topItem)) {
                        if (topItem.position >= item.position) {
                            ToggleListBoxItem(listbox, topIndex);
                            stackSize--;
                            Sleep(50);
                        } else {
                            break;
                        }
                    } else {
                        stackSize--;
                    }
                }
            }
            
            // Nasli jsme hledanou polozku?
            if (_stricmp(item.text, searchText) == 0) {
                printf("\n[OK] NALEZENO! Index: %d, Level: %d, Text: '%s'\n", 
                       i, item.position, item.text);
                return i;
            }
            
            // Zkus otevrit polozku, pokud neni uz otevrena
            bool shouldTryOpen = false;
            
            if (item.hasChildren && folderState == 0) {
                shouldTryOpen = true;
            } else if (!item.hasChildren && folderState == 0) {
                shouldTryOpen = true;
            }
            
            if (shouldTryOpen) {
                int countBefore = itemCount;
                ToggleListBoxItem(listbox, i);
                Sleep(100);
                
                // Zrus selection (aby nezustal modry след)
                SendMessage(listbox, LB_SETCURSEL, (WPARAM)-1, 0);
                
                int countAfter = GetListBoxItemCount(listbox);
                
                if (countAfter > countBefore) {
                    // Otevreli jsme slozku, pridej na stack
                    openedStack[stackSize++] = i;
                    foundNewFolder = true;
                    printf("  [%d] '%s' otevreno (+%d polozek)\n", i, item.text, countAfter - countBefore);
                    break;
                } else if (countAfter < countBefore) {
                    // Omylem jsme zavreli slozku, vratime zpet
                    ToggleListBoxItem(listbox, i);
                    Sleep(100);
                } 
                // Jinak to neni slozka, pokracujeme
            }
            
            previousLevel = item.position;
        }
        
        // Pokud jsme v teto iteraci neotrevreli zadnou novou slozku, 
        // prohledali jsme vse
        if (!foundNewFolder) {
            printf("\n[X] Polozka '%s' nenalezena\n", searchText);
            
            // Zavri vsechny otevrene slozky
            while (stackSize > 0) {
                int idx = openedStack[--stackSize];
                ToggleListBoxItem(listbox, idx);
                Sleep(50);
            }
            
            return -1;
        }
        
        iteration++;
    }
    
    printf("[X] Prekrocen limit iteraci!\n");
    return -1;
}

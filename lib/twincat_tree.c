/**
 * twincat_tree.c - Implementace funkcí pro navigaci ve stromu TwinCAT projektu
 * 
 * Klíčová funkce FindAndExpandPath():
 * - Načte cache z JSON (funkce LoadCacheFromFile z twincat_cache.c)
 * - Najde položku v cache podle textu (case-insensitive)
 * - Parsuje cestu na části oddělené "/" (např. "POUs/MAIN/ST_Funkce")
 * - Postupně expanduje složky:
 *   1. Najde složku podle textu v aktuálně viditelných položkách
 *   2. Vybere ji (LB_SETCURSEL)
 *   3. Double-click pro rozbalení (WM_LBUTTONDBLCLK)
 *   4. Čeká na změnu počtu položek (nové položky se objeví)
 *   5. Pokračuje na další část cesty
 * - Vrátí index nalezené položky nebo -1
 */

#include "twincat_tree.h"
#include "twincat_cache.h"
#include <stdio.h>
#include <stdlib.h>
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
    fflush(stdout);
    
    // 1. Nacti cache (pouzij malloc pro velke pole)
    CachedItem* cache = (CachedItem*)malloc(MAX_CACHE_ITEMS * sizeof(CachedItem));
    if (!cache) {
        printf("[X] Nelze alokovat pamet pro cache!\n");
        return -1;
    }
    
    printf("[DEBUG] Pokus o nacteni cache souboru...\n");
    fflush(stdout);
    int cacheSize = LoadCacheFromFile("twincat_tree_cache.json", cache, MAX_CACHE_ITEMS);
    printf("[DEBUG] LoadCacheFromFile vratil: %d\n", cacheSize);
    fflush(stdout);
    
    if (cacheSize == 0) {
        printf("[!] Cache neexistuje, pouzivam fallback (plne expandovani)\n");
        free(cache);
        // TODO: Fallback na stary zpusob
        return -1;
    }
    
    printf("[OK] Cache nactena (%d polozek)\n", cacheSize);
    
    // 2. Najdi polozku v cache
    int cacheIndex = FindInCache(cache, cacheSize, searchText);
    if (cacheIndex < 0) {
        printf("[X] Polozka '%s' nenalezena v cache!\n", searchText);
        free(cache);
        return -1;
    }
    
    printf("[OK] Polozka nalezena v cache:\n");
    printf("     Cesta: %s\n", cache[cacheIndex].path);
    printf("     Level: %d\n", cache[cacheIndex].level);
    
    // 3. Parsuj cestu (napr. "POUs/Stations/ST_00/ST00_PRGs/ST00_CallPRGs")
    char pathCopy[1024];
    strcpy(pathCopy, cache[cacheIndex].path);
    
    char* pathParts[20];
    int pathCount = 0;
    
    char* token = strtok(pathCopy, "/");
    while (token && pathCount < 20) {
        pathParts[pathCount++] = token;
        token = strtok(NULL, "/");
    }
    
    printf("[OK] Cesta rozdelena na %d casti\n", pathCount);
    for (int i = 0; i < pathCount; i++) {
        printf("     [%d] %s\n", i, pathParts[i]);
    }
    
    // 4. Postupne otviraj slozky v ceste (krome posledni = samotna polozka)
    printf("\n[EXPANDOVANI] Oteviram slozky v ceste...\n");
    
    for (int partIdx = 0; partIdx < pathCount - 1; partIdx++) {
        const char* folderName = pathParts[partIdx];
        int targetLevel = partIdx;  // Level odpovida pozici v ceste
        
        printf("\n[%d/%d] Hledam slozku '%s' na levelu %d...\n", 
               partIdx + 1, pathCount - 1, folderName, targetLevel);
        
        // Najdi slozku v aktualnim stavu ListBoxu
        int itemCount = GetListBoxItemCount(listbox);
        int foundIndex = -1;
        
        for (int i = 0; i < itemCount; i++) {
            TreeItem item;
            if (ExtractTreeItem(hProcess, listbox, i, &item)) {
                if (item.position == targetLevel && _stricmp(item.text, folderName) == 0) {
                    foundIndex = i;
                    break;
                }
            }
        }
        
        if (foundIndex < 0) {
            printf("[X] Slozka '%s' nenalezena na levelu %d!\n", folderName, targetLevel);
            return -1;
        }
        
        printf("[OK] Slozka '%s' nalezena na indexu %d\n", folderName, foundIndex);
        
        // Zkontroluj, jestli neni uz otevrena
        int folderState = GetFolderState(listbox, hProcess, foundIndex);
        
        if (folderState == 0) {  // CLOSED
            printf("     -> Oteviram slozku...\n");
            ToggleListBoxItem(listbox, foundIndex);
            Sleep(100);
            
            // Zrus selection
            SendMessage(listbox, LB_SETCURSEL, (WPARAM)-1, 0);
        } else {
            printf("     -> Slozka uz je otevrena\n");
        }
    }
    
    // Klikni na POUs dvakrat pro odznaceni vsech ostatnich polozek
    int pousIndex = -1;
    int itemCount = GetListBoxItemCount(listbox);
    for (int i = 0; i < itemCount; i++) {
        TreeItem item;
        if (ExtractTreeItem(hProcess, listbox, i, &item)) {
            if (item.position == 0 && _stricmp(item.text, "POUs") == 0) {
                pousIndex = i;
                break;
            }
        }
    }
    
    if (pousIndex >= 0) {
        // Klikni dvakrat pomoci ToggleListBoxItem
        ToggleListBoxItem(listbox, pousIndex);
        Sleep(50);
        ToggleListBoxItem(listbox, pousIndex);
        Sleep(50);
    }
    
    // 5. Najdi finalni polozku
    printf("\n[FINALIZACE] Hledam cilovou polozku '%s'...\n", searchText);
    
    itemCount = GetListBoxItemCount(listbox);
    for (int i = 0; i < itemCount; i++) {
        TreeItem item;
        if (ExtractTreeItem(hProcess, listbox, i, &item)) {
            if (_stricmp(item.text, searchText) == 0) {
                printf("\n[OK] NALEZENO! Index: %d, Level: %d\n", i, item.position);
                free(cache);
                return i;
            }
        }
    }
    
    printf("[X] Polozka nenalezena v expandovanem stromu!\n");
    free(cache);
    return -1;
}

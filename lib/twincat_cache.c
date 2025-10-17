#include "twincat_cache.h"
#include "twincat_navigator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Pomocna funkce: Zjisti nazev projektu z prvni polozky
void GetProjectName(HWND listbox, HANDLE hProcess, char* projectName, int maxLen) {
    TreeItem item;
    if (ExtractTreeItem(hProcess, listbox, 0, &item)) {
        strncpy(projectName, item.text, maxLen - 1);
        projectName[maxLen - 1] = '\0';
    } else {
        strcpy(projectName, "Unknown");
    }
}

// Expandne VSECHNY slozky v projektu s vypisem progressu
int ExpandAllFoldersWrapper(HWND listbox, HANDLE hProcess) {
    printf("\n[EXPANDALL] Oteviram vsechny slozky...\n");
    
    int iteration = 0;
    int maxIterations = 100;
    
    while (iteration < maxIterations) {
        int itemCount = GetListBoxItemCount(listbox);
        bool foundClosed = false;
        
        printf("  [Iterace %d] Polozek: %d\n", iteration, itemCount);
        
        for (int i = 0; i < itemCount; i++) {
            TreeItem item;
            if (!ExtractTreeItem(hProcess, listbox, i, &item)) {
                continue;
            }
            
            int folderState = GetFolderState(listbox, hProcess, i);
            
            // Je to zavrena slozka?
            if ((item.hasChildren && folderState == 0) || 
                (!item.hasChildren && folderState == 0)) {
                
                int countBefore = itemCount;
                ToggleListBoxItem(listbox, i);
                Sleep(1);
                SendMessage(listbox, LB_SETCURSEL, (WPARAM)-1, 0);
                
                int countAfter = GetListBoxItemCount(listbox);
                if (countAfter > countBefore) {
                    printf("    [%d] '%s' otevreno (+%d)\n", i, item.text, countAfter - countBefore);
                    foundClosed = true;
                    break; // Restart iterace
                }
            }
        }
        
        if (!foundClosed) {
            printf("[OK] Vsechny slozky otevreny!\n");
            return GetListBoxItemCount(listbox);
        }
        
        iteration++;
    }
    
    printf("[WARNING] Dosazeno max iteraci!\n");
    return GetListBoxItemCount(listbox);
}

// Nacteni celeho stromu do cache (vsechny slozky musi byt otevrene!)
int LoadFullTree(HWND listbox, HANDLE hProcess, CachedItem* cache, int maxItems) {
    printf("\n[LOADTREE] Nacitam cely strom do cache...\n");
    
    int itemCount = GetListBoxItemCount(listbox);
    if (itemCount > maxItems) {
        printf("[WARNING] Prilis mnoho polozek! (%d > %d)\n", itemCount, maxItems);
        itemCount = maxItems;
    }
    
    // Zasobnik pro budovani cest
    char levelTexts[10][256] = {0};
    int prevLevel = -1;
    
    for (int i = 0; i < itemCount; i++) {
        TreeItem item;
        if (!ExtractTreeItem(hProcess, listbox, i, &item)) {
            continue;
        }
        
        cache[i].index = i;
        strncpy(cache[i].text, item.text, 255);
        cache[i].level = item.position;
        cache[i].hasChildren = item.hasChildren;
        cache[i].flags = item.flags;
        
        // Uloz text pro aktualni level
        strncpy(levelTexts[item.position], item.text, 255);
        
        // Sestav cestu (parent1/parent2/.../item)
        cache[i].path[0] = '\0';
        for (int level = 0; level <= item.position; level++) {
            if (level > 0) {
                strcat(cache[i].path, "/");
            }
            strcat(cache[i].path, levelTexts[level]);
        }
        
        prevLevel = item.position;
    }
    
    printf("[OK] Nacteno %d polozek do cache\n", itemCount);
    return itemCount;
}

// Zavri vsechny slozky od nejvyssiho levelu dolu (pouziva cache)
void CollapseAllFoldersFromCache(HWND listbox, HANDLE hProcess, CachedItem* cache, int count) {
    printf("\n[COLLAPSE] Zaviram slozky od nejvyssiho levelu...\n");
    
    // Najdi nejvyssi level
    int maxLevel = 0;
    for (int i = 0; i < count; i++) {
        if (cache[i].hasChildren && cache[i].level > maxLevel) {
            maxLevel = cache[i].level;
        }
    }
    
    printf("  Nejvyssi level: %d\n", maxLevel);
    
    // Zavri slozky od nejvyssiho levelu k L1
    for (int level = maxLevel; level >= 1; level--) {
        printf("  Zaviram level %d...\n", level);
        int closedCount = 0;
        
        for (int i = count - 1; i >= 0; i--) {  // Odzadu!
            if (cache[i].hasChildren && cache[i].level == level) {
                // Najdi aktualni index ve skutecnem ListBoxu podle textu a levelu
                int currentCount = GetListBoxItemCount(listbox);
                int actualIndex = -1;
                
                for (int j = 0; j < currentCount; j++) {
                    TreeItem item;
                    if (ExtractTreeItem(hProcess, listbox, j, &item)) {
                        if (strcmp(item.text, cache[i].text) == 0 && item.position == cache[i].level) {
                            actualIndex = j;
                            break;
                        }
                    }
                }
                
                if (actualIndex == -1) {
                    continue; // Polozka uz neni viditelna (parent zavren)
                }
                
                // Over jestli je otevrena
                int folderState = GetFolderState(listbox, hProcess, actualIndex);
                
                if (folderState == 1) {  // Otevrena
                    printf("    [cache:%d -> listbox:%d] '%s' (L%d): Zaviram...\n", 
                           i, actualIndex, cache[i].text, cache[i].level);
                    ToggleListBoxItem(listbox, actualIndex);
                    Sleep(1);
                    SendMessage(listbox, LB_SETCURSEL, (WPARAM)-1, 0);
                    closedCount++;
                }
            }
        }
        printf("    Zavreno: %d slozek\n", closedCount);
    }
    
    // Dvojite kliknuti na POUs pro vymazani modreho zvyrazneni
    printf("  Odstranuji modre zvyrazneni (dvojite kliknuti na POUs)...\n");
    SetFocus(listbox);
    Sleep(100);
    
    // Prvni kliknuti - otevre POUs
    SendMessage(listbox, LB_SETCURSEL, 0, 0);
    Sleep(100);
    PostMessage(listbox, WM_KEYDOWN, VK_RETURN, 0);
    Sleep(500);
    
    // Druhe kliknuti - zavre POUs
    SendMessage(listbox, LB_SETCURSEL, 0, 0);
    Sleep(100);
    PostMessage(listbox, WM_KEYDOWN, VK_RETURN, 0);
    Sleep(500);
    
    printf("[OK] Slozky zavreny!\n");
}

// Ulozeni cache do JSON souboru
bool SaveCacheToFile(const char* filename, CachedItem* cache, int count, const char* projectName) {
    printf("\n[SAVECACHE] Ukladam cache do '%s'...\n", filename);
    
    FILE* f = fopen(filename, "w");
    if (!f) {
        printf("[X] Nelze otevrit soubor pro zapis!\n");
        return false;
    }
    
    // Ziskej timestamp
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", localtime(&now));
    
    // JSON header
    fprintf(f, "{\n");
    fprintf(f, "  \"project\": \"%s\",\n", projectName);
    fprintf(f, "  \"timestamp\": \"%s\",\n", timestamp);
    fprintf(f, "  \"itemCount\": %d,\n", count);
    fprintf(f, "  \"items\": [\n");
    
    // Items
    for (int i = 0; i < count; i++) {
        fprintf(f, "    {\n");
        fprintf(f, "      \"index\": %d,\n", cache[i].index);
        fprintf(f, "      \"text\": \"%s\",\n", cache[i].text);
        fprintf(f, "      \"level\": %d,\n", cache[i].level);
        fprintf(f, "      \"path\": \"%s\",\n", cache[i].path);
        fprintf(f, "      \"hasChildren\": %s,\n", cache[i].hasChildren ? "true" : "false");
        fprintf(f, "      \"flags\": \"0x%06X\"\n", cache[i].flags);
        fprintf(f, "    }%s\n", (i < count - 1) ? "," : "");
    }
    
    fprintf(f, "  ]\n");
    fprintf(f, "}\n");
    
    fclose(f);
    printf("[OK] Cache ulozen (%d polozek)\n", count);
    return true;
}

// Nacteni cache ze souboru
int LoadCacheFromFile(const char* filename, CachedItem* cache, int maxItems) {
    printf("\n[LOADCACHE] Nacitam cache z '%s'...\n", filename);
    
    FILE* f = fopen(filename, "r");
    if (!f) {
        printf("[INFO] Cache soubor neexistuje\n");
        return 0;
    }
    
    // Jednoduchy parser - preskoc header, hledej items
    char line[2048];
    int count = 0;
    bool inItems = false;
    
    while (fgets(line, sizeof(line), f) && count < maxItems) {
        if (strstr(line, "\"items\":")) {
            inItems = true;
            continue;
        }
        
        if (!inItems) continue;
        
        // Parsuj index
        if (strstr(line, "\"index\":")) {
            sscanf(line, " \"index\": %d", &cache[count].index);
        }
        // Parsuj text
        else if (strstr(line, "\"text\":")) {
            char* start = strchr(line, '"');
            if (start) {
                start = strchr(start + 1, '"');
                if (start) {
                    start++;
                    char* end = strchr(start, '"');
                    if (end) {
                        int len = end - start;
                        if (len > 255) len = 255;
                        strncpy(cache[count].text, start, len);
                        cache[count].text[len] = '\0';
                    }
                }
            }
        }
        // Parsuj level
        else if (strstr(line, "\"level\":")) {
            sscanf(line, " \"level\": %d", &cache[count].level);
        }
        // Parsuj path
        else if (strstr(line, "\"path\":")) {
            char* start = strchr(line, '"');
            if (start) {
                start = strchr(start + 1, '"');
                if (start) {
                    start++;
                    char* end = strchr(start, '"');
                    if (end) {
                        int len = end - start;
                        if (len > 1023) len = 1023;
                        strncpy(cache[count].path, start, len);
                        cache[count].path[len] = '\0';
                    }
                }
            }
        }
        // Parsuj hasChildren
        else if (strstr(line, "\"hasChildren\":")) {
            cache[count].hasChildren = (strstr(line, "true") != NULL);
        }
        // Parsuj flags
        else if (strstr(line, "\"flags\":")) {
            sscanf(line, " \"flags\": \"0x%X\"", &cache[count].flags);
        }
        // Konec objektu
        else if (strstr(line, "}")) {
            count++;
        }
    }
    
    fclose(f);
    printf("[OK] Nacteno %d polozek z cache\n", count);
    return count;
}

// Najdi polozku v cache podle textu
int FindInCache(CachedItem* cache, int count, const char* searchText) {
    for (int i = 0; i < count; i++) {
        if (_stricmp(cache[i].text, searchText) == 0) {
            return i;
        }
    }
    return -1;
}

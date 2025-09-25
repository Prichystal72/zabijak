#include "twincat_navigator.h"

// Najde TwinCAT okno
HWND FindTwinCatWindow(void) {
    HWND hWnd = GetTopWindow(GetDesktopWindow());
    
    while (hWnd) {
        char windowTitle[512];
        if (GetWindowText(hWnd, windowTitle, sizeof(windowTitle))) {
            if (strstr(windowTitle, "TwinCAT") != NULL) {
                return hWnd;
            }
        }
        hWnd = GetNextWindow(hWnd, GW_HWNDNEXT);
    }
    return NULL;
}

// Najde project explorer ListBox
HWND FindProjectListBox(HWND parentWindow) {
    printf("🔍 Hledám project explorer ListBox...\n");

    HWND bestListBox = NULL;
    int bestScore = 0;
    RECT parentRect;
    GetWindowRect(parentWindow, &parentRect);
    int windowWidth = parentRect.right - parentRect.left;

    HWND childWindow = GetWindow(parentWindow, GW_CHILD);
    while (childWindow != NULL) {
        char className[256];
        GetClassName(childWindow, className, sizeof(className));

        if (strcmp(className, "ListBox") == 0) {
            RECT rect;
            GetWindowRect(childWindow, &rect);

            int itemCount = SendMessage(childWindow, LB_GETCOUNT, 0, 0);
            int height = rect.bottom - rect.top;

            // Skóre pro výběr nejlepšího ListBoxu
            int score = itemCount + height / 10;
            if (rect.left < windowWidth / 3) score += 100; // Bonus za levé umístění

            printf("  ListBox 0x%p: pozice (%d,%d), velikost %dx%d, položek: %d, skóre: %d\n",
                   childWindow, rect.left, rect.top,
                   rect.right - rect.left, height, itemCount, score);

            if (score > bestScore) {
                bestScore = score;
                bestListBox = childWindow;
            }
        }

        // Rekurzivně prohledej child okna
        HWND subChild = GetWindow(childWindow, GW_CHILD);
        while (subChild != NULL) {
            char subClassName[256];
            GetClassName(subChild, subClassName, sizeof(subClassName));

            if (strcmp(subClassName, "ListBox") == 0) {
                RECT rect;
                GetWindowRect(subChild, &rect);

                int itemCount = SendMessage(subChild, LB_GETCOUNT, 0, 0);
                int height = rect.bottom - rect.top;

                int score = itemCount + height / 10;
                if (rect.left < windowWidth / 3) score += 100;

                printf("    Sub-ListBox 0x%p: pozice (%d,%d), velikost %dx%d, položek: %d, skóre: %d\n",
                       subChild, rect.left, rect.top,
                       rect.right - rect.left, height, itemCount, score);

                if (score > bestScore) {
                    bestScore = score;
                    bestListBox = subChild;
                }
            }

            subChild = GetWindow(subChild, GW_HWNDNEXT);
        }

        childWindow = GetWindow(childWindow, GW_HWNDNEXT);
    }

    if (bestListBox) {
        printf("✅ Nejlepší ListBox: 0x%p (skóre: %d)\n", bestListBox, bestScore);
    } else {
        printf("❌ Žádný vhodný ListBox nenalezen!\n");
    }

    return bestListBox;
}

// Otevře TwinCAT proces pro čtení paměti
HANDLE OpenTwinCatProcess(HWND hListBox) {
    DWORD processId;
    GetWindowThreadProcessId(hListBox, &processId);
    return OpenProcess(PROCESS_VM_READ, FALSE, processId);
}

// Získá počet položek v ListBoxu
int GetListBoxItemCount(HWND hListBox) {
    return SendMessage(hListBox, LB_GETCOUNT, 0, 0);
}

// Extrahuje jednu položku stromu
bool ExtractTreeItem(HANDLE hProcess, HWND hListBox, int index, TreeItem* item) {
    if (!item) return false;
    
    // Inicializace
    memset(item, 0, sizeof(TreeItem));
    item->index = index;
    
    // Získej ItemData
    LRESULT itemData = SendMessage(hListBox, LB_GETITEMDATA, index, 0);
    if (itemData == LB_ERR || itemData == 0) {
        printf("DEBUG [%d]: ItemData failed (0x%lX)\n", index, (long)itemData);
        return false;
    }
    
    item->itemData = (DWORD)itemData;
    
    // Přečti strukturu ItemData
    DWORD structure[10];
    SIZE_T bytesRead;
    
    if (!ReadProcessMemory(hProcess, (void*)itemData, structure, sizeof(structure), &bytesRead)) {
        return false;
    }
    
    if (bytesRead < 24) return false;
    
    // Parse struktury
    item->position = structure[1];      // Pozice v hierarchii
    item->flags = structure[2];         // Flags
    item->textPtr = structure[5];       // Text pointer (offset 20)
    
    // OPRAVENO: hasChildren podle flags, ne podle paměti!
    // Zavřené složky mají FLAG_SPECIAL, ale stále mají děti
    item->hasChildren = (item->flags == FLAG_FOLDER || item->flags == FLAG_SPECIAL) ? 1 : 0;
    
    // Načti text
    if (item->textPtr > 0x400000 && item->textPtr < 0x80000000) {
        char textBuffer[MAX_TEXT_LENGTH] = {0};
        SIZE_T textRead;
        
        if (ReadProcessMemory(hProcess, (void*)item->textPtr, textBuffer, sizeof(textBuffer)-1, &textRead)) {
            // Text začíná na pozici 1 (přeskoč null byte)
            strncpy(item->text, textBuffer + 1, MAX_TEXT_LENGTH - 1);
            item->text[MAX_TEXT_LENGTH - 1] = '\0';
        }
    }
    
    // Určení typu podle flags
    switch(item->flags) {
        case FLAG_FOLDER:  
            item->type = "FOLDER"; 
            break;
        case FLAG_FILE:    
            item->type = "FILE"; 
            break;
        case FLAG_SPECIAL: 
            item->type = "SPECIAL"; 
            break;
        case FLAG_ACTION:  
            item->type = "ACTION"; 
            break;
        default:           
            item->type = "OTHER"; 
            break;
    }
    
    // Level určím podle pozice - jednoduše podle position hodnoty
    if (item->position == 0) {
        item->level = 0;  // Root level
    } else if (item->position < 10) {
        item->level = 1;  // První úroveň
    } else {
        item->level = 2;  // Vnořené
    }
    
    return strlen(item->text) > 0;
}

// Zobrazí strukturu stromu s ASCII značkami
void PrintTreeStructure(TreeItem* items, int count) {
    printf("=== STRUKTURA STROMU TWINCAT ===\n\n");
    
    for (int i = 0; i < count; i++) {
        TreeItem* item = &items[i];
        
        // Odsazení podle úrovně
        for (int j = 0; j < item->level; j++) {
            printf("  ");
        }
        
        // JEDNODUCHÉ ASCII ZNAČKY - POUZE podle FLAGS
        const char* mark = " ";
        
        if (item->flags == FLAG_FOLDER) {
            mark = "[+]";  // Otevřená složka s dětmi
        } else if (item->flags == FLAG_SPECIAL) {
            mark = "[-]";  // Zavřená složka (může mít děti)
        } else if (item->flags == FLAG_ACTION) {
            mark = "{A}";  // Akce
        } else if (item->flags == FLAG_FILE) {
            mark = " - ";  // Soubor
        } else {
            mark = " ? ";  // Neznámý typ
        }
        
        printf("[%02d] %s %s (flags=0x%lX, children=%d)\n", 
               item->index, mark, item->text, (long)item->flags, (int)item->hasChildren);
    }
    
    printf("\nCelkem: %d polozek\n", count);
}

// Rozbalí všechny zavřené složky [-]
bool ExpandAllFolders(HWND hListBox, HANDLE hProcess) {
    printf("🔄 Rozbaluji všechny zavřené složky...\n");
    
    bool expandedAny = false;
    int maxAttempts = 30;  // Snížený počet pokusů
    int processedItems[200] = {0};  // Větší buffer pro více položek
    int processedCount = 0;
    
    for (int attempt = 0; attempt < maxAttempts; attempt++) {
        // Přečti aktuální stav
        int itemCount = GetListBoxItemCount(hListBox);
        TreeItem* items = malloc(itemCount * sizeof(TreeItem));
        if (!items) return false;
        
        int extractedCount = 0;
        for (int i = 0; i < itemCount; i++) {
            if (ExtractTreeItem(hProcess, hListBox, i, &items[extractedCount])) {
                extractedCount++;
            }
        }
        
        // Najdi první zavřenou složku [-] kterou jsme ještě nezpracovali
        int targetIndex = -1;
        for (int i = 0; i < extractedCount; i++) {
            if (items[i].flags == FLAG_SPECIAL) {  // 0x4047D = zavřená složka
                // Zkontroluj jestli jsme tuto položku už nezpracovali
                bool alreadyProcessed = false;
                for (int j = 0; j < processedCount; j++) {
                    if (processedItems[j] == items[i].index) {
                        alreadyProcessed = true;
                        break;
                    }
                }
                
                if (!alreadyProcessed) {
                    targetIndex = items[i].index;
                    printf("  Rozbaluji [%d]: %s\n", targetIndex, items[i].text);
                    break;
                }
            }
        }
        
        free(items);
        
        if (targetIndex == -1) {
            printf("✅ Všechny složky rozbaleny!\n");
            break;
        }
        
        // Zaměř se na zavřenou složku
        if (!FocusOnItem(hListBox, targetIndex)) {
            printf("❌ Nepodařilo se zaměřit na položku %d\n", targetIndex);
            break;
        }
        
        // Ověř že focus je skutečně nastaven
        int currentSelection = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
        if (currentSelection != targetIndex) {
            printf("❌ Focus není nastaven správně (%d != %d)\n", currentSelection, targetIndex);
            break;
        }
        
        // Aktivuj okno a rozbal
        SetForegroundWindow(GetParent(hListBox));
        Sleep(50);
        
        // Kontrola před rozbalením - kolik je položek
        int itemsBefore = GetListBoxItemCount(hListBox);
        
        // Pošli WM_LBUTTONDBLCLK pro rozbalení (FUNGUJE!)
        SendMessage(hListBox, WM_LBUTTONDBLCLK, 0, MAKELPARAM(10, 10));
        
        Sleep(50);  // Kratší pauza - TwinCAT je rychlý
        
        // Kontrola po rozbalení - změnil se počet položek?
        int itemsAfter = GetListBoxItemCount(hListBox);
        if (itemsAfter > itemsBefore) {
            printf("    ✅ Rozbaleno! (%d -> %d položek)\n", itemsBefore, itemsAfter);
            expandedAny = true;
            
            // Kratší pauza mezi úspěšnými rozbaleními
            Sleep(20);
        } else {
            printf("    ⚠️  Nerozbalilo se (možná nemá děti)\n");
            // Označíme tuto položku jako zpracovanou aby jsme ji přeskočili příště
            if (processedCount < 199) {
                processedItems[processedCount++] = targetIndex;
            }
        }
    }
    
    return expandedAny;
}

// Zavře otevřenou složku (opačně k rozbalení)
bool CloseFolder(HWND hListBox, int index) {
    printf("    🔒 Zavírám složku [%d]\n", index);
    
    // Zaměř se na otevřenou složku
    if (!FocusOnItem(hListBox, index)) {
        printf("    ❌ Nepodařilo se zaměřit na položku %d\n", index);
        return false;
    }
    
    // Aktivuj okno 
    SetForegroundWindow(GetParent(hListBox));
    Sleep(20);
    
    // Kontrola před zavřením - kolik je položek
    int itemsBefore = GetListBoxItemCount(hListBox);
    
    // Pošli WM_LBUTTONDBLCLK pro zavření (stejné jako pro otevření)
    SendMessage(hListBox, WM_LBUTTONDBLCLK, 0, MAKELPARAM(10, 10));
    
    Sleep(50);
    
    // Kontrola po zavření - změnil se počet položek?
    int itemsAfter = GetListBoxItemCount(hListBox);
    if (itemsAfter < itemsBefore) {
        printf("    ✅ Zavřeno! (%d -> %d položek)\n", itemsBefore, itemsAfter);
        return true;
    } else {
        printf("    ⚠️  Nezavřelo se\n");
        return false;
    }
}

// Zaměří se na položku
bool FocusOnItem(HWND hListBox, int index) {
    LRESULT result = SendMessage(hListBox, LB_SETCURSEL, index, 0);
    return (result != LB_ERR);
}

// Extrahuje text z titulku okna pro vyhledávání
bool ExtractTargetFromTitle(const char* windowTitle, char* targetText, int maxLength) {
    printf("📝 Parsování titulku: '%s'\n", windowTitle);
    
    // Hledáme text v hranatých závorkách: "[text (něco)]"
    const char* start = strchr(windowTitle, '[');
    if (!start) {
        printf("❌ Nenalezena '[' v titulku\n");
        return false;
    }
    
    start++; // Přeskoč '['
    
    // Najdi konec - buď " (" nebo "]"
    const char* end = strstr(start, " (");
    if (!end) {
        end = strchr(start, ']');
        if (!end) {
            printf("❌ Nenalezen konec v titulku\n");
            return false;
        }
    }
    
    int len = end - start;
    if (len >= maxLength) len = maxLength - 1;
    if (len <= 0) {
        printf("❌ Prázdný text v titulku\n");
        return false;
    }
    
    strncpy(targetText, start, len);
    targetText[len] = '\0';
    
    // Ořízni mezery
    char* textStart = targetText;
    while (*textStart == ' ') textStart++;
    
    char* textEnd = targetText + strlen(targetText) - 1;
    while (textEnd > textStart && *textEnd == ' ') textEnd--;
    *(textEnd + 1) = '\0';
    
    if (textStart != targetText) {
        memmove(targetText, textStart, strlen(textStart) + 1);
    }
    
    printf("✅ Extrahovaný text: '%s'\n", targetText);
    return strlen(targetText) > 0;
}

// Najde položku podle textu v aktuální úrovni
int SearchInLevel(TreeItem* items, int count, const char* searchText) {
    for (int i = 0; i < count; i++) {
        if (strstr(items[i].text, searchText) != NULL) {
            return items[i].index;
        }
    }
    return -1;
}

// Inteligentní vyhledávání - rozbaluje jen potřebné cesty
int FindAndOpenPath(HWND hListBox, HANDLE hProcess, const char* searchText) {
    printf("🎯 Inteligentní hledání: '%s'\n", searchText);
    
    int maxAttempts = 50;
    int openedFolders[100];  // Stack otevřených složek pro případné zavření
    int openedCount = 0;
    
    for (int attempt = 0; attempt < maxAttempts; attempt++) {
        // Přečti aktuální stav stromu
        int itemCount = GetListBoxItemCount(hListBox);
        TreeItem* items = malloc(itemCount * sizeof(TreeItem));
        if (!items) return -1;
        
        int extractedCount = 0;
        for (int i = 0; i < itemCount; i++) {
            if (ExtractTreeItem(hProcess, hListBox, i, &items[extractedCount])) {
                extractedCount++;
            }
        }
        
        // Nejdřív zkontroluj jestli už není target viditelný
        int foundIndex = SearchInLevel(items, extractedCount, searchText);
        if (foundIndex != -1) {
            printf("✅ Cíl nalezen na pozici [%d]!\n", foundIndex);
            free(items);
            return foundIndex;
        }
        
        // Najdi první zavřenou složku [-] k rozbalení
        int folderToExpand = -1;
        for (int i = 0; i < extractedCount; i++) {
            if (items[i].flags == FLAG_SPECIAL) {  // 0x4047D = zavřená složka
                folderToExpand = items[i].index;
                printf("  🔓 Zkouším rozbalit [%d]: %s\n", folderToExpand, items[i].text);
                break;
            }
        }
        
        if (folderToExpand == -1) {
            printf("❌ Žádné další složky k rozbalení, cíl nenalezen\n");
            free(items);
            return -1;
        }
        
        // Zapamatuj si počet položek před rozbalením
        int itemsBefore = extractedCount;
        
        // Rozbal složku
        if (!FocusOnItem(hListBox, folderToExpand)) {
            printf("❌ Nepodařilo se zaměřit na složku\n");
            free(items);
            return -1;
        }
        
        SetForegroundWindow(GetParent(hListBox));
        Sleep(20);
        SendMessage(hListBox, WM_LBUTTONDBLCLK, 0, MAKELPARAM(10, 10));
        Sleep(50);
        
        // Kontrola jestli se rozbalilo
        int itemsAfter = GetListBoxItemCount(hListBox);
        if (itemsAfter <= itemsBefore) {
            printf("    ⚠️  Složka se nerozbalila\n");
            free(items);
            continue;
        }
        
        printf("    ✅ Rozbaleno (%d -> %d položek)\n", itemsBefore, itemsAfter);
        
        // Přečti nový stav a hledej target v nově odhalených položkách
        free(items);
        items = malloc(itemsAfter * sizeof(TreeItem));
        if (!items) return -1;
        
        extractedCount = 0;
        for (int i = 0; i < itemsAfter; i++) {
            if (ExtractTreeItem(hProcess, hListBox, i, &items[extractedCount])) {
                extractedCount++;
            }
        }
        
        // Hledej target v nově rozbalené části
        foundIndex = SearchInLevel(items, extractedCount, searchText);
        if (foundIndex != -1) {
            printf("✅ Cíl nalezen po rozbalení na pozici [%d]!\n", foundIndex);
            free(items);
            return foundIndex;
        }
        
        // Target nebyl nalezen - zapamatuj si rozbalnou složku pro možné zavření
        if (openedCount < 99) {
            openedFolders[openedCount++] = folderToExpand;
        }
        
        free(items);
    }
    
    printf("❌ Cíl nenalezen po všech pokusech\n");
    return -1;
}

// Najde položku podle textu (stará verze pro kompatibilitu)
int FindItemByText(TreeItem* items, int count, const char* searchText) {
    printf("🔍 Hledám text: '%s'\n", searchText);
    
    for (int i = 0; i < count; i++) {
        if (strstr(items[i].text, searchText) != NULL) {
            printf("✅ Nalezeno na pozici [%d]: %s\n", items[i].index, items[i].text);
            return items[i].index;
        }
    }
    
    printf("❌ Text '%s' nenalezen v %d položkách\n", searchText, count);
    return -1;
}
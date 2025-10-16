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
    printf("Hledam project explorer ListBox...\n");

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

            // Skore pro vyber nejlepsiho ListBoxu
            int score = itemCount + height / 10;
            if (rect.left < windowWidth / 3) score += 100; // Bonus za leve umisteni

            printf("  ListBox 0x%p: pozice (%d,%d), velikost %dx%d, polozek: %d, skore: %d\n",
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

                printf("    Sub-ListBox 0x%p: pozice (%d,%d), velikost %dx%d, polozek: %d, skore: %d\n",
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
        printf("Nejlepsi ListBox: 0x%p (skore: %d)\n", bestListBox, bestScore);
    } else {
        printf("Zadny vhodny ListBox nenalezen!\n");
    }

    return bestListBox;
}

// Otevre TwinCAT proces pro cteni pameti
HANDLE OpenTwinCatProcess(HWND hListBox) {
    DWORD processId;
    GetWindowThreadProcessId(hListBox, &processId);
    return OpenProcess(PROCESS_VM_READ, FALSE, processId);
}

// Ziska pocet polozek v ListBoxu
int GetListBoxItemCount(HWND hListBox) {
    return SendMessage(hListBox, LB_GETCOUNT, 0, 0);
}

// Extrahuje jednu polozku stromu
bool ExtractTreeItem(HANDLE hProcess, HWND hListBox, int index, TreeItem* item) {
    if (!item) return false;
    
    // Inicializace
    memset(item, 0, sizeof(TreeItem));
    item->index = index;
    
    // Ziskej ItemData
    LRESULT itemData = SendMessage(hListBox, LB_GETITEMDATA, index, 0);
    if (itemData == LB_ERR || itemData == 0) return false;
    
    item->itemData = (DWORD)itemData;
    
    // Precti strukturu ItemData
    DWORD structure[10];
    SIZE_T bytesRead;
    
    if (!ReadProcessMemory(hProcess, (void*)itemData, structure, sizeof(structure), &bytesRead)) {
        return false;
    }
    
    if (bytesRead < 24) return false;
    
    // Parse struktury
    item->position = structure[1];      // Pozice v hierarchii
    item->flags = structure[2];         // Flags
    item->hasChildren = structure[3];   // Ma podpolozky
    item->textPtr = structure[5];       // Text pointer (offset 20)
    
    // Nacti text (format: [metadata - 4-5 bajtu] [text...])
    // Ruzne polozky maji metadata ruzne delky, proto zkousime offset 1 a 5
    if (item->textPtr > 0x400000 && item->textPtr < 0x80000000) {
        char textBuffer[MAX_TEXT_LENGTH + 10] = {0};
        SIZE_T textRead;
        
        if (ReadProcessMemory(hProcess, (void*)item->textPtr, textBuffer, sizeof(textBuffer)-1, &textRead)) {
            // Zkus offset 1 (vetsi polozek - za jednim null bytem)
            int offset = 1;
            char* text = textBuffer + offset;
            
            // Kontrola, zda je na offsetu 1 validni text
            if (offset < textRead && text[0] >= 32 && text[0] <= 126) {
                size_t len = 0;
                while (len < MAX_TEXT_LENGTH - 1 && offset + len < textRead && 
                       text[len] >= 32 && text[len] <= 126) {
                    len++;
                }
                
                if (len >= 3) {
                    strncpy(item->text, text, len);
                    item->text[len] = '\0';
                }
            }
            
            // Pokud offset 1 nevysel, zkus offset 5 (za DWORD + null byte)
            if (strlen(item->text) < 3 && textRead > 5) {
                offset = 5;
                text = textBuffer + offset;
                
                if (text[0] >= 32 && text[0] <= 126) {
                    size_t len = 0;
                    while (len < MAX_TEXT_LENGTH - 1 && offset + len < textRead && 
                           text[len] >= 32 && text[len] <= 126) {
                        len++;
                    }
                    
                    if (len >= 3) {
                        strncpy(item->text, text, len);
                        item->text[len] = '\0';
                    }
                }
            }
        }
    }
    
    // Urceni typu a ikony podle flags
    switch(item->flags) {
        case FLAG_FOLDER:  
            item->type = "FOLDER"; 
            item->icon = "[F]"; 
            item->level = (item->position == 0) ? 0 : 1;
            break;
        case FLAG_FILE:    
            item->type = "FILE"; 
            item->icon = "[f]"; 
            item->level = 2;
            break;
        case FLAG_SPECIAL: 
            item->type = "SPECIAL"; 
            item->icon = "[S]"; 
            item->level = 1;
            break;
        case FLAG_ACTION:  
            item->type = "ACTION"; 
            item->icon = "[A]"; 
            item->level = 2;
            break;
        default:           
            item->type = "OTHER"; 
            item->icon = "[?]"; 
            item->level = 0;
            break;
    }
    
    // Specialni upravy levelu
    if (strcmp(item->text, "POUs") == 0) item->level = 0;
    else if (item->position >= 4) item->level = 2;
    
    return strlen(item->text) > 0;
}

// Zobrazi strukturu stromu
void PrintTreeStructure(TreeItem* items, int count) {
    printf("=== STRUKTURA STROMU TWINCAT ===\n\n");
    
    for (int i = 0; i < count; i++) {
        TreeItem* item = &items[i];
        
        // Odsazeni podle urovne
        for (int j = 0; j < item->level; j++) {
            printf("  ");
        }
        
        printf("[%02d] %s %s\n", item->index, item->icon, item->text);
        
        // Debug info (volitelné)
        #ifdef DEBUG_MODE
        printf("     (pos=%d, flags=0x%06X, children=%d, ptr=0x%08X)\n", 
               item->position, item->flags, item->hasChildren, item->textPtr);
        #endif
    }
    
    printf("\nCelkem: %d polozek\n", count);
}

// Rozbali vsechny slozky (zakladni implementace)
bool ExpandAllFolders(HWND hListBox) {
    // TODO: Implementovat rozbalovani pomoci klavesnice
    // Zatim jen placeholder
    return false;
}

// Zameri se na polozku
bool FocusOnItem(HWND hListBox, int index) {
    LRESULT result = SendMessage(hListBox, LB_SETCURSEL, index, 0);
    return (result != LB_ERR);
}

// Zjisti, zda je polozka otevrena (ma viditelne podpolozky)
bool IsItemExpanded(HWND hListBox, HANDLE hProcess, int index) {
    int totalCount = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
    
    // Pokud je to posledni polozka, nemuze mit viditelne deti
    if (index >= totalCount - 1) {
        return false;
    }
    
    // Nacti level aktualni polozky
    LRESULT itemData = SendMessage(hListBox, LB_GETITEMDATA, index, 0);
    if (itemData == LB_ERR || itemData == 0) {
        return false;
    }
    
    DWORD structure[STRUCT_FIELD_COUNT] = {0};
    SIZE_T bytesRead = 0;
    
    if (!ReadProcessMemory(hProcess, (void*)itemData, structure, sizeof(structure), &bytesRead) || bytesRead < 24) {
        return false;
    }
    
    int currentLevel = (int)structure[1];  // Level je na offsetu 1
    
    // Nacti level dalsi polozky
    LRESULT nextItemData = SendMessage(hListBox, LB_GETITEMDATA, index + 1, 0);
    if (nextItemData == LB_ERR || nextItemData == 0) {
        return false;
    }
    
    DWORD nextStructure[STRUCT_FIELD_COUNT] = {0};
    SIZE_T nextBytesRead = 0;
    
    if (!ReadProcessMemory(hProcess, (void*)nextItemData, nextStructure, sizeof(nextStructure), &nextBytesRead) || nextBytesRead < 24) {
        return false;
    }
    
    int nextLevel = (int)nextStructure[1];
    
    // Pokud dalsi polozka ma vyssi level, znamena to ze aktualni polozka je otevrena
    return nextLevel > currentLevel;
}

// Zjisti stav slozky pomoci structure[3] (spolehlivejsi nez IsItemExpanded)
// Vraci: 1 = otevrena (ma viditelne deti), 0 = zavrena, -1 = chyba
int GetFolderState(HWND hListBox, HANDLE hProcess, int index) {
    // Nacti ItemData
    LRESULT itemData = SendMessage(hListBox, LB_GETITEMDATA, index, 0);
    if (itemData == LB_ERR || itemData == 0) {
        return -1;  // Chyba
    }
    
    // Nacti strukturu
    DWORD structure[STRUCT_FIELD_COUNT] = {0};
    SIZE_T bytesRead = 0;
    
    if (!ReadProcessMemory(hProcess, (void*)itemData, structure, sizeof(structure), &bytesRead) || bytesRead < 24) {
        return -1;  // Chyba cteni
    }
    
    // structure[3] obsahuje stav slozky:
    // 0 = zavrena / nema viditelne deti
    // 1 = otevrena / ma viditelne deti
    return (int)structure[3];
}

// Prepne (otevre/zavre) polozku v ListBoxu
// Vraci: pocet nove zobrazenych polozek (kladne = otevreno, zaporne = zavreno, 0 = chyba nebo zadna zmena)
int ToggleListBoxItem(HWND hListBox, int index) {
    // Ziskej pocet polozek PRED akci
    int countBefore = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
    if (countBefore == LB_ERR) {
        return 0;
    }
    
    // Zamer na ListBox
    SetFocus(hListBox);
    Sleep(50);
    
    // Vyber polozku
    LRESULT result = SendMessage(hListBox, LB_SETCURSEL, index, 0);
    if (result == LB_ERR) {
        return 0;
    }
    Sleep(50);
    
    // Posli RETURN pro otevreni/zavreni
    PostMessage(hListBox, WM_KEYDOWN, VK_RETURN, 0);
    PostMessage(hListBox, WM_KEYUP, VK_RETURN, 0);
    
    Sleep(100);  // Pauza pro dokonceni akce
    
    // Ziskej pocet polozek PO akci
    int countAfter = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
    if (countAfter == LB_ERR) {
        return 0;
    }
    
    // Vrat rozdil (+ = otevreno, - = zavreno)
    return countAfter - countBefore;
}

// Analyzuje celou paměť procesu a najde všechny textové řetězce (WORKING VERSION)
bool AnalyzeFullMemoryStructure(HANDLE hProcess, const char* outputFileName) {
    printf("🔍 === ANALÝZA PAMĚTI PROCESU ===\n");
    printf("🎯 Používám working přístup přes ItemData...\n");
    
    FILE* file = fopen(outputFileName, "w");
    if (!file) {
        printf("❌ Nelze vytvořit soubor %s\n", outputFileName);
        return false;
    }
    
    fprintf(file, "=== WORKING ANALÝZA PAMĚTI TWINCAT ===\n");
    fprintf(file, "Použití ItemData přístupu pro hledání všech ST_ položek\n\n");
    
    // Najdi TwinCAT okno a ListBox
    HWND twincatWindow = FindTwinCatWindow();
    if (!twincatWindow) {
        fprintf(file, "❌ TwinCAT okno nenalezeno!\n");
        fclose(file);
        return false;
    }
    
    HWND listbox = FindProjectListBox(twincatWindow);
    if (!listbox) {
        fprintf(file, "❌ ListBox nenalezen!\n");
        fclose(file);
        return false;
    }
    
    int itemCount = GetListBoxItemCount(listbox);
    fprintf(file, "✅ ListBox nalezen: 0x%p\n", (void*)listbox);
    fprintf(file, "📊 Celkem položek: %d\n\n", itemCount);
    
    int foundItems = 0;
    int targetFound = 0;
    
    // Analyzuj každou položku v ListBoxu
    for (int i = 0; i < itemCount; i++) {
        LRESULT itemData = SendMessage(listbox, LB_GETITEMDATA, i, 0);
        
        if (itemData == LB_ERR || itemData == 0) {
            continue;
        }
        
        // Přečti strukturu ItemData
        DWORD structure[16];
        SIZE_T bytesRead;
        
        if (!ReadProcessMemory(hProcess, (void*)itemData, structure, sizeof(structure), &bytesRead)) {
            continue;
        }
        
        if (bytesRead < 24) continue;
        
        // Text pointer je na offsetu 20 (index 5)
        DWORD textPtr = structure[5];
        
        if (textPtr > 0x400000 && textPtr < 0x80000000) {
            char textBuffer[512];
            SIZE_T textRead;
            
            if (ReadProcessMemory(hProcess, (void*)textPtr, textBuffer, sizeof(textBuffer)-1, &textRead)) {
                // Text často začíná na pozici 1 (přeskoč null byte)
                for (int offset = 0; offset < 4; offset++) {
                    if (offset >= textRead) continue;
                    
                    char* text = textBuffer + offset;
                    size_t len = 0;
                    
                    // Extrahuj printable text
                    while (len < 200 && offset + len < textRead) {
                        char c = text[len];
                        if (c >= 32 && c <= 126) {
                            len++;
                        } else {
                            break;
                        }
                    }
                    
                    if (len > 3) {
                        text[len] = '\0';
                        
                        // Zapiš všechny texty
                        fprintf(file, "[%03d] 0x%08X -> 0x%08X: %s\n", 
                               i, (DWORD)itemData, textPtr, text);
                        foundItems++;
                        
                        // Speciální označení pro ST_ položky
                        if (strstr(text, "ST_") == text) {
                            fprintf(file, "    🎯 ST_ POLOŽKA!\n");
                            printf("🎯 ST_ nalezeno [%d]: %s\n", i, text);
                            
                            if (strstr(text, "ST_Markiere_WT_NIO") != NULL) {
                                fprintf(file, "    � >>> TARGET NALEZEN! <<<\n");
                                printf("� TARGET ST_Markiere_WT_NIO nalezen na pozici %d!\n", i);
                                targetFound = 1;
                            }
                        }
                        
                        break; // Použij první platný text
                    }
                }
            }
        }
    }
    
    fprintf(file, "\n=== SOUHRN ===\n");
    fprintf(file, "Prohledáno ItemData položek: %d\n", itemCount);
    fprintf(file, "Nalezeno textů: %d\n", foundItems);
    fprintf(file, "Target ST_Markiere_WT_NIO: %s\n", targetFound ? "NALEZEN" : "NENALEZEN");
    
    fclose(file);
    
    printf("✅ ItemData analýza dokončena: %d textů z %d položek\n", foundItems, itemCount);
    printf("📁 Výsledky uloženy do: %s\n", outputFileName);
    
    if (targetFound) {
        printf("🏆 TARGET ST_Markiere_WT_NIO byl nalezen!\n");
    }
    
    return foundItems > 0;
}

// NOVÁ FUNKCE: Hledá kompletní projektovou strukturu v paměti (všechny ST_ položky)
bool SearchCompleteProjectStructure(HANDLE hProcess, const char* outputFileName) {
    printf("🧠 === HLEDÁNÍ KOMPLETNÍ PROJEKTOVÉ STRUKTURY ===\n");
    printf("🎯 Hledám všechny ST_ položky v celé paměti procesu...\n");
    
    FILE* file = fopen(outputFileName, "w");
    if (!file) {
        printf("❌ Nelze vytvořit soubor %s\n", outputFileName);
        return false;
    }
    
    fprintf(file, "=== KOMPLETNÍ PROJEKTOVÁ STRUKTURA V PAMĚTI ===\n");
    fprintf(file, "Hledání všech ST_ položek nezávisle na TreeView stavu\n\n");
    
    MEMORY_BASIC_INFORMATION mbi;
    LPBYTE currentAddress = (LPBYTE)0x400000;
    int foundItems = 0;
    int totalRegions = 0;
    
    while (currentAddress < (LPBYTE)0x80000000) {
        SIZE_T result = VirtualQueryEx(hProcess, currentAddress, &mbi, sizeof(mbi));
        
        if (result == 0) {
            currentAddress += 0x10000;
            continue;
        }
        
        // Prohledej jen committed readable paměť
        if ((mbi.State == MEM_COMMIT) && 
            (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))) {
            
            totalRegions++;
            
            SIZE_T regionSize = mbi.RegionSize;
            if (regionSize > 0x100000) regionSize = 0x100000; // Max 1MB
            
            BYTE* buffer = malloc(regionSize);
            if (buffer) {
                SIZE_T bytesRead;
                if (ReadProcessMemory(hProcess, mbi.BaseAddress, buffer, regionSize, &bytesRead)) {
                    
                    // Hledej všechny ST_ stringy
                    for (SIZE_T i = 0; i < bytesRead - 10; i++) {
                        if (buffer[i] == 'S' && buffer[i+1] == 'T' && buffer[i+2] == '_') {
                            char textBuffer[256] = {0};
                            int textLen = 0;
                            
                            // Extrahuj celý ST_ název
                            for (int j = 0; j < 255 && (i + j) < bytesRead; j++) {
                                char c = buffer[i + j];
                                if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || 
                                    (c >= '0' && c <= '9') || c == '_') {
                                    textBuffer[textLen++] = c;
                                } else {
                                    break;
                                }
                            }
                            
                            // Zapiš jen validní ST_ názvy (min 4 znaky)
                            if (textLen >= 4) {
                                textBuffer[textLen] = '\0';
                                DWORD address = (DWORD)((LPBYTE)mbi.BaseAddress + i);
                                
                                fprintf(file, "[%08lX] %s\n", address, textBuffer);
                                foundItems++;
                                
                                // Zvýrazni target
                                if (strcmp(textBuffer, "ST_Markiere_WT_NIO") == 0) {
                                    fprintf(file, "    🎯 >>> TARGET NALEZEN V PAMĚTI! <<<\n");
                                    printf("🎯 TARGET nalezen v paměti na: 0x%08lX\n", address);
                                }
                                
                                // Progress info každých 100 položek
                                if (foundItems % 100 == 0) {
                                    printf("  📊 Nalezeno %d ST_ položek...\n", foundItems);
                                }
                            }
                        }
                    }
                }
                free(buffer);
            }
        }
        
        currentAddress = (LPBYTE)mbi.BaseAddress + mbi.RegionSize;
    }
    
    fprintf(file, "\n=== SOUHRN KOMPLETNÍ STRUKTURY ===\n");
    fprintf(file, "Prohledáno paměťových regionů: %d\n", totalRegions);
    fprintf(file, "Nalezeno celkem ST_ položek: %d\n", foundItems);
    fprintf(file, "Poznámka: Zahrnuje všechny ST_ položky v paměti, ne jen TreeView\n");
    
    fclose(file);
    
    printf("✅ Kompletní struktura analyzována: %d ST_ položek nalezeno\n", foundItems);
    printf("📁 Kompletní dump uložen do: %s\n", outputFileName);
    
    return foundItems > 0;
}

// Hledá konkrétní text v paměti a vrací adresy
bool SearchInMemoryDump(HANDLE hProcess, const char* searchText, char* outputFileName) {
    printf("🎯 Hledám '%s' v paměti procesu...\n", searchText);
    
    FILE* file = fopen(outputFileName, "w");
    if (!file) {
        printf("❌ Nelze vytvořit soubor %s\n", outputFileName);
        return false;
    }
    
    fprintf(file, "=== HLEDÁNÍ '%s' V PAMĚTI ===\n\n", searchText);
    
    MEMORY_BASIC_INFORMATION mbi;
    LPBYTE currentAddress = (LPBYTE)0x400000;
    int foundCount = 0;
    
    while (currentAddress < (LPBYTE)0x80000000) {
        SIZE_T result = VirtualQueryEx(hProcess, currentAddress, &mbi, sizeof(mbi));
        
        if (result == 0) {
            currentAddress += 0x10000;
            continue;
        }
        
        if ((mbi.State == MEM_COMMIT) && 
            (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))) {
            
            SIZE_T regionSize = mbi.RegionSize;
            if (regionSize > 0x100000) regionSize = 0x100000;
            
            BYTE* buffer = malloc(regionSize);
            if (buffer) {
                SIZE_T bytesRead;
                if (ReadProcessMemory(hProcess, mbi.BaseAddress, buffer, regionSize, &bytesRead)) {
                    
                    // Hledej target text
                    for (SIZE_T i = 0; i < bytesRead - strlen(searchText); i++) {
                        if (memcmp(buffer + i, searchText, strlen(searchText)) == 0) {
                            DWORD address = (DWORD)((LPBYTE)mbi.BaseAddress + i);
                            
                            // Extrahuj kontext okolo nálezu
                            char context[512] = {0};
                            SIZE_T contextStart = (i > 50) ? i - 50 : 0;
                            SIZE_T contextLen = (bytesRead - contextStart > 100) ? 100 : bytesRead - contextStart;
                            
                            for (SIZE_T j = 0; j < contextLen; j++) {
                                char c = buffer[contextStart + j];
                                context[j] = (c >= 32 && c <= 126) ? c : '.';
                            }
                            context[contextLen] = '\0';
                            
                            fprintf(file, "NALEZENO na adrese: 0x%08X\n", address);
                            fprintf(file, "Kontext: %s\n\n", context);
                            
                            foundCount++;
                            printf("✅ Nalezen na adrese: 0x%08X\n", address);
                        }
                    }
                }
                free(buffer);
            }
        }
        
        currentAddress = (LPBYTE)mbi.BaseAddress + mbi.RegionSize;
    }
    
    fprintf(file, "Celkem nalezeno: %d výskytů\n", foundCount);
    fclose(file);
    
    printf("🎯 Hledání dokončeno: %d výskytů nalezeno\n", foundCount);
    printf("📁 Výsledky v: %s\n", outputFileName);
    
    return foundCount > 0;
}

// Extrahuje všechny položky přímo z paměti (bez ListBox omezení)
int ExtractAllItemsFromMemory(HANDLE hProcess, TreeItem* items, int maxItems) {
    printf("🧠 === EXTRAKCE Z PAMĚTI ===\n");
    
    MEMORY_BASIC_INFORMATION mbi;
    LPBYTE currentAddress = (LPBYTE)0x400000;
    int itemCount = 0;
    
    while (currentAddress < (LPBYTE)0x80000000 && itemCount < maxItems) {
        SIZE_T result = VirtualQueryEx(hProcess, currentAddress, &mbi, sizeof(mbi));
        
        if (result == 0) {
            currentAddress += 0x10000;
            continue;
        }
        
        if ((mbi.State == MEM_COMMIT) && 
            (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))) {
            
            SIZE_T regionSize = mbi.RegionSize;
            if (regionSize > 0x100000) regionSize = 0x100000;
            
            BYTE* buffer = malloc(regionSize);
            if (buffer) {
                SIZE_T bytesRead;
                if (ReadProcessMemory(hProcess, mbi.BaseAddress, buffer, regionSize, &bytesRead)) {
                    
                    // Hledej ST_ položky
                    for (SIZE_T i = 0; i < bytesRead - 10; i++) {
                        if (buffer[i] == 'S' && buffer[i+1] == 'T' && buffer[i+2] == '_') {
                            char text[256] = {0};
                            int textLen = 0;
                            
                            for (int j = 0; j < 255 && (i + j) < bytesRead; j++) {
                                char c = buffer[i + j];
                                if (c >= 32 && c <= 126) {
                                    text[textLen++] = c;
                                } else {
                                    break;
                                }
                            }
                            
                            if (textLen > 3 && itemCount < maxItems) {
                                text[textLen] = '\0';
                                
                                // Vytvořit TreeItem
                                TreeItem* item = &items[itemCount];
                                memset(item, 0, sizeof(TreeItem));
                                
                                item->index = itemCount;
                                strncpy(item->text, text, MAX_TEXT_LENGTH - 1);
                                item->textPtr = (DWORD)((LPBYTE)mbi.BaseAddress + i);
                                item->type = "MEMORY";
                                item->icon = "🧠";
                                
                                itemCount++;
                            }
                        }
                    }
                }
                free(buffer);
            }
        }
        
        currentAddress = (LPBYTE)mbi.BaseAddress + mbi.RegionSize;
    }
    
    printf("🧠 Z paměti extrahováno: %d ST_ položek\n", itemCount);
    return itemCount;
}
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
    if (itemData == LB_ERR || itemData == 0) return false;
    
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
    item->hasChildren = structure[3];   // Má podpoložky
    item->textPtr = structure[5];       // Text pointer (offset 20)
    
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
    
    // Určení typu a ikony podle flags
    switch(item->flags) {
        case FLAG_FOLDER:  
            item->type = "FOLDER"; 
            item->icon = "📁"; 
            item->level = (item->position == 0) ? 0 : 1;
            break;
        case FLAG_FILE:    
            item->type = "FILE"; 
            item->icon = "📄"; 
            item->level = 2;
            break;
        case FLAG_SPECIAL: 
            item->type = "SPECIAL"; 
            item->icon = "⚙️"; 
            item->level = 1;
            break;
        case FLAG_ACTION:  
            item->type = "ACTION"; 
            item->icon = "🔧"; 
            item->level = 2;
            break;
        default:           
            item->type = "OTHER"; 
            item->icon = "❓"; 
            item->level = 0;
            break;
    }
    
    // Speciální úpravy levelu
    if (strcmp(item->text, "POUs") == 0) item->level = 0;
    else if (item->position >= 4) item->level = 2;
    
    return strlen(item->text) > 0;
}

// Zobrazí strukturu stromu
void PrintTreeStructure(TreeItem* items, int count) {
    printf("=== STRUKTURA STROMU TWINCAT ===\n\n");
    
    for (int i = 0; i < count; i++) {
        TreeItem* item = &items[i];
        
        // Odsazení podle úrovně
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
    
    printf("\nCelkem: %d položek\n", count);
}

// Rozbalí všechny složky (základní implementace)
bool ExpandAllFolders(HWND hListBox) {
    // TODO: Implementovat rozbalování pomocí klávesnice
    // Zatím jen placeholder
    return false;
}

// Zaměří se na položku
bool FocusOnItem(HWND hListBox, int index) {
    LRESULT result = SendMessage(hListBox, LB_SETCURSEL, index, 0);
    return (result != LB_ERR);
}
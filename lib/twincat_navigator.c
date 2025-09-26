#include "twincat_navigator.h"
#include <ctype.h>

#define REMOTE_STRING_BUFFER 512
#define MIN_TREE_TEXT_LENGTH 2

#if defined(_WIN64)
#define POINTER_MAX_CANDIDATE 0x00007FFFFFFFFFFFULL
#else
#define POINTER_MAX_CANDIDATE 0x7FFFFFFFUL
#endif

#define POINTER_MIN_CANDIDATE 0x00010000UL

static bool isPrintableByte(unsigned char c) {
    return (c >= 32 && c < 127);
}

static void sanitizeText(char* text) {
    if (!text) return;

    size_t len = strlen(text);
    size_t write = 0;

    for (size_t i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)text[i];
        if (c >= 32 && c < 127) {
            text[write++] = (char)c;
        } else if (c == '\t') {
            text[write++] = ' ';
        }
    }

    text[write] = '\0';

    char* start = text;
    while (*start && isspace((unsigned char)*start)) start++;

    if (start != text) {
        memmove(text, start, strlen(start) + 1);
    }

    size_t newLen = strlen(text);
    while (newLen > 0 && isspace((unsigned char)text[newLen - 1])) {
        text[--newLen] = '\0';
    }

    write = 0;
    bool prevSpace = false;
    for (size_t i = 0; text[i]; ++i) {
        if (text[i] == ' ') {
            if (!prevSpace) {
                text[write++] = ' ';
                prevSpace = true;
            }
        } else {
            text[write++] = text[i];
            prevSpace = false;
        }
    }
    text[write] = '\0';
}

static bool extractAnsiSequence(const unsigned char* buffer, SIZE_T size, char* dest, size_t destLen) {
    if (!buffer || !dest || destLen == 0) return false;

    SIZE_T bestLen = 0;
    SIZE_T bestStart = size;

    for (SIZE_T i = 0; i < size; ++i) {
        if (isPrintableByte(buffer[i])) {
            SIZE_T j = i;
            while (j < size && isPrintableByte(buffer[j]) && (j - i) < destLen - 1) {
                j++;
            }
            SIZE_T segmentLen = j - i;
            if (segmentLen > bestLen) {
                bestLen = segmentLen;
                bestStart = i;
            }
            i = j;
        }
    }

    if (bestLen < MIN_TREE_TEXT_LENGTH || bestStart >= size) {
        return false;
    }

    if (bestLen >= destLen) {
        bestLen = destLen - 1;
    }

    memcpy(dest, buffer + bestStart, bestLen);
    dest[bestLen] = '\0';
    return true;
}

static bool extractTextFromRawBuffer(const unsigned char* buffer, SIZE_T size, char* dest, size_t destLen) {
    if (!buffer || size == 0 || !dest) return false;

    if (extractAnsiSequence(buffer, size, dest, destLen)) {
        return true;
    }

    unsigned char collapsed[REMOTE_STRING_BUFFER];
    SIZE_T collapsedLen = 0;

    for (SIZE_T i = 0; i < size && collapsedLen < sizeof(collapsed); ++i) {
        if (buffer[i] != 0) {
            collapsed[collapsedLen++] = buffer[i];
        }
    }

    if (collapsedLen > 0) {
        return extractAnsiSequence(collapsed, collapsedLen, dest, destLen);
    }

    return false;
}

static bool isPointerCandidate(DWORD_PTR value) {
    return value >= POINTER_MIN_CANDIDATE && value <= (DWORD_PTR)POINTER_MAX_CANDIDATE;
}

static bool extractStringFromPointer(HANDLE hProcess, DWORD_PTR address, char* dest, size_t destLen) {
    if (!dest || destLen == 0 || !isPointerCandidate(address)) {
        return false;
    }

    unsigned char buffer[REMOTE_STRING_BUFFER] = {0};
    SIZE_T bytesRead = 0;

    if (!ReadProcessMemory(hProcess, (LPCVOID)address, buffer, sizeof(buffer), &bytesRead)) {
        return false;
    }

    return extractTextFromRawBuffer(buffer, bytesRead, dest, destLen);
}

static bool isKnownFlag(DWORD flag) {
    return flag == FLAG_FOLDER || flag == FLAG_FILE || flag == FLAG_SPECIAL || flag == FLAG_ACTION;
}

static int findItemIndexByAddress(TreeItem* items, int count, DWORD_PTR address) {
    if (!items) return -1;
    for (int i = 0; i < count; ++i) {
        if (items[i].itemData == address) {
            return i;
        }
    }
    return -1;
}

static int detectParentOffset(TreeItem* items, int count) {
    if (!items || count <= 0) return -1;

    int bestOffset = -1;
    int bestScore = 0;

    for (int offset = 0; offset < STRUCT_FIELD_COUNT; ++offset) {
        int matches = 0;
        int zeros = 0;

        for (int i = 0; i < count; ++i) {
            if (offset >= items[i].rawCount) {
                continue;
            }

            DWORD_PTR candidate = items[i].raw[offset];

            if (candidate == 0) {
                zeros++;
                continue;
            }

            if (candidate == items[i].itemData) {
                continue;
            }

            if (findItemIndexByAddress(items, count, candidate) != -1) {
                matches++;
            }
        }

        int score = matches * 4 + zeros;
        if (score > bestScore) {
            bestScore = score;
            bestOffset = offset;
        }
    }

    if (bestScore == 0) {
        return -1;
    }

    return bestOffset;
}

static void buildTreeHierarchy(TreeItem* items, int count) {
    if (!items || count <= 0) return;

    for (int i = 0; i < count; ++i) {
        items[i].parentIndex = -1;
        items[i].firstChild = -1;
        items[i].nextSibling = -1;
        items[i].hasChildren = 0;
    }

    int parentOffset = detectParentOffset(items, count);

    if (parentOffset >= 0) {
        for (int i = 0; i < count; ++i) {
            if (parentOffset < items[i].rawCount) {
                DWORD_PTR candidate = items[i].raw[parentOffset];
                if (candidate != 0 && candidate != items[i].itemData) {
                    int parent = findItemIndexByAddress(items, count, candidate);
                    if (parent != -1) {
                        items[i].parentIndex = parent;
                    }
                }
            }
        }
    }

    for (int i = 0; i < count; ++i) {
        if (items[i].parentIndex == -1 && items[i].position > 0) {
            for (int j = i - 1; j >= 0; --j) {
                if (items[j].position < items[i].position) {
                    items[i].parentIndex = j;
                    break;
                }
            }
        }
    }

    for (int i = 0; i < count; ++i) {
        int parent = items[i].parentIndex;
        if (parent >= 0 && parent < count) {
            if (items[parent].firstChild == -1) {
                items[parent].firstChild = i;
            } else {
                int sibling = items[parent].firstChild;
                while (items[sibling].nextSibling != -1) {
                    sibling = items[sibling].nextSibling;
                }
                items[sibling].nextSibling = i;
            }
            items[parent].hasChildren = 1;
        }
    }

    for (int i = 0; i < count; ++i) {
        int level = 0;
        int guard = 0;
        int parent = items[i].parentIndex;

        while (parent != -1 && guard < count) {
            level++;
            parent = items[parent].parentIndex;
            guard++;
        }

        items[i].level = level;
    }
}

static void printTreeNodeRecursive(TreeItem* items, int index, const char* prefix, bool isLast) {
    if (!items || index < 0) return;

    char linePrefix[512];
    snprintf(linePrefix, sizeof(linePrefix), "%s%s", prefix, isLast ? "`-- " : "|-- ");

    const char* typeLabel = items[index].type ? items[index].type : "";
    const char* text = items[index].text[0] ? items[index].text : "(no text)";

    if (typeLabel[0] != '\0') {
        printf("%s[%02d] %s (%s, flags=0x%lX)\n",
               linePrefix,
               items[index].index,
               text,
               typeLabel,
               (unsigned long)items[index].flags);
    } else {
        printf("%s[%02d] %s (flags=0x%lX)\n",
               linePrefix,
               items[index].index,
               text,
               (unsigned long)items[index].flags);
    }

    if (items[index].firstChild != -1) {
        char childPrefix[512];
        snprintf(childPrefix, sizeof(childPrefix), "%s%s", prefix, isLast ? "    " : "|   ");

        int child = items[index].firstChild;
        while (child != -1) {
            bool childIsLast = (items[child].nextSibling == -1);
            printTreeNodeRecursive(items, child, childPrefix, childIsLast);
            child = items[child].nextSibling;
        }
    }
}

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

    memset(item, 0, sizeof(TreeItem));
    item->index = index;
    item->parentIndex = -1;
    item->firstChild = -1;
    item->nextSibling = -1;

    LRESULT itemData = SendMessage(hListBox, LB_GETITEMDATA, index, 0);
    if (itemData == LB_ERR || itemData == 0) {
        printf("DEBUG [%d]: ItemData failed (0x%lX)\n", index, (long)itemData);
        return false;
    }

    item->itemData = (DWORD_PTR)itemData;

    bool textResolved = false;

    WCHAR wideBuffer[MAX_TEXT_LENGTH];
    LRESULT textLen = SendMessageW(hListBox, LB_GETTEXTLEN, index, 0);
    if (textLen != LB_ERR && textLen > 0 && textLen < MAX_TEXT_LENGTH) {
        LRESULT copied = SendMessageW(hListBox, LB_GETTEXT, index, (LPARAM)wideBuffer);
        if (copied != LB_ERR) {
            if (copied >= MAX_TEXT_LENGTH) copied = MAX_TEXT_LENGTH - 1;
            wideBuffer[copied] = L'\0';

            int converted = WideCharToMultiByte(CP_UTF8, 0, wideBuffer, -1, item->text, MAX_TEXT_LENGTH, NULL, NULL);
            if (converted > 0) {
                sanitizeText(item->text);
                if (item->text[0] != '\0') {
                    textResolved = true;
                }
            }
        }
    }

    DWORD structure[STRUCT_FIELD_COUNT] = {0};
    SIZE_T bytesRead = 0;

    if (ReadProcessMemory(hProcess, (LPCVOID)item->itemData, structure, sizeof(structure), &bytesRead) && bytesRead >= sizeof(DWORD)) {
        item->rawCount = (int)(bytesRead / sizeof(DWORD));
        if (item->rawCount > STRUCT_FIELD_COUNT) {
            item->rawCount = STRUCT_FIELD_COUNT;
        }

        memcpy(item->raw, structure, item->rawCount * sizeof(DWORD));

        if (item->rawCount > 0) {
            DWORD candidatePos = structure[0];
            if (candidatePos < 1024) {
                item->position = candidatePos;
            } else if (item->rawCount > 1 && structure[1] < 1024) {
                item->position = structure[1];
            }
        }

        DWORD flagCandidate = 0;
        if (item->rawCount > 2) {
            flagCandidate = structure[2];
        }
        if (!isKnownFlag(flagCandidate) && item->rawCount > 4) {
            flagCandidate = structure[4];
        }
        item->flags = flagCandidate;

        if (item->rawCount > 5) {
            item->textPtr = structure[5];
        }

        if (!textResolved) {
            for (int i = 0; i < item->rawCount; ++i) {
                if (extractStringFromPointer(hProcess, structure[i], item->text, sizeof(item->text))) {
                    item->textPtr = structure[i];
                    textResolved = true;
                    break;
                }
            }
        }

        if (!textResolved) {
            if (extractTextFromRawBuffer((const unsigned char*)structure, bytesRead, item->text, sizeof(item->text))) {
                textResolved = true;
            }
        }
    } else {
        item->rawCount = 0;
    }

    sanitizeText(item->text);

    switch (item->flags) {
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
            item->type = "ITEM";
            break;
    }

    return item->text[0] != '\0';
}

// Zobrazí strukturu stromu s ASCII značkami
void PrintTreeStructure(TreeItem* items, int count) {
    printf("=== STRUKTURA STROMU TWINCAT ===\n\n");

    buildTreeHierarchy(items, count);

    int rootIndices[MAX_ITEMS];
    int rootCount = 0;

    for (int i = 0; i < count; ++i) {
        if (items[i].parentIndex == -1) {
            rootIndices[rootCount++] = i;
        }
    }

    if (rootCount == 0) {
        for (int i = 0; i < count; ++i) {
            printTreeNodeRecursive(items, i, "", i == count - 1);
        }
    } else {
        for (int r = 0; r < rootCount; ++r) {
            bool isLastRoot = (r == rootCount - 1);
            printTreeNodeRecursive(items, rootIndices[r], "", isLastRoot);
        }
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
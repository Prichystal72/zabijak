#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>

// Globální proměnné pro hook
HHOOK g_hHook = NULL;
HWND g_targetListBox = NULL;
char g_capturedTexts[100][256];
int g_textCount = 0;

// Najde název jednotky z titulku hlavního okna TwinCAT (pro maximalizované okno)
BOOL GetUnitFromMainWindow(HWND hwnd, char* unitName, int maxLength) {
    // Přečti titulek hlavního okna
    char mainTitle[1024];
    GetWindowText(hwnd, mainTitle, sizeof(mainTitle));
    printf("Hlavní okno TwinCAT: '%s'\n", mainTitle);

    // Extrahuj název jednotky z [název] v titulku hlavního okna
    char *start = strchr(mainTitle, '[');
    char *end = strchr(mainTitle, ']');

    if (start && end && end > start) {
        int len = end - start - 1;
        if (len >= maxLength) len = maxLength - 1;
        strncpy(unitName, start + 1, len);
        unitName[len] = '\0';
        printf("Jednotka z hlavního okna: '%s'\n", unitName);
        return TRUE;
    }

    printf("Nelze extrahovat název jednotky z hlavního okna!\n");
    return FALSE;
}// Najde project explorer ListBox (zjednodušená verze)
HWND FindProjectListBox(HWND parentWindow) {
    printf("Hledám project explorer ListBox...\n");

    // Projdi všechny child okna a najdi ListBox s nejvíce položkami
    HWND bestListBox = NULL;
    int bestItemCount = 0;
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
            if (rect.left < windowWidth / 3) score += 100; // Bonus za levý umístění

            printf("ListBox %p: pozice (%d,%d), velikost %dx%d, položek: %d, skóre: %d\n",
                   childWindow, rect.left, rect.top,
                   rect.right - rect.left, height, itemCount, score);

            if (score > bestItemCount) {
                bestItemCount = score;
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

                printf("  Sub-ListBox %p: pozice (%d,%d), velikost %dx%d, položek: %d, skóre: %d\n",
                       subChild, rect.left, rect.top,
                       rect.right - rect.left, height, itemCount, score);

                if (score > bestItemCount) {
                    bestItemCount = score;
                    bestListBox = subChild;
                }
            }

            subChild = GetWindow(subChild, GW_HWNDNEXT);
        }

        childWindow = GetWindow(childWindow, GW_HWNDNEXT);
    }

    if (bestListBox) {
        printf("✓ Vybrán project explorer ListBox: %p (skóre: %d)\n", bestListBox, bestItemCount);
    } else {
        printf("❌ Žádný ListBox nenalezen!\n");
    }

    return bestListBox;
}

// Rekurzivní funkce pro rozbalení všech uzlů ve stromu
void ExpandAllTreeNodes(HWND hTreeView, HTREEITEM hItem) {
    if (!hItem) return;

    // Rozbal tento uzel
    SendMessage(hTreeView, TVM_EXPAND, TVE_EXPAND, (LPARAM)hItem);

    // Rekurzivně rozbal všechny podřízené uzly
    HTREEITEM hChild = (HTREEITEM)SendMessage(hTreeView, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)hItem);
    while (hChild) {
        ExpandAllTreeNodes(hTreeView, hChild);
        hChild = (HTREEITEM)SendMessage(hTreeView, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hChild);
    }
}

// Rozbalí všechny uzly v TreeView - PŘÍMÝ PŘÍSTUP PŘES API
void ExpandAllNodes(HWND hTreeView) {
    printf("=== ROZBALUJI VŠECHNY UZLY VE STROMU (TreeView API) ===\n");

    // Najdi root položku
    HTREEITEM hRoot = (HTREEITEM)SendMessage(hTreeView, TVM_GETNEXTITEM, TVGN_ROOT, 0);
    if (!hRoot) {
        printf("❌ Žádný root uzel nenalezen!\n");
        return;
    }

    printf("Rozbaluji všechny uzly od root...\n");

    // Rekurzivně rozbal všechny uzly
    ExpandAllTreeNodes(hTreeView, hRoot);

    printf("✓ Všechny uzly rozbaleny přes TreeView API\n");
}

// Struktura pro uložení informací o položce TreeView
struct TreeItemInfo {
    int index;
    char text[256];
};

// Získá všechny texty přímo z controlu pomocí Windows API (bez otevírání oken)
int GetAllControlTexts(HWND hControl, struct TreeItemInfo* items, int maxItems) {
    printf("=== ZÍSKÁVÁM TEXTY PŘÍMO Z CONTROLU ===\n");
    
    char className[256];
    GetClassName(hControl, className, sizeof(className));
    printf("Typ controlu: %s\n", className);
    
    int collectedItems = 0;
    
    if (strcmp(className, "SysTreeView32") == 0) {
        // TreeView přístup
        printf("Používám TreeView API...\n");
        
        HTREEITEM hItem = (HTREEITEM)SendMessage(hControl, TVM_GETNEXTITEM, TVGN_ROOT, 0);
        int index = 0;
        
        while (hItem && collectedItems < maxItems) {
            TVITEM tvItem;
            char buffer[256];
            
            tvItem.mask = TVIF_TEXT;
            tvItem.hItem = hItem;
            tvItem.pszText = buffer;
            tvItem.cchTextMax = sizeof(buffer);
            
            if (SendMessage(hControl, TVM_GETITEM, 0, (LPARAM)&tvItem)) {
                items[collectedItems].index = index;
                strncpy(items[collectedItems].text, buffer, 255);
                items[collectedItems].text[255] = '\0';
                printf("TreeView položka %d: '%s'\n", index, buffer);
                collectedItems++;
            }
            
            // Získej další položku (včetně child items)
            HTREEITEM hNext = (HTREEITEM)SendMessage(hControl, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)hItem);
            if (!hNext) {
                hNext = (HTREEITEM)SendMessage(hControl, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hItem);
            }
            if (!hNext) {
                // Zkus parent a pak next
                HTREEITEM hParent = (HTREEITEM)SendMessage(hControl, TVM_GETNEXTITEM, TVGN_PARENT, (LPARAM)hItem);
                if (hParent) {
                    hNext = (HTREEITEM)SendMessage(hControl, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hParent);
                }
            }
            
            hItem = hNext;
            index++;
        }
        
    } else if (strcmp(className, "ListBox") == 0) {
        // ListBox přístup
        printf("Používám ListBox API...\n");
        
        int itemCount = SendMessage(hControl, LB_GETCOUNT, 0, 0);
        printf("ListBox má %d položek\n", itemCount);
        
        for (int i = 0; i < itemCount && collectedItems < maxItems; i++) {
            char buffer[256];
            int textLen = SendMessage(hControl, LB_GETTEXT, i, (LPARAM)buffer);
            
            if (textLen > 0) {
                items[collectedItems].index = i;
                strncpy(items[collectedItems].text, buffer, 255);
                items[collectedItems].text[255] = '\0';
                printf("ListBox položka %d: '%s'\n", i, buffer);
                collectedItems++;
            } else {
                printf("ListBox položka %d: (prázdná nebo nedostupná)\n", i);
            }
        }
    } else {
        printf("Neznámý typ controlu: %s\n", className);
    }
    
    printf("=== CELKEM ZÍSKÁNO %d TEXTŮ ===\n\n", collectedItems);
    return collectedItems;
}

/*
// Naviguje k hledané jednotce pomocí porovnání textů
BOOL NavigateToItem(HWND hwnd, HWND hListBox, char* targetUnit) {
    printf("Navigace k jednotce: %s (pomocí textového porovnání)\n", targetUnit);

    // Získej všechny texty z TreeView
    struct TreeItemInfo items[300];
    int itemCount = GetAllTreeItemTexts(hwnd, hListBox, items, 300);

    // Najdi shodu v textech
    for (int i = 0; i < itemCount; i++) {
        if (strcmp(items[i].text, targetUnit) == 0) {
            printf("✓ NALEZENA PŘESNÁ SHODA: '%s' na pozici %d!\n", targetUnit, items[i].index);
            
            // Nastav focus na správnou pozici
            SetForegroundWindow(hwnd);
            SetActiveWindow(hwnd);
            SetFocus(hListBox);
            SendMessage(hListBox, LB_SETCURSEL, items[i].index, 0);
            
            printf("✓ Focus nastaven na pozici %d v project exploreru!\n", items[i].index);
            return TRUE;
        }
    }

    printf("❌ Přesná shoda pro '%s' nebyla nalezena!\n", targetUnit);
    return FALSE;
}
*/


// Deklarace funkcí
void SetListBoxFocus();
LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam);
void InstallDrawItemHook(HWND targetListBox);
void UninstallDrawItemHook();

// Hlavní funkce programu - hlavní vstupní bod
int main() {
    printf("=== TWINCAT PROJECT NAVIGATOR (MDI Focus) ===\n");

    /*
    // Najdi TwinCAT okno
    HWND twinCATWindow = FindWindow(NULL, NULL);
    while (twinCATWindow != NULL) {
        char title[1024];
        GetWindowText(twinCATWindow, title, sizeof(title));
        if (strstr(title, "TwinCAT PLC Control") != NULL) {
            printf("Nalezeno TwinCAT okno: %s\n", title);
            break;
        }
        twinCATWindow = GetWindow(twinCATWindow, GW_HWNDNEXT);
    }

    if (twinCATWindow == NULL) {
        printf("TwinCAT okno nenalezeno!\n");
        printf("Zkontrolujte, že je TwinCAT PLC Control otevřený.\n");
        getchar();
        return 1;
    }
    */

    /*
    // Najdi project explorer ListBox
    HWND projectListBox = FindProjectListBox(twinCATWindow);
    if (projectListBox == NULL) {
        printf("Project explorer ListBox nenalezen!\n");
        getchar();
        return 1;
    }

    // Nejdříve zjistíme, jaký je to skutečně control
    printf("\n=== ANALÝZA TYPU CONTROLU ===\n");
    char className[256];
    GetClassName(projectListBox, className, sizeof(className));
    printf("Třída controlu: '%s'\n", className);
    
    // Zkusíme různé způsoby zjištění typu
    LONG style = GetWindowLong(projectListBox, GWL_STYLE);
    printf("Window Style: 0x%08lX\n", style);
    
    if (strcmp(className, "SysTreeView32") == 0) {
        printf("→ Je to TreeView!\n");
        // TODO: Implementovat TreeView API přístup
    } else if (strcmp(className, "ListBox") == 0) {
        printf("→ Je to ListBox!\n");
    } else {
        printf("→ Je to něco jiného: %s\n", className);
    }
    
    printf("\n=== ZÍSKÁVÁNÍ TEXTŮ BEZ OTEVÍRÁNÍ OKEN ===\n");
    struct TreeItemInfo items[300];
    int itemCount = GetAllControlTexts(projectListBox, items, 300);
    
    printf("\n=== SHRNUTÍ NALEZENÝCH POLOŽEK ===\n");
    for (int i = 0; i < itemCount; i++) {
        printf("Pozice %d: '%s'\n", items[i].index, items[i].text);
    }
    printf("Celkem nalezeno %d položek.\n", itemCount);
    
    // Zkusíme najít nějakou jednotku
    printf("\n=== HLEDÁNÍ JEDNOTEK ===\n");
    for (int i = 0; i < itemCount; i++) {
        if (strstr(items[i].text, "FB_") || strstr(items[i].text, "PRG_") || strstr(items[i].text, "FC_")) {
            printf("→ Možná jednotka na pozici %d: '%s'\n", items[i].index, items[i].text);
        }
    }
    */

    // Test focus na 25. položku
    SetListBoxFocus();

    printf("\nStiskněte Enter pro ukončení...\n");
    getchar();
    return 0;
}

// Jednoduchá funkce pro nastavení focus na 25. položku ListBoxu
void SetListBoxFocus() {
    printf("\n=== TEST FOCUS NA 25. POLOŽKU (bez Tab Control) ===\n");
    
    // Najdi TwinCAT okno
    HWND twinCATWindow = FindWindow(NULL, NULL);
    while (twinCATWindow != NULL) {
        char title[1024];
        GetWindowText(twinCATWindow, title, sizeof(title));
        if (strstr(title, "TwinCAT PLC Control") != NULL) {
            printf("Nalezeno TwinCAT okno: %s\n", title);
            break;
        }
        twinCATWindow = GetWindow(twinCATWindow, GW_HWNDNEXT);
    }
    
    if (!twinCATWindow) {
        printf("❌ TwinCAT okno nenalezeno!\n");
        return;
    }
    
    // Najdi všechny ListBoxy v TwinCAT okně
    printf("\n=== HLEDÁNÍ VŠECH LISTBOXŮ ===\n");
    HWND child = GetWindow(twinCATWindow, GW_CHILD);
    int listBoxCount = 0;
    
    while (child) {
        char className[256];
        GetClassName(child, className, sizeof(className));
        
        if (strcmp(className, "ListBox") == 0) {
            listBoxCount++;
            int itemCount = SendMessage(child, LB_GETCOUNT, 0, 0);
            BOOL isVisible = IsWindowVisible(child);
            
            printf("\n=== ListBox %d ===\n", listBoxCount);
            printf("Handle: 0x%08X\n", (unsigned int)child);
            printf("Položky: %d\n", itemCount);
            printf("Viditelný: %s\n", isVisible ? "ANO" : "NE");
            
            // Vypiš obsah ListBoxu
            if (itemCount > 0) {
                printf("OBSAH:\n");
                for (int i = 0; i < itemCount && i < 50; i++) { // max 50 položek
                    char buffer[512];
                    int textLen = SendMessage(child, LB_GETTEXT, i, (LPARAM)buffer);
                    
                    if (textLen > 0) {
                        printf("  [%2d] %s\n", i, buffer);
                    } else {
                        printf("  [%2d] (prázdná nebo nedostupná)\n", i);
                    }
                }
                
                if (itemCount > 50) {
                    printf("  ... a dalších %d položek\n", itemCount - 50);
                }
            } else {
                printf("OBSAH: (prázdný)\n");
            }
            
            // Je to náš cílový ListBox? Analyzujme strukturu přímo!
            if (child == (HWND)0x00070400) {
                printf("→ ✓ Toto je náš cílový ListBox!\n");
                printf("→ SPOUŠTÍM MEMORY DISASSEMBLER/ANALYZER\n");
                
                DWORD processId;
                GetWindowThreadProcessId(child, &processId);
                HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId);
                
                if (hProcess) {
                    printf("→ Získán přístup k procesu %lu\n", processId);
                    
                    int count = SendMessage(child, LB_GETCOUNT, 0, 0);
                    printf("→ ListBox má %d položek\n", count);
                    
                    // Analyzuj první 10 položek detailně
                    for (int item = 0; item < min(10, count); item++) {
                        printf("\n=== POLOŽKA %d ===\n", item);
                        
                        LRESULT itemData = SendMessage(child, LB_GETITEMDATA, item, 0);
                        if (itemData != LB_ERR && itemData != 0) {
                            printf("ItemData pointer: 0x%08lX\n", itemData);
                            
                            // Přečti prvních 200 bytů struktury jako hex dump
                            unsigned char buffer[200];
                            SIZE_T bytesRead;
                            
                            if (ReadProcessMemory(hProcess, (void*)itemData, buffer, sizeof(buffer), &bytesRead)) {
                                printf("Raw hex dump (%zu bytes):\n", bytesRead);
                                
                                for (size_t i = 0; i < bytesRead; i += 16) {
                                    printf("%04zX: ", i);
                                    
                                    // Hex část
                                    for (size_t j = 0; j < 16 && i + j < bytesRead; j++) {
                                        printf("%02X ", buffer[i + j]);
                                    }
                                    
                                    // Padding pro alignment
                                    for (size_t j = bytesRead - i; j < 16; j++) {
                                        printf("   ");
                                    }
                                    
                                    printf(" | ");
                                    
                                    // ASCII část
                                    for (size_t j = 0; j < 16 && i + j < bytesRead; j++) {
                                        unsigned char c = buffer[i + j];
                                        if (c >= 32 && c <= 126) {
                                            printf("%c", c);
                                        } else {
                                            printf(".");
                                        }
                                    }
                                    printf("\n");
                                }
                                
                                // Hledej string pointery v prvních 100 bytech
                                printf("\nAnalýza možných string pointerů:\n");
                                for (size_t offset = 0; offset < min(100, bytesRead); offset += 4) {
                                    if (offset + 4 <= bytesRead) {
                                        DWORD* ptr = (DWORD*)(buffer + offset);
                                        DWORD addr = *ptr;
                                        
                                        // Zkontroluj jestli vypadá jako platný pointer
                                        if (addr > 0x10000 && addr < 0x7FFFFFFF) {
                                            // Zkus číst string z této adresy
                                            char testStr[256] = {0};
                                            SIZE_T strBytesRead;
                                            
                                            if (ReadProcessMemory(hProcess, (void*)addr, testStr, sizeof(testStr)-1, &strBytesRead)) {
                                                // Zkontroluj jestli to vypadá jako validní string
                                                bool isValidString = strBytesRead > 0;
                                                size_t strLen = 0;
                                                for (size_t i = 0; i < strBytesRead && i < 100; i++) {
                                                    if (testStr[i] == 0) break;
                                                    if (testStr[i] < 32 || testStr[i] > 126) {
                                                        if (i < 3) isValidString = false; // Příliš brzy non-printable
                                                        break;
                                                    }
                                                    strLen++;
                                                }
                                                
                                                if (isValidString && strLen > 2 && strLen < 100) {
                                                    printf("  [+%03zu] 0x%08X -> '%.*s' (len:%zu)\n", 
                                                           offset, addr, (int)strLen, testStr, strLen);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    CloseHandle(hProcess);
                } else {
                    printf("→ ❌ Nelze získat přístup k procesu\n");
                }
                
                // Odstran hook
                UninstallDrawItemHook();
                
                return;
                
                // Získej více informací o ListBoxu
                DWORD processId;
                GetWindowThreadProcessId(child, &processId);
                printf("Process ID: %lu\n", processId);
                
                LONG_PTR style = GetWindowLongPtr(child, GWL_STYLE);
                printf("Style: 0x%08llX\n", style);
                printf("Owner-drawn: %s\n", (style & LBS_OWNERDRAWFIXED) ? "ANO" : "NE");
                printf("Has strings: %s\n", (style & LBS_HASSTRINGS) ? "ANO" : "NE");
                
                // Zkus různé ListBox zprávy pro více informací
                printf("\n→ TESTOVÁNÍ LISTBOX ZPRÁV:\n");
                
                // LB_GETITEMDATA - možná obsahuje pointery na texty
                for (int i = 0; i < 10 && i < itemCount; i++) {
                    LRESULT itemData = SendMessage(child, LB_GETITEMDATA, i, 0);
                    printf("Item[%d] data: 0x%08llX\n", i, itemData);
                }
                
                // LB_GETITEMHEIGHT
                int itemHeight = SendMessage(child, LB_GETITEMHEIGHT, 0, 0);
                printf("Item height: %d\n", itemHeight);
                
                // LB_GETITEMRECT
                for (int i = 0; i < 5 && i < itemCount; i++) {
                    RECT rect;
                    if (SendMessage(child, LB_GETITEMRECT, i, (LPARAM)&rect) != LB_ERR) {
                        printf("Item[%d] rect: (%ld,%ld) - (%ld,%ld)\n", 
                               i, rect.left, rect.top, rect.right, rect.bottom);
                    }
                }
                
                // Zkus posílat WM_DRAWITEM zprávy a zachytit je
                printf("\n→ POKUS O ZACHYCENÍ WM_DRAWITEM:\n");
                
                // Vynutit překreslení několika položek
                for (int i = 0; i < 5 && i < itemCount; i++) {
                    RECT itemRect;
                    if (SendMessage(child, LB_GETITEMRECT, i, (LPARAM)&itemRect) != LB_ERR) {
                        // Invalidate položku pro překreslení
                        InvalidateRect(child, &itemRect, TRUE);
                        UpdateWindow(child);
                        Sleep(50);
                    }
                }
                
                // ČTENÍ PAMĚTI PROCESU - hledání textů na pointerech
                printf("\n→ ČTENÍ PAMĚTI PROCESU TwinCAT:\n");
                
                HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId);
                if (hProcess) {
                    printf("✓ Proces otevřen pro čtení paměti\n");
                    
                    // Zkus číst data na pointerech z LB_GETITEMDATA
                    for (int i = 0; i < 10 && i < itemCount; i++) {
                        LRESULT itemDataPtr = SendMessage(child, LB_GETITEMDATA, i, 0);
                        if (itemDataPtr != 0 && itemDataPtr != LB_ERR) {
                            printf("\nItem[%d] pointer: 0x%08llX\n", i, itemDataPtr);
                            
                            // Čti paměť na této adrese
                            char buffer[512];
                            SIZE_T bytesRead;
                            
                            // Zkus číst jako string
                            if (ReadProcessMemory(hProcess, (LPCVOID)itemDataPtr, buffer, sizeof(buffer)-1, &bytesRead)) {
                                buffer[bytesRead] = '\0';
                                printf("  Direct string: ");
                                
                                // Vypiš jen tisknutelné znaky
                                for (SIZE_T j = 0; j < bytesRead && j < 100; j++) {
                                    if (buffer[j] >= 32 && buffer[j] <= 126) {
                                        printf("%c", buffer[j]);
                                    } else if (buffer[j] == 0) {
                                        break;
                                    } else {
                                        printf(".");
                                    }
                                }
                                printf("\n");
                            }
                            
                            // Zkus číst jako pointer na pointer (double indirection)
                            void* secondPtr;
                            if (ReadProcessMemory(hProcess, (LPCVOID)itemDataPtr, &secondPtr, sizeof(secondPtr), &bytesRead)) {
                                if (secondPtr != NULL && (ULONG_PTR)secondPtr > 0x10000) {
                                    printf("  Second pointer: 0x%p\n", secondPtr);
                                    
                                    // Čti na druhém pointeru
                                    if (ReadProcessMemory(hProcess, secondPtr, buffer, sizeof(buffer)-1, &bytesRead)) {
                                        buffer[bytesRead] = '\0';
                                        printf("  Double indirect: ");
                                        
                                        for (SIZE_T j = 0; j < bytesRead && j < 100; j++) {
                                            if (buffer[j] >= 32 && buffer[j] <= 126) {
                                                printf("%c", buffer[j]);
                                            } else if (buffer[j] == 0) {
                                                break;
                                            } else {
                                                printf(".");
                                            }
                                        }
                                        printf("\n");
                                    }
                                }
                            }
                            
                            // Dump hex pro debug a analýzu struktury
                            printf("  Raw hex: ");
                            unsigned char rawData[128];
                            if (ReadProcessMemory(hProcess, (LPCVOID)itemDataPtr, rawData, sizeof(rawData), &bytesRead)) {
                                for (SIZE_T j = 0; j < bytesRead && j < 32; j++) {
                                    printf("%02X ", rawData[j]);
                                }
                                printf("\n");
                                
                                // Analyzuj strukturu - hledej všechny možné pointery
                                for (int offset = 0; offset < 64; offset += 4) {
                                    if (offset + 4 <= bytesRead) {
                                        // Čti jako 32-bit pointer
                                        DWORD* ptr32 = (DWORD*)(rawData + offset);
                                        
                                        // Je to rozumný pointer? (0x08000000 - 0x20000000 range)
                                        if (*ptr32 > 0x08000000 && *ptr32 < 0x20000000) {
                                            printf("    Offset[%d]: možný pointer 0x%08X\n", offset, *ptr32);
                                            
                                            // Zkus číst string na tomto pointeru
                                            char strBuffer[256];
                                            SIZE_T strBytesRead;
                                            if (ReadProcessMemory(hProcess, (LPCVOID)*ptr32, strBuffer, sizeof(strBuffer)-1, &strBytesRead)) {
                                                strBuffer[strBytesRead] = '\0';
                                                
                                                // Zkontroluj, jestli to vypadá jako string
                                                BOOL isString = TRUE;
                                                int validChars = 0;
                                                for (SIZE_T k = 0; k < strBytesRead && k < 50; k++) {
                                                    if (strBuffer[k] == 0) break;
                                                    if (strBuffer[k] >= 32 && strBuffer[k] <= 126) {
                                                        validChars++;
                                                    } else if (strBuffer[k] < 32 && strBuffer[k] != 9 && strBuffer[k] != 10 && strBuffer[k] != 13) {
                                                        isString = FALSE;
                                                        break;
                                                    }
                                                }
                                                
                                                if (isString && validChars > 2) {
                                                    printf("      → STRING: '");
                                                    for (SIZE_T k = 0; k < strBytesRead && k < 100; k++) {
                                                        if (strBuffer[k] == 0) break;
                                                        if (strBuffer[k] >= 32 && strBuffer[k] <= 126) {
                                                            printf("%c", strBuffer[k]);
                                                        } else {
                                                            printf("\\x%02X", (unsigned char)strBuffer[k]);
                                                        }
                                                    }
                                                    printf("'\n");
                                                }
                                            }
                                        }
                                    }
                                    
                                    // Zkus i jako 64-bit pointer (pro x64)
                                    if (offset + 8 <= bytesRead) {
                                        ULONG_PTR* ptr64 = (ULONG_PTR*)(rawData + offset);
                                        
                                        // Je to rozumný 64-bit pointer?
                                        if (*ptr64 > 0x100000 && *ptr64 < 0x7FFFFFFF00000000ULL) {
                                            printf("    Offset[%d]: možný 64-bit pointer 0x%p\n", offset, (void*)*ptr64);
                                            
                                            // Zkus číst string
                                            char strBuffer[256];
                                            SIZE_T strBytesRead;
                                            if (ReadProcessMemory(hProcess, (LPCVOID)*ptr64, strBuffer, sizeof(strBuffer)-1, &strBytesRead)) {
                                                strBuffer[strBytesRead] = '\0';
                                                
                                                BOOL isString = TRUE;
                                                int validChars = 0;
                                                for (SIZE_T k = 0; k < strBytesRead && k < 50; k++) {
                                                    if (strBuffer[k] == 0) break;
                                                    if (strBuffer[k] >= 32 && strBuffer[k] <= 126) {
                                                        validChars++;
                                                    } else if (strBuffer[k] < 32 && strBuffer[k] != 9 && strBuffer[k] != 10 && strBuffer[k] != 13) {
                                                        isString = FALSE;
                                                        break;
                                                    }
                                                }
                                                
                                                if (isString && validChars > 2) {
                                                    printf("      → 64-BIT STRING: '");
                                                    for (SIZE_T k = 0; k < strBytesRead && k < 100; k++) {
                                                        if (strBuffer[k] == 0) break;
                                                        if (strBuffer[k] >= 32 && strBuffer[k] <= 126) {
                                                            printf("%c", strBuffer[k]);
                                                        } else {
                                                            printf("\\x%02X", (unsigned char)strBuffer[k]);
                                                        }
                                                    }
                                                    printf("'\n");
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    CloseHandle(hProcess);
                } else {
                    printf("❌ Nelze otevřít proces pro čtení paměti (error: %lu)\n", GetLastError());
                }
                
                return;
            }
        }
        child = GetWindow(child, GW_HWNDNEXT);
    }
    
    printf("Celkem nalezeno %d ListBoxů\n", listBoxCount);
    
    if (listBoxCount == 0) {
        printf("❌ Žádné ListBoxy nenalezeny!\n");
    } else {
        printf("❌ Cílový ListBox 0x00070400 nenalezen!\n");
    }
}

// Hook procedura pro zachycení WM_DRAWITEM zpráv
LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        MSG* pMsg = (MSG*)lParam;
        
        // Zachyť WM_DRAWITEM zprávy pro náš ListBox
        if (pMsg->message == WM_DRAWITEM && pMsg->hwnd != NULL) {
            DRAWITEMSTRUCT* pDIS = (DRAWITEMSTRUCT*)pMsg->lParam;
            
            // Je to náš cílový ListBox?
            if (pDIS && pDIS->hwndItem == g_targetListBox && pDIS->CtlType == ODT_LISTBOX) {
                printf("→ ZACHYCENO WM_DRAWITEM: Item %d\n", pDIS->itemID);
                
                // Získej HDC pro čtení textu
                HDC hdc = pDIS->hDC;
                RECT rect = pDIS->rcItem;
                
                // Zkus různé metody zachycení textu
                char buffer[256];
                
                // Metoda 1: ExtTextOut hook - bohužel složité
                // Metoda 2: Analyzuj itemData
                if (pDIS->itemData != 0) {
                    printf("  ItemData: 0x%08llX\n", pDIS->itemData);
                    
                    // Zkus přečíst data z paměti
                    DWORD processId;
                    GetWindowThreadProcessId(g_targetListBox, &processId);
                    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId);
                    
                    if (hProcess) {
                        // Zkus číst na různých offsetech
                        ULONG_PTR baseAddr = (ULONG_PTR)pDIS->itemData;
                        
                        // Offset 20 (z předchozí analýzy)
                        ULONG_PTR textPtr;
                        SIZE_T bytesRead;
                        if (ReadProcessMemory(hProcess, (LPCVOID)(baseAddr + 20), &textPtr, sizeof(textPtr), &bytesRead)) {
                            if (textPtr > 0x08000000 && textPtr < 0x20000000) {
                                // Zkus číst string
                                if (ReadProcessMemory(hProcess, (LPCVOID)textPtr, buffer, sizeof(buffer)-1, &bytesRead)) {
                                    buffer[bytesRead] = '\0';
                                    
                                    // Zkontroluj, jestli to vypadá jako string
                                    BOOL isValidString = TRUE;
                                    int validChars = 0;
                                    for (int i = 0; i < bytesRead && i < 100; i++) {
                                        if (buffer[i] == 0) break;
                                        if (buffer[i] >= 32 && buffer[i] <= 126) {
                                            validChars++;
                                        } else if (buffer[i] < 32 && buffer[i] != 9 && buffer[i] != 10 && buffer[i] != 13) {
                                            isValidString = FALSE;
                                            break;
                                        }
                                    }
                                    
                                    if (isValidString && validChars > 2) {
                                        printf("  ✓ TEXT: '%s'\n", buffer);
                                        
                                        // Ulož zachycený text
                                        if (g_textCount < 100 && pDIS->itemID < 100) {
                                            strncpy(g_capturedTexts[pDIS->itemID], buffer, 255);
                                            g_capturedTexts[pDIS->itemID][255] = '\0';
                                            if (pDIS->itemID >= g_textCount) {
                                                g_textCount = pDIS->itemID + 1;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        CloseHandle(hProcess);
                    }
                }
            }
        }
    }
    
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

// Instalace hook pro zachycení WM_DRAWITEM
void InstallDrawItemHook(HWND targetListBox) {
    g_targetListBox = targetListBox;
    g_textCount = 0;
    memset(g_capturedTexts, 0, sizeof(g_capturedTexts));
    
    // Zkusíme jednodušší přístup - scroll hook
    printf("→ Zkouším získat texty pomocí Scroll a Invalidation triků...\n");
    
    // Získej počet položek
    int count = SendMessage(targetListBox, LB_GETCOUNT, 0, 0);
    printf("→ ListBox má %d položek\n", count);
    
    // Proveď scroll test pro vynutí překreslení
    for (int i = 0; i < min(count, 50); i++) {
        // Nastav top index na každou položku
        SendMessage(targetListBox, LB_SETTOPINDEX, i, 0);
        Sleep(10);
        
        // Zkus získat text standardním způsobem (Unicode)
        wchar_t wBuffer[256] = {0};
        char buffer[256] = {0};
        int len = SendMessageW(targetListBox, LB_GETTEXT, i, (LPARAM)wBuffer);
        
        if (len > 0 && wcslen(wBuffer) > 0) {
            // Převeď Unicode na UTF-8
            WideCharToMultiByte(CP_UTF8, 0, wBuffer, -1, buffer, sizeof(buffer), NULL, NULL);
            strncpy(g_capturedTexts[g_textCount], buffer, sizeof(g_capturedTexts[0])-1);
            g_textCount++;
            printf("→ [%d] '%s' (Unicode)\n", i, buffer);
        } else {
            // Zkus ANSI verzi
            len = SendMessageA(targetListBox, LB_GETTEXT, i, (LPARAM)buffer);
            if (len > 0 && strlen(buffer) > 0) {
                strncpy(g_capturedTexts[g_textCount], buffer, sizeof(g_capturedTexts[0])-1);
                g_textCount++;
                printf("→ [%d] '%s' (ANSI)\n", i, buffer);
            } else {
                // Pokud standardní způsob nefunguje, zkus memory reading
                DWORD processId;
            GetWindowThreadProcessId(targetListBox, &processId);
            HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId);
            
            if (hProcess) {
                LRESULT itemData = SendMessage(targetListBox, LB_GETITEMDATA, i, 0);
                if (itemData != LB_ERR && itemData != 0) {
                    // Zkus číst text z offset 20 (jak jsme objevili dříve)
                    DWORD* ptr = (DWORD*)itemData;
                    DWORD textPtr;
                    SIZE_T bytesRead;
                    
                    if (ReadProcessMemory(hProcess, (char*)ptr + 20, &textPtr, sizeof(DWORD), &bytesRead)) {
                        if (bytesRead == sizeof(DWORD) && textPtr > 0x1000) {
                            char textBuffer[256] = {0};
                            if (ReadProcessMemory(hProcess, (void*)textPtr, textBuffer, sizeof(textBuffer)-1, &bytesRead)) {
                                if (bytesRead > 0 && strlen(textBuffer) > 0) {
                                    strncpy(g_capturedTexts[g_textCount], textBuffer, sizeof(g_capturedTexts[0])-1);
                                    g_textCount++;
                                    printf("→ [%d] '%s' (memory)\n", i, textBuffer);
                                }
                            }
                        }
                    }
                }
                CloseHandle(hProcess);
            }
        }
    }
    
    printf("→ ✓ Získáno %d textů\n", g_textCount);
}

// Odstranění hook
void UninstallDrawItemHook() {
    if (g_hHook) {
        UnhookWindowsHookEx(g_hHook);
        g_hHook = NULL;
        printf("✓ Hook odstraněn\n");
    }
}


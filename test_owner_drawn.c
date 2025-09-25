#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <commctrl.h>

char targetUnit[256] = {0};
HWND dockedListBox = NULL;

// Callback pro hledání TwinCAT okna a identifikaci dokovaného panelu
BOOL CALLBACK FindDockedPanel(HWND hwnd, LPARAM lParam) {
    char title[1024];
    GetWindowText(hwnd, title, sizeof(title));
    
    if (strstr(title, "TwinCAT PLC Control") != NULL && strstr(title, "[") != NULL) {
        printf("=== ANALÝZA DOKOVANÉHO PANELU ===\n");
        printf("Titulek: %s\n", title);
        
        // Extrahuj cílovou jednotku
        char *start = strchr(title, '[');
        char *end = strchr(title, ']');
        if (start && end && end > start) {
            strncpy(targetUnit, start + 1, end - start - 1);
            targetUnit[end - start - 1] = '\0';
            printf("Hledáme jednotku: '%s'\n", targetUnit);
        }
        
        // Najdi dokovaný ListBox (ten co má rozměry 590x1047)
        HWND hChild = GetWindow(hwnd, GW_CHILD);
        while (hChild) {
            char className[256];
            RECT rect;
            GetClassName(hChild, className, sizeof(className));
            GetWindowRect(hChild, &rect);
            
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;
            
            // Hledáme dokovaný panel vlevo
            if (strcmp(className, "ListBox") == 0 && width > 500 && width < 700 && height > 1000) {
                dockedListBox = hChild;
                printf("\nNalezen dokovaný ListBox: %08p [%dx%d]\n", hChild, width, height);
                
                // Test různých způsobů čtení dat
                printf("\n=== TESTOVÁNÍ RŮZNÝCH ZPŮSOBŮ ČTENÍ ===\n");
                
                int itemCount = SendMessage(hChild, LB_GETCOUNT, 0, 0);
                printf("Počet položek: %d\n", itemCount);
                
                // Test 1: Klasické LB_GETTEXT
                printf("\n1. Test LB_GETTEXT (prvních 5 položek):\n");
                for (int i = 0; i < min(itemCount, 5); i++) {
                    char buffer[1024];
                    int len = SendMessage(hChild, LB_GETTEXT, i, (LPARAM)buffer);
                    printf("   [%d] Délka: %d, Data: ", i, len);
                    
                    if (len > 0) {
                        // Zobrazit prvních 20 bytů jako hex
                        for (int j = 0; j < min(len, 20); j++) {
                            printf("%02X ", (unsigned char)buffer[j]);
                        }
                        printf("\n");
                    } else {
                        printf("(prázdné)\n");
                    }
                }
                
                // Test 2: Zkusit získat text přes WM_GETTEXT
                printf("\n2. Test WM_GETTEXT na ListBox:\n");
                char windowText[1024];
                int textLen = GetWindowText(hChild, windowText, sizeof(windowText));
                printf("   Délka textu okna: %d\n", textLen);
                if (textLen > 0) {
                    printf("   Text: '%.50s%s'\n", windowText, textLen > 50 ? "..." : "");
                }
                
                // Test 3: Zkusit poslat WM_CHAR nebo jiné zprávy
                printf("\n3. Test owner-drawn vlastností:\n");
                DWORD style = GetWindowLong(hChild, GWL_STYLE);
                printf("   Window Style: 0x%08X\n", style);
                
                if (style & LBS_OWNERDRAWFIXED) {
                    printf("   -> LBS_OWNERDRAWFIXED: ANO\n");
                } else {
                    printf("   -> LBS_OWNERDRAWFIXED: NE\n");
                }
                
                if (style & LBS_OWNERDRAWVARIABLE) {
                    printf("   -> LBS_OWNERDRAWVARIABLE: ANO\n");
                } else {
                    printf("   -> LBS_OWNERDRAWVARIABLE: NE\n");
                }
                
                if (style & LBS_HASSTRINGS) {
                    printf("   -> LBS_HASSTRINGS: ANO\n");
                } else {
                    printf("   -> LBS_HASSTRINGS: NE (používá jen data/ukazatele)\n");
                }
                
                // Test 4: Zkusit simulovat klik na položku
                printf("\n4. Test simulace kliku na první položku:\n");
                SendMessage(hChild, LB_SETCURSEL, 0, 0);
                int selectedIndex = SendMessage(hChild, LB_GETCURSEL, 0, 0);
                printf("   Vybraná položka: %d\n", selectedIndex);
                
                // Test 5: NOVÁ POKROČILÁ ANALÝZA PAMĚTI
                printf("\n5. === POKROČILÁ ANALÝZA PAMĚTI ===\n");
                
                // Získej process handle pro čtení paměti
                DWORD processId = 0;
                GetWindowThreadProcessId(hwnd, &processId);
                HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId);
                
                if (hProcess) {
                    printf("   Process Handle získán (PID: %d)\n", processId);
                    
                    // Analyzuj prvních 3 pointery detailně
                    for (int i = 0; i < min(itemCount, 3); i++) {
                        DWORD pointer = 0;
                        int len = SendMessage(hChild, LB_GETTEXT, i, (LPARAM)&pointer);
                        
                        if (len == 4) {
                            printf("\n   [%d] ANALÝZA POINTERU: 0x%08X\n", i, pointer);
                            
                            // Čtení raw dat z adresy
                            unsigned char rawData[128];
                            SIZE_T bytesRead = 0;
                            
                            if (ReadProcessMemory(hProcess, (LPCVOID)pointer, rawData, sizeof(rawData), &bytesRead)) {
                                printf("      Raw data (%zu bytů):\n", bytesRead);
                                
                                // Hex dump prvních 64 bytů
                                for (int row = 0; row < min(64, bytesRead); row += 16) {
                                    printf("      %04X: ", row);
                                    
                                    // Hex hodnoty
                                    for (int col = 0; col < 16 && (row + col) < min(64, bytesRead); col++) {
                                        printf("%02X ", rawData[row + col]);
                                    }
                                    
                                    // Zarovnání pro ASCII část
                                    for (int col = row + 16 - min(16, min(64, bytesRead) - row); col < 16; col++) {
                                        printf("   ");
                                    }
                                    
                                    printf(" | ");
                                    
                                    // ASCII reprezentace
                                    for (int col = 0; col < 16 && (row + col) < min(64, bytesRead); col++) {
                                        char c = rawData[row + col];
                                        printf("%c", (c >= 32 && c <= 126) ? c : '.');
                                    }
                                    printf("\n");
                                }
                                
                                // Hledat textové řetězce uvnitř struktury
                                printf("      Hledání textových řetězců ve struktuře:\n");
                                
                                for (int offset = 0; offset < min(64, bytesRead) - 4; offset += 4) {
                                    DWORD possiblePtr = *(DWORD*)(rawData + offset);
                                    
                                    // Zkontrolovat, zda vypadá jako validní pointer
                                    if (possiblePtr > 0x400000 && possiblePtr < 0x7FFFFFFF) {
                                        char textBuffer[128];
                                        SIZE_T textBytesRead = 0;
                                        
                                        if (ReadProcessMemory(hProcess, (LPCVOID)possiblePtr, textBuffer, sizeof(textBuffer) - 1, &textBytesRead)) {
                                            textBuffer[min(sizeof(textBuffer) - 1, textBytesRead)] = 0;
                                            
                                            // Zkontrolovat validitu textu
                                            int validChars = 0;
                                            int totalChars = 0;
                                            for (int k = 0; k < min(40, textBytesRead) && textBuffer[k] != 0; k++) {
                                                totalChars++;
                                                if (textBuffer[k] >= 32 && textBuffer[k] <= 126) {
                                                    validChars++;
                                                } else if (textBuffer[k] == 0) {
                                                    break;
                                                } else {
                                                    break; // Nevalidní znak
                                                }
                                            }
                                            
                                            if (validChars > 4 && validChars == totalChars) {
                                                printf("      +%02d -> 0x%08X: \"%s\"\n", offset, possiblePtr, textBuffer);
                                                
                                                // Kontrola cílové jednotky
                                                if (strlen(targetUnit) > 0 && strstr(textBuffer, targetUnit)) {
                                                    printf("      *** NALEZENA CÍLOVÁ JEDNOTKA na offsetu +%d! ***\n", offset);
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                printf("      CHYBA čtení paměti: %d\n", GetLastError());
                            }
                        }
                    }
                    
                    CloseHandle(hProcess);
                } else {
                    printf("   CHYBA: Nelze získat process handle! Chyba: %d\n", GetLastError());
                }
                
                break;
            }
            
            hChild = GetWindow(hChild, GW_HWNDNEXT);
        }
        
        // Pokud jsme nenašli dokovaný ListBox, zkusme hledat TreeView
        if (dockedListBox == NULL) {
            printf("\nDokovaný ListBox nenalezen. Hledáme TreeView...\n");
            
            hChild = GetWindow(hwnd, GW_CHILD);
            while (hChild) {
                char className[256];
                GetClassName(hChild, className, sizeof(className));
                
                if (strstr(className, "Tree") != NULL) {
                    RECT rect;
                    GetWindowRect(hChild, &rect);
                    
                    printf("TreeView nalezen: %08p [%dx%d] Class: '%s'\n", 
                           hChild, rect.right - rect.left, rect.bottom - rect.top, className);
                }
                
                hChild = GetWindow(hChild, GW_HWNDNEXT);
            }
        }
        
        return FALSE; // Zastavit hledání
    }
    
    return TRUE;
}

int main() {
    printf("=== ANALÝZA OWNER-DRAWN LISTBOX ===\n");
    printf("Testujeme různé způsoby čtení dat z dokovaného panelu.\n\n");
    
    EnumWindows(FindDockedPanel, 0);
    
    if (dockedListBox == NULL) {
        printf("Dokovaný panel nebyl nalezen!\n");
    }
    
    printf("\nAnalýza dokončena. Stiskněte Enter pro ukončení...\n");
    getchar();
    return 0;
}
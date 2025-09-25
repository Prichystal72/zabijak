#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <commctrl.h>

char targetUnit[256] = {0};
HWND dockedListBox = NULL;
DWORD processId = 0;
HANDLE hProcess = NULL;

// Funkce pro bezpečné čtení z paměti procesu
BOOL ReadProcessMemorySafe(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize) {
    SIZE_T bytesRead = 0;
    BOOL result = ReadProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, &bytesRead);
    
    if (!result) {
        DWORD error = GetLastError();
        printf("      Chyba čtení paměti: %d (0x%X)\n", error, error);
        return FALSE;
    }
    
    if (bytesRead != nSize) {
        printf("      Přečteno jen %zu z %zu bytů\n", bytesRead, nSize);
    }
    
    return TRUE;
}

// Funkce pro pokus o čtení textového řetězce z pointeru
void TryReadStringFromPointer(DWORD_PTR pointer, int itemIndex) {
    printf("  [%d] Pointer: 0x%08X\n", itemIndex, (DWORD)pointer);
    
    // Detailní analýza struktury v paměti
    unsigned char rawData[128];
    memset(rawData, 0, sizeof(rawData));
    
    if (ReadProcessMemorySafe(hProcess, (LPCVOID)pointer, rawData, sizeof(rawData))) {
        printf("      Raw data: ");
        for (int i = 0; i < 32; i++) {
            printf("%02X ", rawData[i]);
        }
        printf("\n");
        
        // Zobrazit jako ASCII
        printf("      ASCII:    ");
        for (int i = 0; i < 32; i++) {
            printf("%c  ", (rawData[i] >= 32 && rawData[i] <= 126) ? rawData[i] : '.');
        }
        printf("\n");
    }
    
    // Test 1: Přímé čtení jako řetězce
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    
    if (ReadProcessMemorySafe(hProcess, (LPCVOID)pointer, buffer, sizeof(buffer) - 1)) {
        printf("      Přímé čtení: ");
        
        // Zkontrolovat, zda vypadá jako text
        BOOL isText = TRUE;
        int printableChars = 0;
        for (int i = 0; i < 50 && buffer[i] != 0; i++) {
            if (buffer[i] >= 32 && buffer[i] <= 126) {
                printableChars++;
            } else if (buffer[i] == 0) {
                break;
            }
        }
        
        if (printableChars > 3) {
            printf("'%.50s%s'\n", buffer, strlen(buffer) > 50 ? "..." : "");
            
            // Zkontrolovat, zda obsahuje naši cílovou jednotku
            if (strlen(targetUnit) > 0 && strstr(buffer, targetUnit) != NULL) {
                printf("      *** NALEZENA CÍLOVÁ JEDNOTKA! ***\n");
            }
        } else {
            printf("(nejspíš ne text)\n");
        }
    }
    
    // Test 2: Pointer na pointer (double indirection)
    DWORD_PTR secondPointer = 0;
    if (ReadProcessMemorySafe(hProcess, (LPCVOID)pointer, &secondPointer, sizeof(secondPointer))) {
        if (secondPointer != 0 && secondPointer > 0x1000 && secondPointer < 0x7FFFFFFF) {
            printf("      Druhý pointer: 0x%08X\n", (DWORD)secondPointer);
            
            memset(buffer, 0, sizeof(buffer));
            if (ReadProcessMemorySafe(hProcess, (LPCVOID)secondPointer, buffer, sizeof(buffer) - 1)) {
                int printableChars = 0;
                for (int i = 0; i < 50 && buffer[i] != 0; i++) {
                    if (buffer[i] >= 32 && buffer[i] <= 126) {
                        printableChars++;
                    } else if (buffer[i] == 0) {
                        break;
                    }
                }
                
                if (printableChars > 3) {
                    printf("      Nepřímé čtení: '%.50s%s'\n", buffer, strlen(buffer) > 50 ? "..." : "");
                    
                    if (strlen(targetUnit) > 0 && strstr(buffer, targetUnit) != NULL) {
                        printf("      *** NALEZENA CÍLOVÁ JEDNOTKA (nepřímé)! ***\n");
                    }
                }
            }
        }
    }
    
    // Test 3: Zkusit číst jako strukturu s různými offsety
    for (int offset = 0; offset < 64; offset += 4) {
        DWORD_PTR testPointer = 0;
        if (ReadProcessMemorySafe(hProcess, (LPCVOID)(pointer + offset), &testPointer, sizeof(testPointer))) {
            if (testPointer > 0x400000 && testPointer < 0x7FFFFFFF) {
                memset(buffer, 0, sizeof(buffer));
                if (ReadProcessMemorySafe(hProcess, (LPCVOID)testPointer, buffer, 100)) {
                    int printableChars = 0;
                    int totalChars = 0;
                    for (int i = 0; i < 60 && buffer[i] != 0; i++) {
                        totalChars++;
                        if (buffer[i] >= 32 && buffer[i] <= 126) {
                            printableChars++;
                        } else if (buffer[i] == 0) {
                            break;
                        } else {
                            break; // Nevalidní znak
                        }
                    }
                    
                    if (printableChars > 5 && printableChars == totalChars) {
                        printf("      Offset +%02d (0x%08X): '", offset, (DWORD)testPointer);
                        for (int i = 0; i < min(40, printableChars); i++) {
                            printf("%c", buffer[i]);
                        }
                        if (printableChars > 40) printf("...");
                        printf("'\n");
                        
                        if (strlen(targetUnit) > 0 && strstr(buffer, targetUnit) != NULL) {
                            printf("      *** NALEZENA CÍLOVÁ JEDNOTKA (offset +%d)! ***\n", offset);
                        }
                    }
                }
            }
        }
    }
    
    // Test 4: Zkusit číst Unicode text
    wchar_t wideBuffer[128];
    memset(wideBuffer, 0, sizeof(wideBuffer));
    if (ReadProcessMemorySafe(hProcess, (LPCVOID)pointer, wideBuffer, sizeof(wideBuffer) - sizeof(wchar_t))) {
        int validWideChars = 0;
        for (int i = 0; i < 60 && wideBuffer[i] != 0; i++) {
            if (wideBuffer[i] >= 32 && wideBuffer[i] <= 126) {
                validWideChars++;
            } else if (wideBuffer[i] == 0) {
                break;
            } else {
                break;
            }
        }
        if (validWideChars > 3) {
            printf("      Unicode text: '");
            for (int i = 0; i < min(30, validWideChars); i++) {
                printf("%c", (char)wideBuffer[i]);
            }
            if (validWideChars > 30) printf("...");
            printf("'\n");
            
            // Převést na ASCII pro porovnání
            char asciiBuffer[128];
            memset(asciiBuffer, 0, sizeof(asciiBuffer));
            WideCharToMultiByte(CP_ACP, 0, wideBuffer, -1, asciiBuffer, sizeof(asciiBuffer) - 1, NULL, NULL);
            if (strlen(targetUnit) > 0 && strstr(asciiBuffer, targetUnit) != NULL) {
                printf("      *** NALEZENA CÍLOVÁ JEDNOTKA (Unicode)! ***\n");
            }
        }
    }
    
    printf("\n");
}

// Callback pro hledání TwinCAT okna
BOOL CALLBACK FindTwinCatWindow(HWND hwnd, LPARAM lParam) {
    char title[1024];
    GetWindowText(hwnd, title, sizeof(title));
    
    if (strstr(title, "TwinCAT PLC Control") != NULL && strstr(title, "[") != NULL) {
        printf("=== ČTENÍ PAMĚTI Z POINTERŮ ===\n");
        printf("Titulek: %s\n", title);
        
        // Extrahuj cílovou jednotku
        char *start = strchr(title, '[');
        char *end = strchr(title, ']');
        if (start && end && end > start) {
            strncpy(targetUnit, start + 1, end - start - 1);
            targetUnit[end - start - 1] = '\0';
            printf("Hledáme jednotku: '%s'\n", targetUnit);
        }
        
        // Získej process ID a handle
        GetWindowThreadProcessId(hwnd, &processId);
        hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId);
        
        if (hProcess == NULL) {
            printf("Nepodařilo se otevřít proces pro čtení! Chyba: %d\n", GetLastError());
            return FALSE;
        }
        
        printf("Process ID: %d, Handle: %p\n\n", processId, hProcess);
        
        // Najdi dokovaný ListBox
        HWND hChild = GetWindow(hwnd, GW_CHILD);
        while (hChild) {
            char className[256];
            RECT rect;
            GetClassName(hChild, className, sizeof(className));
            GetWindowRect(hChild, &rect);
            
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;
            
            if (strcmp(className, "ListBox") == 0 && width > 500 && width < 700 && height > 1000) {
                dockedListBox = hChild;
                printf("Dokovaný ListBox nalezen: %08p\n\n", hChild);
                
                int itemCount = SendMessage(hChild, LB_GETCOUNT, 0, 0);
                printf("Počet položek: %d\n\n", itemCount);
                
                // Číst první 10 položek z paměti
                printf("=== ČTENÍ POINTERŮ Z PAMĚTI ===\n");
                for (int i = 0; i < min(itemCount, 10); i++) {
                    // Získat pointer pro položku i
                    DWORD_PTR itemData = 0;
                    int len = SendMessage(hChild, LB_GETTEXT, i, (LPARAM)&itemData);
                    
                    if (len == 4) { // 4 byty = pointer
                        TryReadStringFromPointer(itemData, i);
                    }
                }
                
                break;
            }
            
            hChild = GetWindow(hChild, GW_HWNDNEXT);
        }
        
        return FALSE;
    }
    
    return TRUE;
}

int main() {
    printf("=== ANALÝZA POINTERŮ V PAMĚTI ===\n");
    printf("Pokusíme se přečíst skutečný obsah projektového stromu.\n\n");
    
    EnumWindows(FindTwinCatWindow, 0);
    
    if (hProcess) {
        CloseHandle(hProcess);
    }
    
    printf("Analýza dokončena. Stiskněte Enter pro ukončení...\n");
    getchar();
    return 0;
}
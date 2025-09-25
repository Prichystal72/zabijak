#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <commctrl.h>

char targetUnit[256] = {0};
DWORD processId = 0;
HANDLE hProcess = NULL;

// Pokročilá analýza paměti na dané adrese
void AnalyzeMemoryAddress(DWORD address, int index) {
    printf("\n[%d] === ANALÝZA ADRESY 0x%08X ===\n", index, address);
    
    // Přečíst raw data
    unsigned char rawData[256];
    SIZE_T bytesRead = 0;
    
    if (!ReadProcessMemory(hProcess, (LPCVOID)address, rawData, sizeof(rawData), &bytesRead)) {
        printf("   CHYBA čtení paměti: %d\n", GetLastError());
        return;
    }
    
    printf("   Přečteno %zu bytů\n", bytesRead);
    
    // Zobrazit hex dump prvních 64 bytů
    printf("   Hex dump:\n");
    for (int i = 0; i < min(64, bytesRead); i += 16) {
        printf("   %04X: ", i);
        
        // Hex hodnoty
        for (int j = 0; j < 16 && (i + j) < min(64, bytesRead); j++) {
            printf("%02X ", rawData[i + j]);
        }
        
        // Zarovnání
        for (int j = i + 16 - min(16, min(64, bytesRead) - i); j < 16; j++) {
            printf("   ");
        }
        
        printf(" | ");
        
        // ASCII reprezentace
        for (int j = 0; j < 16 && (i + j) < min(64, bytesRead); j++) {
            char c = rawData[i + j];
            printf("%c", (c >= 32 && c <= 126) ? c : '.');
        }
        printf("\n");
    }
    
    // Hledat textové řetězce ve struktuře
    printf("\n   Hledání textových řetězců:\n");
    
    for (int offset = 0; offset < min(128, bytesRead) - 4; offset += 4) {
        DWORD possiblePtr = *(DWORD*)(rawData + offset);
        
        // Zkontrolovat, zda vypadá jako validní pointer
        if (possiblePtr > 0x400000 && possiblePtr < 0x7FFFFFFF) {
            char textBuffer[200];
            SIZE_T textBytesRead = 0;
            
            if (ReadProcessMemory(hProcess, (LPCVOID)possiblePtr, textBuffer, sizeof(textBuffer) - 1, &textBytesRead)) {
                textBuffer[min(sizeof(textBuffer) - 1, textBytesRead)] = 0;
                
                // Zkontrolovat validitu textu
                int validChars = 0;
                int totalChars = 0;
                for (int i = 0; i < min(60, textBytesRead) && textBuffer[i] != 0; i++) {
                    totalChars++;
                    if (textBuffer[i] >= 32 && textBuffer[i] <= 126) {
                        validChars++;
                    } else if (textBuffer[i] == 0) {
                        break;
                    } else {
                        break; // Nevalidní znak
                    }
                }
                
                if (validChars > 4 && validChars == totalChars) {
                    printf("   +%03d -> 0x%08X: \"%s\"\n", offset, possiblePtr, textBuffer);
                    
                    // Kontrola cílové jednotky
                    if (strlen(targetUnit) > 0 && strstr(textBuffer, targetUnit)) {
                        printf("   *** NALEZENA CÍLOVÁ JEDNOTKA na offsetu +%d! ***\n", offset);
                    }
                }
            }
        }
    }
    
    // Zkusit číst jako přímý text z různých offsetů
    printf("\n   Přímé čtení textu:\n");
    for (int offset = 0; offset < min(64, bytesRead) - 10; offset += 4) {
        char directText[100];
        memcpy(directText, rawData + offset, sizeof(directText) - 1);
        directText[sizeof(directText) - 1] = 0;
        
        int validChars = 0;
        int totalChars = 0;
        for (int i = 0; i < 40 && directText[i] != 0; i++) {
            totalChars++;
            if (directText[i] >= 32 && directText[i] <= 126) {
                validChars++;
            } else if (directText[i] == 0) {
                break;
            } else {
                break;
            }
        }
        
        if (validChars > 6 && validChars == totalChars) {
            printf("   +%03d (direct): \"%s\"\n", offset, directText);
            
            if (strlen(targetUnit) > 0 && strstr(directText, targetUnit)) {
                printf("   *** NALEZENA CÍLOVÁ JEDNOTKA (přímé čtení) na offsetu +%d! ***\n", offset);
            }
        }
    }
    
    // Zkusit Unicode
    printf("\n   Unicode test:\n");
    wchar_t* widePtr = (wchar_t*)rawData;
    int wideLen = min(32, bytesRead / 2);
    
    for (int offset = 0; offset < 32; offset += 2) {
        if (offset + 10 < wideLen) {
            int validWideChars = 0;
            for (int i = 0; i < 20 && widePtr[offset/2 + i] != 0; i++) {
                if (widePtr[offset/2 + i] >= 32 && widePtr[offset/2 + i] <= 126) {
                    validWideChars++;
                } else if (widePtr[offset/2 + i] == 0) {
                    break;
                } else {
                    break;
                }
            }
            
            if (validWideChars > 4) {
                printf("   +%03d (Unicode): \"", offset);
                for (int i = 0; i < validWideChars && i < 30; i++) {
                    printf("%c", (char)widePtr[offset/2 + i]);
                }
                printf("\"\n");
            }
        }
    }
}

BOOL CALLBACK FindTwinCatWindow(HWND hwnd, LPARAM lParam) {
    char title[1024];
    GetWindowText(hwnd, title, sizeof(title));
    
    if (strstr(title, "TwinCAT PLC Control") && strstr(title, "[") && strstr(title, "]")) {
        printf("=== DETAILNÍ ANALYZÁTOR PAMĚTI TWINCAT ===\n");
        printf("Okno: %s\n", title);
        
        // Získej cílovou jednotku
        char *start = strchr(title, '[');
        char *end = strchr(title, ']');
        if (start && end && end > start) {
            strncpy(targetUnit, start + 1, end - start - 1);
            targetUnit[end - start - 1] = '\0';
            printf("Hledáme: '%s'\n", targetUnit);
        }
        
        // Získej process handle
        GetWindowThreadProcessId(hwnd, &processId);
        hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId);
        
        if (!hProcess) {
            printf("CHYBA: Nelze otevřít proces!\n");
            return FALSE;
        }
        
        printf("Process ID: %d\n", processId);
        
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
                printf("\nListBox nalezen: %08p [%dx%d]\n", hChild, width, height);
                
                int itemCount = SendMessage(hChild, LB_GETCOUNT, 0, 0);
                printf("Počet položek: %d\n", itemCount);
                
                // Analyzuj prvních 5 položek detailně
                int maxItems = min(itemCount, 5);
                for (int i = 0; i < maxItems; i++) {
                    DWORD pointer = 0;
                    int len = SendMessage(hChild, LB_GETTEXT, i, (LPARAM)&pointer);
                    
                    if (len == 4) {
                        AnalyzeMemoryAddress(pointer, i);
                        if (i < maxItems - 1) {
                            printf("\n%s\n", "══════════════════════════════════════════════════════════");
                        }
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
    printf("=== DETAILNÍ ANALYZÁTOR PAMĚTI TWINCAT ===\n");
    printf("Pokročilá analýza struktury dat v owner-drawn ListBoxu...\n\n");
    
    EnumWindows(FindTwinCatWindow, 0);
    
    if (hProcess) {
        CloseHandle(hProcess);
    }
    
    printf("\n\nAnalýza dokončena. Stiskněte Enter...\n");
    getchar();
    return 0;
}
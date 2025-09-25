#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <commctrl.h>

char targetUnit[256] = {0};
DWORD twinCatProcessId = 0;
HANDLE hTwinCatProcess = NULL;
HWND dockedListBox = NULL;

// Struktura pro uložení pointer dat
typedef struct {
    DWORD address;
    int index;
    char description[128];
} PointerInfo;

PointerInfo pointers[100];
int pointerCount = 0;

// Bezpečné čtení z paměti procesu
BOOL ReadProcessMemorySafe(HANDLE hProcess, LPCVOID address, LPVOID buffer, SIZE_T size) {
    if (!hProcess || !address) return FALSE;
    
    SIZE_T bytesRead = 0;
    BOOL result = ReadProcessMemory(hProcess, address, buffer, size, &bytesRead);
    
    if (!result || bytesRead != size) {
        return FALSE;
    }
    
    return TRUE;
}

// Pokus o čtení textového řetězce z adresy
BOOL ReadStringFromAddress(DWORD address, char* buffer, int maxLen) {
    if (!hTwinCatProcess) return FALSE;
    
    // Vynulovat buffer
    memset(buffer, 0, maxLen);
    
    // Zkusit číst přímo jako ASCII řetězec
    if (ReadProcessMemorySafe(hTwinCatProcess, (LPCVOID)address, buffer, maxLen - 1)) {
        // Zkontrolovat, zda vypadá jako validní řetězec
        int validChars = 0;
        for (int i = 0; i < maxLen - 1 && buffer[i] != 0; i++) {
            if ((buffer[i] >= 32 && buffer[i] <= 126) || buffer[i] == 0) {
                validChars++;
            } else {
                break;
            }
        }
        
        if (validChars > 3) { // Alespoň 3 validní znaky
            return TRUE;
        }
    }
    
    // Zkusit číst jako pointer na řetězec (double indirection)
    DWORD secondaryAddress;
    if (ReadProcessMemorySafe(hTwinCatProcess, (LPCVOID)address, &secondaryAddress, sizeof(DWORD))) {
        if (secondaryAddress != 0 && secondaryAddress != address) { // Vyhnout se nekonečné smyčce
            if (ReadProcessMemorySafe(hTwinCatProcess, (LPCVOID)secondaryAddress, buffer, maxLen - 1)) {
                int validChars = 0;
                for (int i = 0; i < maxLen - 1 && buffer[i] != 0; i++) {
                    if ((buffer[i] >= 32 && buffer[i] <= 126) || buffer[i] == 0) {
                        validChars++;
                    } else {
                        break;
                    }
                }
                
                if (validChars > 3) {
                    return TRUE;
                }
            }
        }
    }
    
    return FALSE;
}

// Pokus o čtení struktury s různými offsety
void AnalyzeStructureAtAddress(DWORD address, int index) {
    printf("\n[%d] Analyzuji adresu: 0x%08X\n", index, address);
    
    // Přečíst prvních 128 bytů jako raw data
    unsigned char rawData[128];
    if (ReadProcessMemorySafe(hTwinCatProcess, (LPCVOID)address, rawData, sizeof(rawData))) {
        printf("   Raw data (prvních 64 bytů):\n");
        for (int i = 0; i < 64; i += 16) {
            printf("   %04X: ", i);
            for (int j = 0; j < 16 && (i + j) < 64; j++) {
                printf("%02X ", rawData[i + j]);
            }
            printf(" | ");
            for (int j = 0; j < 16 && (i + j) < 64; j++) {
                char c = rawData[i + j];
                printf("%c", (c >= 32 && c <= 126) ? c : '.');
            }
            printf("\n");
        }
        
        // Zkusit různé offsety pro textové řetězce (pointery)
        printf("\n   Hledání pointerů na text:\n");
        for (int offset = 0; offset < 64; offset += 4) {
            DWORD possiblePointer = *(DWORD*)(rawData + offset);
            
            // Zkontrolovat, zda vypadá jako validní pointer
            if (possiblePointer > 0x400000 && possiblePointer < 0x7FFFFFFF) {
                char textBuffer[256];
                if (ReadStringFromAddress(possiblePointer, textBuffer, sizeof(textBuffer))) {
                    printf("   +%02d -> 0x%08X: '%s'\n", offset, possiblePointer, textBuffer);
                    
                    // Zkontrolovat, zda obsahuje naši cílovou jednotku
                    if (strlen(targetUnit) > 0 && strstr(textBuffer, targetUnit) != NULL) {
                        printf("   *** NALEZENA CÍLOVÁ JEDNOTKA! ***\n");
                        sprintf(pointers[index].description, "CÍLEK! +%d -> %s", offset, textBuffer);
                    }
                }
            }
        }
        
        // Zkusit přímé čtení textu z různých offsetů
        printf("\n   Hledání přímého textu:\n");
        for (int offset = 0; offset < 64; offset += 4) {
            char directText[128];
            if (ReadProcessMemorySafe(hTwinCatProcess, (LPCVOID)(address + offset), directText, sizeof(directText) - 1)) {
                directText[127] = 0;
                
                // Zkontrolovat validní ASCII text
                int validChars = 0;
                int totalChars = 0;
                for (int i = 0; i < 127 && directText[i] != 0; i++) {
                    totalChars++;
                    if ((directText[i] >= 32 && directText[i] <= 126)) {
                        validChars++;
                    } else if (directText[i] == 0) {
                        break;
                    } else {
                        break; // Nevalidní znak
                    }
                }
                
                if (validChars > 5 && validChars == totalChars) { // Celý řetězec musí být validní
                    printf("   +%02d (direct): '%s'\n", offset, directText);
                    
                    if (strlen(targetUnit) > 0 && strstr(directText, targetUnit) != NULL) {
                        printf("   *** NALEZENA CÍLOVÁ JEDNOTKA (přímé čtení)! ***\n");
                        sprintf(pointers[index].description, "CÍLEK DIRECT! +%d -> %s", offset, directText);
                    }
                }
            }
        }
    } else {
        printf("   CHYBA: Nelze číst z adresy 0x%08X\n", address);
    }
}

// Callback pro nalezení TwinCAT okna a analýzu pointerů
BOOL CALLBACK AnalyzePointers(HWND hwnd, LPARAM lParam) {
    char title[1024];
    GetWindowText(hwnd, title, sizeof(title));
    
    if (strstr(title, "TwinCAT PLC Control") != NULL && strstr(title, "[") != NULL) {
        printf("=== POKROČILÁ ANALÝZA PAMĚTI PROCESU ===\n");
        printf("Okno: %s\n", title);
        
        // Extrahuj cílovou jednotku
        char *start = strchr(title, '[');
        char *end = strchr(title, ']');
        if (start && end && end > start) {
            strncpy(targetUnit, start + 1, end - start - 1);
            targetUnit[end - start - 1] = '\0';
            printf("Hledáme: '%s'\n", targetUnit);
        }
        
        // Získej process ID a handle
        GetWindowThreadProcessId(hwnd, &twinCatProcessId);
        hTwinCatProcess = OpenProcess(PROCESS_VM_READ, FALSE, twinCatProcessId);
        
        if (!hTwinCatProcess) {
            printf("CHYBA: Nelze otevřít proces pro čtení paměti!\n");
            return FALSE;
        }
        
        printf("Process ID: %d, Handle: %p\n", twinCatProcessId, hTwinCatProcess);
        
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
                printf("\nDokovaný ListBox: %08p [%dx%d]\n", hChild, width, height);
                
                int itemCount = SendMessage(hChild, LB_GETCOUNT, 0, 0);
                printf("Počet položek: %d\n", itemCount);
                
                // Získat pointery z prvních 15 položek
                pointerCount = min(itemCount, 15);
                printf("\n=== ZÍSKÁVÁNÍ POINTERŮ (prvních %d položek) ===\n", pointerCount);
                
                for (int i = 0; i < pointerCount; i++) {
                    DWORD pointer = 0;
                    int len = SendMessage(hChild, LB_GETTEXT, i, (LPARAM)&pointer);
                    
                    if (len == 4) { // 4 byty = DWORD pointer
                        pointers[i].address = pointer;
                        pointers[i].index = i;
                        strcpy(pointers[i].description, "");
                        printf("Položka [%d]: 0x%08X\n", i, pointer);
                    }
                }
                
                // Analyzovat každý pointer
                printf("\n=== DETAILNÍ ANALÝZA OBSAHU PAMĚTI ===\n");
                for (int i = 0; i < pointerCount; i++) {
                    AnalyzeStructureAtAddress(pointers[i].address, i);
                    if (i < pointerCount - 1) {
                        printf("\n" "════════════════════════════════════════════════════════════════\n");
                    }
                }
                
                // Shrnutí nalezených údajů
                printf("\n=== SHRNUTÍ VÝSLEDKŮ ===\n");
                for (int i = 0; i < pointerCount; i++) {
                    if (strlen(pointers[i].description) > 0) {
                        printf("[%d] 0x%08X: %s\n", i, pointers[i].address, pointers[i].description);
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
    printf("=== POKROČILÝ ANALYZÁTOR PAMĚTI TWINCAT PROCESU ===\n");
    printf("Detailní analýza dat z pointerů v owner-drawn ListBoxu...\n\n");
    
    EnumWindows(AnalyzePointers, 0);
    
    if (hTwinCatProcess) {
        CloseHandle(hTwinCatProcess);
    }
    
    printf("\nAnalýza dokončena. Stiskněte Enter pro ukončení...\n");
    getchar();
    return 0;
}
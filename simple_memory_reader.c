#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <commctrl.h>

char targetUnit[256] = {0};
DWORD twinCatProcessId = 0;
HANDLE hTwinCatProcess = NULL;

// Bezpečné čtení z paměti procesu
BOOL ReadProcessMemorySafe(HANDLE hProcess, LPCVOID address, LPVOID buffer, SIZE_T size) {
    if (!hProcess || !address) return FALSE;
    
    SIZE_T bytesRead = 0;
    BOOL result = ReadProcessMemory(hProcess, address, buffer, size, &bytesRead);
    return (result && bytesRead == size);
}

// Pokus o čtení různých formátů z adresy
void AnalyzeAddress(DWORD address, int index) {
    printf("\n[%d] Adresa: 0x%08X\n", index, address);
    
    // 1. Zkusit číst přímý text
    char directText[128];
    memset(directText, 0, sizeof(directText));
    if (ReadProcessMemorySafe(hTwinCatProcess, (LPCVOID)address, directText, sizeof(directText) - 1)) {
        printf("   Přímý text: ");
        int validChars = 0;
        for (int i = 0; i < 40 && directText[i] != 0; i++) {
            if (directText[i] >= 32 && directText[i] <= 126) {
                printf("%c", directText[i]);
                validChars++;
            } else if (directText[i] == 0) {
                break;
            } else {
                printf(".");
            }
        }
        if (validChars > 3) {
            printf(" (validní text)\n");
            if (strlen(targetUnit) > 0 && strstr(directText, targetUnit)) {
                printf("   *** NALEZENA CÍLOVÁ JEDNOTKA! ***\n");
            }
        } else {
            printf(" (není text)\n");
        }
    }
    
    // 2. Zkusit číst jako pointer na text
    DWORD pointer;
    if (ReadProcessMemorySafe(hTwinCatProcess, (LPCVOID)address, &pointer, sizeof(DWORD))) {
        if (pointer != 0 && pointer != address && pointer > 0x400000) {
            char indirectText[128];
            memset(indirectText, 0, sizeof(indirectText));
            if (ReadProcessMemorySafe(hTwinCatProcess, (LPCVOID)pointer, indirectText, sizeof(indirectText) - 1)) {
                printf("   Nepřímý text (0x%08X): ", pointer);
                int validChars = 0;
                for (int i = 0; i < 40 && indirectText[i] != 0; i++) {
                    if (indirectText[i] >= 32 && indirectText[i] <= 126) {
                        printf("%c", indirectText[i]);
                        validChars++;
                    } else if (indirectText[i] == 0) {
                        break;
                    } else {
                        printf(".");
                    }
                }
                if (validChars > 3) {
                    printf(" (validní text)\n");
                    if (strlen(targetUnit) > 0 && strstr(indirectText, targetUnit)) {
                        printf("   *** NALEZENA CÍLOVÁ JEDNOTKA! ***\n");
                    }
                } else {
                    printf(" (není text)\n");
                }
            }
        }
    }
    
    // 3. Zkusit číst jako strukturu s různými offsety
    unsigned char rawData[64];
    if (ReadProcessMemorySafe(hTwinCatProcess, (LPCVOID)address, rawData, sizeof(rawData))) {
        printf("   Raw data: ");
        for (int i = 0; i < 32; i++) {
            printf("%02X ", rawData[i]);
        }
        printf("\n");
        
        // Zkusit různé offsety jako pointery
        for (int offset = 0; offset < 32; offset += 4) {
            DWORD possiblePointer = *(DWORD*)(rawData + offset);
            if (possiblePointer > 0x400000 && possiblePointer < 0x7FFFFFFF) {
                char structText[128];
                memset(structText, 0, sizeof(structText));
                if (ReadProcessMemorySafe(hTwinCatProcess, (LPCVOID)possiblePointer, structText, sizeof(structText) - 1)) {
                    int validChars = 0;
                    for (int i = 0; i < 40 && structText[i] != 0; i++) {
                        if (structText[i] >= 32 && structText[i] <= 126) {
                            validChars++;
                        } else if (structText[i] == 0) {
                            break;
                        } else {
                            break;
                        }
                    }
                    if (validChars > 3) {
                        printf("   Offset +%d (0x%08X): '", offset, possiblePointer);
                        for (int i = 0; i < validChars && i < 40; i++) {
                            printf("%c", structText[i]);
                        }
                        printf("'\n");
                        if (strlen(targetUnit) > 0 && strstr(structText, targetUnit)) {
                            printf("   *** NALEZENA CÍLOVÁ JEDNOTKA na offsetu +%d! ***\n", offset);
                        }
                    }
                }
            }
        }
    }
}

BOOL CALLBACK FindTwinCatWindow(HWND hwnd, LPARAM lParam) {
    char title[1024];
    GetWindowText(hwnd, title, sizeof(title));
    
    if (strstr(title, "TwinCAT PLC Control") && strstr(title, "[") && strstr(title, "]")) {
        printf("=== JEDNODUCHÝ ANALYZÁTOR PAMĚTI ===\n");
        printf("Okno: %s\n", title);
        
        // Získej cílovou jednotku z titulku
        char *start = strchr(title, '[');
        char *end = strchr(title, ']');
        if (start && end && end > start) {
            strncpy(targetUnit, start + 1, end - start - 1);
            targetUnit[end - start - 1] = '\0';
            printf("Hledáme: '%s'\n", targetUnit);
        }
        
        // Získej process handle
        GetWindowThreadProcessId(hwnd, &twinCatProcessId);
        hTwinCatProcess = OpenProcess(PROCESS_VM_READ, FALSE, twinCatProcessId);
        
        if (!hTwinCatProcess) {
            printf("CHYBA: Nelze otevřít proces!\n");
            return FALSE;
        }
        
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
                
                // Analyzuj prvních 10 pointerů
                int maxItems = min(itemCount, 10);
                for (int i = 0; i < maxItems; i++) {
                    DWORD pointer = 0;
                    int len = SendMessage(hChild, LB_GETTEXT, i, (LPARAM)&pointer);
                    
                    if (len == 4) {
                        AnalyzeAddress(pointer, i);
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
    printf("=== JEDNODUCHÝ ANALYZÁTOR PAMĚTI TWINCAT ===\n");
    printf("Analyzuje data z pointerů v owner-drawn ListBoxu...\n\n");
    
    EnumWindows(FindTwinCatWindow, 0);
    
    if (hTwinCatProcess) {
        CloseHandle(hTwinCatProcess);
    }
    
    printf("\nAnalýza dokončena. Stiskněte Enter...\n");
    getchar();
    return 0;
}
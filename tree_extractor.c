#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Počet mezer pro odsazení podle úrovně
void printIndent(int level) {
    for (int i = 0; i < level; i++) {
        printf("  ");
    }
}

// Vyčistí text od neplatných znaků
void cleanText(char* text) {
    char* src = text;
    char* dst = text;
    
    while (*src) {
        if (*src >= 32 && *src <= 126) {  // Jen tisknutelné znaky
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}

int main() {
    printf("=== ÚPLNÝ VÝPIS STROMU PROJEKTU ===\n\n");
    
    // Najdi TwinCAT okno
    HWND hTwinCAT = NULL;
    HWND hWnd = GetTopWindow(GetDesktopWindow());
    
    while (hWnd) {
        char windowTitle[512];
        if (GetWindowText(hWnd, windowTitle, sizeof(windowTitle))) {
            if (strstr(windowTitle, "TwinCAT") != NULL) {
                hTwinCAT = hWnd;
                printf("Nalezeno TwinCAT okno: %s\n", windowTitle);
                break;
            }
        }
        hWnd = GetNextWindow(hWnd, GW_HWNDNEXT);
    }
    
    if (!hTwinCAT) {
        printf("TwinCAT okno nenalezeno!\n");
        return 1;
    }
    
    // ListBox handle
    HWND hListBox = (HWND)0x00070400;
    DWORD processId;
    GetWindowThreadProcessId(hListBox, &processId);
    
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) {
        printf("Nelze otevřít proces!\n");
        return 1;
    }
    
    // Počet položek
    int count = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
    printf("Celkem položek: %d\n\n", count);
    
    // Extrakce všech položek
    for (int item = 0; item < count; item++) {
        // Získej ItemData
        LRESULT itemData = SendMessage(hListBox, LB_GETITEMDATA, item, 0);
        if (itemData == LB_ERR || itemData == 0) {
            continue;
        }
        
        // Přečti strukturu ItemData
        unsigned char buffer[100];
        SIZE_T bytesRead;
        
        if (ReadProcessMemory(hProcess, (void*)itemData, buffer, sizeof(buffer), &bytesRead)) {
            if (bytesRead >= 24) {
                // Text pointer je na offsetu 20
                DWORD* textPtr = (DWORD*)(buffer + 20);
                DWORD textAddr = *textPtr;
                
                if (textAddr > 0x400000 && textAddr < 0x80000000) {
                    // Přečti text
                    char textBuffer[256] = {0};
                    SIZE_T textRead;
                    
                    if (ReadProcessMemory(hProcess, (void*)textAddr, textBuffer, sizeof(textBuffer)-1, &textRead)) {
                        // Text začína na pozici 1 (přeskočí null byte na začátku)
                        char* text = textBuffer + 1;
                        cleanText(text);
                        
                        if (strlen(text) > 0) {
                            // Analyzuj strukturu pro hierarchii
                            DWORD* dword0 = (DWORD*)(buffer + 0);   // D0 1E 62 01
                            DWORD* dword1 = (DWORD*)(buffer + 4);   // index položky  
                            DWORD* dword2 = (DWORD*)(buffer + 8);   // flags/stav
                            DWORD* dword3 = (DWORD*)(buffer + 12);  // možná level
                            DWORD* dword4 = (DWORD*)(buffer + 16);  // padding
                            
                            // Zkus různé způsoby určení levelu
                            int level1 = *dword3 & 0xFF;           // Spodní byte z dword3
                            int level2 = (*dword2 >> 8) & 0xFF;    // Druhý byte z flags
                            int level3 = item < 10 ? 0 : (item < 20 ? 1 : 2);  // Podle pozice
                            
                            printf("[%02d] 0x%08X|%08X|%08X|%08X -> ", 
                                   item, *dword0, *dword1, *dword2, *dword3);
                            
                            // Použij level1 pokud vypadá rozumně
                            int level = (level1 >= 0 && level1 <= 5) ? level1 : 0;
                            
                            printIndent(level);
                            printf("%s\n", text);
                        }
                    }
                }
            }
        }
    }
    
    CloseHandle(hProcess);
    
    printf("\n=== KONEC ===\n");
    printf("Stiskněte Enter...\n");
    getchar();
    return 0;
}
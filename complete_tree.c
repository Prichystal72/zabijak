#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

void printIndent(int level) {
    for (int i = 0; i < level; i++) {
        printf("  ");
    }
}

int main() {
    printf("=== KOMPLETNÍ STROM PROJEKTU TWINCAT ===\n\n");
    
    // TwinCAT okno
    HWND hTwinCAT = NULL;
    HWND hWnd = GetTopWindow(GetDesktopWindow());
    
    while (hWnd) {
        char windowTitle[512];
        if (GetWindowText(hWnd, windowTitle, sizeof(windowTitle))) {
            if (strstr(windowTitle, "TwinCAT") != NULL) {
                hTwinCAT = hWnd;
                printf("TwinCAT: %s\n\n", windowTitle);
                break;
            }
        }
        hWnd = GetNextWindow(hWnd, GW_HWNDNEXT);
    }
    
    if (!hTwinCAT) return 1;
    
    HWND hListBox = (HWND)0x00070400;
    DWORD processId;
    GetWindowThreadProcessId(hListBox, &processId);
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId);
    
    if (!hProcess) return 1;
    
    int count = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
    
    // Stack pro tracking hierarchie
    int levelStack[10];
    int stackPos = 0;
    int currentLevel = 0;
    
    for (int item = 0; item < count; item++) {
        LRESULT itemData = SendMessage(hListBox, LB_GETITEMDATA, item, 0);
        if (itemData == LB_ERR || itemData == 0) continue;
        
        DWORD structure[10];
        SIZE_T bytesRead;
        
        if (ReadProcessMemory(hProcess, (void*)itemData, structure, sizeof(structure), &bytesRead)) {
            if (bytesRead >= 24) {
                DWORD textPtr = structure[5];  // Text pointer na offset 20
                DWORD position = structure[1]; // Pozice v hierarchii
                DWORD flags = structure[2];    // Flags
                DWORD hasChildren = structure[3]; // Má-li podpoložky
                
                if (textPtr > 0x400000 && textPtr < 0x80000000) {
                    char textBuffer[100] = {0};
                    SIZE_T textRead;
                    
                    if (ReadProcessMemory(hProcess, (void*)textPtr, textBuffer, sizeof(textBuffer)-1, &textRead)) {
                        char* text = textBuffer + 1;  // Přeskoč null byte
                        
                        if (strlen(text) > 0) {
                            // Určení hierarchické úrovně podle pozice
                            int level = 0;
                            
                            // Root položky
                            if (position == 0) level = 0;
                            else if (position == 1) level = 1;
                            else if (position == 2) level = 1;
                            else if (position == 3) level = 1;
                            else if (position >= 4) level = 2;
                            
                            // Podle flags
                            if (flags == 0x3604FD) {
                                // Rozbalitelné složky - obvykle vyšší level
                                if (position > 0) level = 1;
                            } else if (flags == 0x704ED) {
                                // Soubory - obvykle nižší level  
                                level = 2;
                            }
                            
                            // Speciální případy podle textu
                            if (strcmp(text, "POUs") == 0) level = 0;
                            else if (strcmp(text, "pBasic") == 0) level = 1;
                            else if (strcmp(text, "PLC_PRG") == 0) level = 2;
                            else if (strstr(text, "ST00") != NULL && hasChildren == 1) level = 1;
                            else if (strstr(text, "ST00") != NULL && hasChildren == 0) level = 2;
                            
                            // Zobraz s odsazením
                            printIndent(level);
                            
                            // Ikona podle typu
                            if (hasChildren == 1) {
                                printf("📁 %s\n", text);
                            } else {
                                if (flags == 0x704ED) {
                                    printf("📄 %s\n", text);
                                } else {
                                    printf("⚙️ %s\n", text);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    CloseHandle(hProcess);
    
    printf("\n=== CELKEM %d POLOŽEK ===\n", count);
    printf("Stiskněte Enter...\n");  
    getchar();
    return 0;
}
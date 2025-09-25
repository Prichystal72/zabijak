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
    printf("=== SEZNAM VŠECH POLOŽEK PRO FOCUS ===\n\n");
    
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
    printf("Celkem položek: %d\n\n", count);
    
    for (int item = 0; item < count; item++) {
        LRESULT itemData = SendMessage(hListBox, LB_GETITEMDATA, item, 0);
        if (itemData == LB_ERR || itemData == 0) {
            printf("[%02d] <bez ItemData>\n", item);
            continue;
        }
        
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
                            // Určení hierarchické úrovně
                            int level = 0;
                            
                            if (position == 0) level = 0;
                            else if (position == 1) level = 1;
                            else if (position == 2) level = 1;
                            else if (position == 3) level = 1;
                            else if (position >= 4) level = 2;
                            
                            if (flags == 0x3604FD && position > 0) level = 1;
                            else if (flags == 0x704ED) level = 2;
                            
                            // Speciální případy
                            if (strcmp(text, "POUs") == 0) level = 0;
                            else if (strcmp(text, "pBasic") == 0) level = 1;
                            else if (strcmp(text, "PLC_PRG") == 0) level = 2;
                            else if (strstr(text, "ST00") != NULL && hasChildren == 1) level = 1;
                            else if (strstr(text, "ST00") != NULL && hasChildren == 0) level = 2;
                            
                            // Zobraz jen číslo a název s odsazením
                            printf("[%02d] ", item);
                            printIndent(level);
                            printf("%s", text);
                            
                            // Debug info pro pochopení struktury
                            printf(" (pos=%d, flags=0x%X, children=%d, ptr=0x%08X)\n", 
                                   position, flags, hasChildren, textPtr);
                            
                            // Zvýrazni 25. položku
                            if (item == 24) {  // 25. položka má index 24
                                printf("    ^^^ TOTO JE 25. POLOŽKA! ^^^\n");
                            }
                        }
                    }
                } else {
                    printf("[%02d] <neplatný text pointer: 0x%08X>\n", item, textPtr);
                }
            } else {
                printf("[%02d] <nedostatek dat: %zu bytů>\n", item, bytesRead);
            }
        } else {
            printf("[%02d] <nelze číst ItemData: 0x%08lX>\n", item, itemData);
        }
    }
    
    CloseHandle(hProcess);
    
    printf("\n=== SHRNUTÍ ===\n");
    printf("Celkem: %d položek\n", count);
    printf("25. položka (index 24) je zvýrazněna výše\n");
    printf("Pro focus použij: SendMessage(hListBox, LB_SETCURSEL, 24, 0)\n");
    
    printf("\nStiskněte Enter...\n");
    getchar();
    return 0;
}
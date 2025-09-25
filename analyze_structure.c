#include <windows.h>
#include <stdio.h>
#include <string.h>

int main() {
    printf("=== ANALÝZA STRUKTUR ITEMDATA ===\n\n");
    
    // Najdi TwinCAT
    HWND hTwinCAT = NULL;
    HWND hWnd = GetTopWindow(GetDesktopWindow());
    
    while (hWnd) {
        char windowTitle[512];
        if (GetWindowText(hWnd, windowTitle, sizeof(windowTitle))) {
            if (strstr(windowTitle, "TwinCAT") != NULL) {
                hTwinCAT = hWnd;
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
    
    // Analyzuj prvních 20 položek
    for (int item = 0; item < min(20, count); item++) {
        LRESULT itemData = SendMessage(hListBox, LB_GETITEMDATA, item, 0);
        if (itemData == LB_ERR || itemData == 0) continue;
        
        // Přečti strukturu
        DWORD structure[10];  // 40 bytů
        SIZE_T bytesRead;
        
        if (ReadProcessMemory(hProcess, (void*)itemData, structure, sizeof(structure), &bytesRead)) {
            if (bytesRead >= 24) {
                DWORD textPtr = structure[5];  // offset 20 = index 5
                
                printf("Item %02d: ItemData=0x%08lX\n", item, itemData);
                printf("  Struct: %08X %08X %08X %08X %08X %08X\n", 
                       structure[0], structure[1], structure[2], 
                       structure[3], structure[4], structure[5]);
                
                // Přečti text
                if (textPtr > 0x400000 && textPtr < 0x80000000) {
                    char textBuffer[100] = {0};
                    SIZE_T textRead;
                    
                    if (ReadProcessMemory(hProcess, (void*)textPtr, textBuffer, sizeof(textBuffer)-1, &textRead)) {
                        char* text = textBuffer + 1;  // Přeskoč null byte
                        printf("  Text: '%s'\n", text);
                        
                        // Pokus o určení úrovně podle struktury
                        DWORD flags = structure[2];    // offset 8
                        DWORD level_candidate = structure[3];  // offset 12
                        
                        printf("  Možná úroveň: flags=0x%X, level_field=%d\n", 
                               flags, level_candidate & 0xFF);
                    }
                }
                printf("\n");
            }
        }
    }
    
    CloseHandle(hProcess);
    
    printf("Stiskněte Enter...\n");
    getchar();
    return 0;
}
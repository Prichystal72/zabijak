#include <windows.h>
#include <stdio.h>
#include "../lib/twincat_navigator.h"

/**
 * Test: Kompletni analyza struktury ItemData
 */

int main() {
    printf("===================================================\n");
    printf("  KOMPLETNI ANALYZA ITEMDATA STRUKTURY\n");
    printf("===================================================\n\n");
    
    HWND twincatWindow = FindTwinCatWindow();
    if (!twincatWindow) return 1;
    
    HWND listbox = FindProjectListBox(twincatWindow);
    if (!listbox) return 1;
    
    HANDLE hProcess = OpenTwinCatProcess(listbox);
    if (!hProcess) return 1;
    
    int itemCount = GetListBoxItemCount(listbox);
    
    printf("IDX | TEXT                | STRUCT[0]  | STRUCT[1] | STRUCT[2]  | STRUCT[3]\n");
    printf("    |                     | (unknown)  | (Level)   | (Flags)    | (Children)\n");
    printf("==================================================================================\n");
    
    for (int i = 0; i < itemCount; i++) {
        LRESULT itemData = SendMessage(listbox, LB_GETITEMDATA, i, 0);
        if (itemData == LB_ERR || itemData == 0) continue;
        
        DWORD structure[10];
        SIZE_T bytesRead;
        
        if (ReadProcessMemory(hProcess, (void*)itemData, structure, sizeof(structure), &bytesRead)) {
            char text[256] = {0};
            
            // Nacti text
            DWORD textPtr = structure[5];
            if (textPtr > 0x400000 && textPtr < 0x80000000) {
                char textBuffer[256];
                SIZE_T textRead;
                if (ReadProcessMemory(hProcess, (void*)textPtr, textBuffer, sizeof(textBuffer)-1, &textRead)) {
                    strncpy(text, textBuffer + 1, 20);
                    text[20] = '\0';
                }
            }
            
            printf("[%2d] | %-19s | 0x%08X | %9d | 0x%08X | %d\n", 
                   i, text, structure[0], structure[1], structure[2], structure[3]);
        }
    }
    
    printf("\n===================================================\n");
    printf("OTEVREME EPT Lib a znovu zkontrolujeme:\n");
    printf("===================================================\n\n");
    
    ToggleListBoxItem(listbox, 1);
    Sleep(200);
    
    itemCount = GetListBoxItemCount(listbox);
    
    printf("IDX | TEXT                | STRUCT[0]  | STRUCT[1] | STRUCT[2]  | STRUCT[3]\n");
    printf("==================================================================================\n");
    
    for (int i = 0; i < itemCount && i < 12; i++) {
        LRESULT itemData = SendMessage(listbox, LB_GETITEMDATA, i, 0);
        if (itemData == LB_ERR || itemData == 0) continue;
        
        DWORD structure[10];
        SIZE_T bytesRead;
        
        if (ReadProcessMemory(hProcess, (void*)itemData, structure, sizeof(structure), &bytesRead)) {
            char text[256] = {0};
            
            DWORD textPtr = structure[5];
            if (textPtr > 0x400000 && textPtr < 0x80000000) {
                char textBuffer[256];
                SIZE_T textRead;
                if (ReadProcessMemory(hProcess, (void*)textPtr, textBuffer, sizeof(textBuffer)-1, &textRead)) {
                    strncpy(text, textBuffer + 1, 20);
                    text[20] = '\0';
                }
            }
            
            printf("[%2d] | %-19s | 0x%08X | %9d | 0x%08X | %d\n", 
                   i, text, structure[0], structure[1], structure[2], structure[3]);
        }
    }
    
    CloseHandle(hProcess);
    return 0;
}

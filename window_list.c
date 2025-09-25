#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <commctrl.h>

int windowCount = 0;

// Scanner VŠECH top-level oken
BOOL CALLBACK ScanAllTopWindows(HWND hwnd, LPARAM lParam) {
    char className[256];
    char windowText[1024];
    RECT rect;
    
    GetClassName(hwnd, className, sizeof(className));
    GetWindowText(hwnd, windowText, sizeof(windowText));
    GetWindowRect(hwnd, &rect);
    
    // Filtrovat jen viditelná okna s nějakým obsahem
    if (IsWindowVisible(hwnd) && (rect.right - rect.left) > 50 && (rect.bottom - rect.top) > 50) {
        windowCount++;
        
        printf("[%d] Handle: %08p, Class: '%-20s' [%dx%d]", 
               windowCount, hwnd, className, 
               rect.right - rect.left, rect.bottom - rect.top);
        
        if (strlen(windowText) > 0) {
            printf(" Text: '%.80s%s'", windowText, strlen(windowText) > 80 ? "..." : "");
        }
        printf("\n");
        
        // Speciální pozornost pro potenciální TwinCAT okna
        if (strstr(windowText, "TwinCAT") != NULL || 
            strstr(className, "CoDeSys") != NULL ||
            strstr(windowText, "PLC") != NULL ||
            strstr(windowText, "Project") != NULL) {
            
            printf("    >>> POTENCIÁLNÍ TWINCAT OKNO! <<<\n");
        }
    }
    
    return TRUE;
}

int main() {
    printf("=== SCANNER VŠECH VIDITELNÝCH OKEN ===\n");
    printf("Hledáme projektové okno TwinCAT...\n\n");
    
    windowCount = 0;
    EnumWindows(ScanAllTopWindows, 0);
    
    printf("\nCelkem nalezeno %d viditelných oken.\n", windowCount);
    printf("Hledejte okna s 'Project', 'TwinCAT', 'CoDeSys' nebo podobnými názvy.\n");
    printf("\nStiskněte Enter pro ukončení...\n");
    getchar();
    return 0;
}
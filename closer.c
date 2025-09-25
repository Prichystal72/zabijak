#include <windows.h>
#include <stdio.h>
#include <string.h>

char currentUnit[256];
int closedWindows = 0;

// Funkce pro zavírání editor oken
BOOL CALLBACK CloseEditorWindows(HWND hwnd, LPARAM lParam) {
    char className[256];
    char windowText[1024];
    
    GetClassName(hwnd, className, sizeof(className));
    GetWindowText(hwnd, windowText, sizeof(windowText));
    
    if (strcmp(className, "CoDeSys_DoubleEd") == 0) {
        printf("Editor okno: '%s'\n", windowText);
        
        if (strcmp(windowText, currentUnit) != 0) {
            printf("  -> ZAVÍRÁM\n");
            SendMessage(hwnd, WM_CLOSE, 0, 0);
            closedWindows++;
        } else {
            printf("  -> PONECHÁVÁM\n");
        }
    }
    
    return TRUE;
}

// Hlavní callback
BOOL CALLBACK FindTwinCATWindow(HWND hwnd, LPARAM lParam) {
    char title[1024];
    GetWindowText(hwnd, title, sizeof(title));
    
    if (strstr(title, "TwinCAT PLC Control") && strstr(title, "[")) {
        printf("Okno: %s\n", title);
        
        // Extrahuj jednotku
        char *start = strchr(title, '[');
        char *end = strchr(title, ']');
        
        if (start && end && end > start) {
            strncpy(currentUnit, start + 1, end - start - 1);
            currentUnit[end - start - 1] = '\0';
            
            printf("Aktuální: '%s'\n", currentUnit);
            
            closedWindows = 0;
            EnumChildWindows(hwnd, CloseEditorWindows, 0);
            
            printf("Zavřeno: %d\n", closedWindows);
        }
        
        return FALSE;
    }
    
    return TRUE;
}

int main() {
    printf("=== ZAVÍRAČ EDITOR OKEN ===\n");
    EnumWindows(FindTwinCATWindow, 0);
    getchar();
    return 0;
}
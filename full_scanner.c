#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <commctrl.h>

char searchTitle[1024];
int windowLevel = 0;

// Rekurzivní výpis všech child windows
BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam) {
    char className[256];
    char windowText[1024];
    
    GetClassName(hwnd, className, sizeof(className));
    GetWindowText(hwnd, windowText, sizeof(windowText));
    
    // Odsazení podle úrovně
    for (int i = 0; i < windowLevel; i++) {
        printf("  ");
    }
    
    printf("Handle: %08p, Třída: '%s'", hwnd, className);
    if (strlen(windowText) > 0) {
        printf(", Text: '%.50s%s'", windowText, strlen(windowText) > 50 ? "..." : "");
    }
    printf("\n");
    
    // Pokud je to TreeView, zkusme analyzovat
    if (strstr(className, "Tree") != NULL || strstr(className, "SysTree") != NULL) {
        printf("    >>> NALEZEN TREE VIEW! Analyzuji...\n");
        
        // Zkusme získat root item
        HTREEITEM hRoot = (HTREEITEM)SendMessage(hwnd, TVM_GETNEXTITEM, TVGN_ROOT, 0);
        if (hRoot) {
            printf("    Root item: %p\n", hRoot);
            
            // Zkusme získat text root itemu
            TVITEM tvi;
            char buffer[256];
            tvi.mask = TVIF_TEXT;
            tvi.hItem = hRoot;
            tvi.pszText = buffer;
            tvi.cchTextMax = sizeof(buffer);
            
            if (SendMessage(hwnd, TVM_GETITEM, 0, (LPARAM)&tvi)) {
                printf("    Root text: '%s'\n", buffer);
            }
        }
    }
    
    // Rekurzivně projít child okna
    windowLevel++;
    EnumChildWindows(hwnd, EnumChildProc, 0);
    windowLevel--;
    
    return TRUE;
}

BOOL CALLBACK FindTwinCatWindow(HWND hwnd, LPARAM lParam) {
    GetWindowText(hwnd, searchTitle, 1024);
    
    if (strstr(searchTitle, "TwinCAT PLC Control") != NULL && strstr(searchTitle, "[") != NULL) {
        printf("=== ANALÝZA OKNA: %s ===\n", searchTitle);
        
        windowLevel = 0;
        EnumChildWindows(hwnd, EnumChildProc, 0);
        
        return FALSE; // Zastavit hledání
    }
    
    return TRUE;
}

int main() {
    printf("=== KOMPLETNÍ SCANNER VŠECH OVLÁDACÍCH PRVKŮ ===\n");
    EnumWindows(FindTwinCatWindow, 0);
    
    printf("\nSken dokončen. Stiskněte Enter pro ukončení...\n");
    getchar();
    return 0;
}
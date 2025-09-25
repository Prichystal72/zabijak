#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <commctrl.h>

int indent = 0;

void PrintIndent() {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

// Velmi detailní rekurzivní scan TwinCAT okna
BOOL CALLBACK DeepTwinCATScan(HWND hwnd, LPARAM lParam) {
    char className[256];
    char windowText[1024];
    RECT rect;
    
    GetClassName(hwnd, className, sizeof(className));
    GetWindowText(hwnd, windowText, sizeof(windowText));
    GetWindowRect(hwnd, &rect);
    
    PrintIndent();
    printf("Handle: %08p, Class: '%-25s' [%dx%d]", 
           hwnd, className, 
           rect.right - rect.left, rect.bottom - rect.top);
    
    if (strlen(windowText) > 0) {
        printf(" Text: '%.50s%s'", windowText, strlen(windowText) > 50 ? "..." : "");
    }
    
    // Kontrola viditelnosti a aktivnosti
    printf(" [%s%s]", 
           IsWindowVisible(hwnd) ? "V" : "H",
           IsWindowEnabled(hwnd) ? "E" : "D");
    
    printf("\n");
    
    // Speciální pozornost pro různé typy ovládacích prvků
    if (strstr(className, "Tree") != NULL || strstr(className, "SysTree") != NULL) {
        PrintIndent();
        printf(">>> TREE VIEW! <<<\n");
        
        int itemCount = SendMessage(hwnd, TVM_GETCOUNT, 0, 0);
        PrintIndent();
        printf("Items: %d\n", itemCount);
        
        // Zkusme získat strukturu
        if (itemCount > 0) {
            HTREEITEM hRoot = (HTREEITEM)SendMessage(hwnd, TVM_GETNEXTITEM, TVGN_ROOT, 0);
            if (hRoot) {
                TVITEM tvi;
                char buffer[256];
                tvi.mask = TVIF_TEXT;
                tvi.hItem = hRoot;
                tvi.pszText = buffer;
                tvi.cchTextMax = sizeof(buffer);
                
                if (SendMessage(hwnd, TVM_GETITEM, 0, (LPARAM)&tvi)) {
                    PrintIndent();
                    printf("Root: '%s'\n", buffer);
                }
            }
        }
    }
    
    // Speciální pozornost pro Tab Control
    if (strstr(className, "Tab") != NULL || strstr(className, "SysTab") != NULL) {
        PrintIndent();
        printf(">>> TAB CONTROL! <<<\n");
        
        int tabCount = SendMessage(hwnd, TCM_GETITEMCOUNT, 0, 0);
        PrintIndent();
        printf("Tabs: %d\n", tabCount);
        
        for (int i = 0; i < tabCount && i < 10; i++) {
            TCITEM tci;
            char tabText[256];
            tci.mask = TCIF_TEXT;
            tci.pszText = tabText;
            tci.cchTextMax = sizeof(tabText);
            
            if (SendMessage(hwnd, TCM_GETITEM, i, (LPARAM)&tci)) {
                PrintIndent();
                printf("Tab %d: '%s'\n", i, tabText);
            }
        }
    }
    
    // Speciální pozornost pro Splitter/Pane
    if (strstr(className, "Splitter") != NULL || strstr(className, "Pane") != NULL) {
        PrintIndent();
        printf(">>> SPLITTER/PANE! <<<\n");
    }
    
    // Rekurzivně skenovat child okna
    if (indent < 15) { // Omezení hloubky
        indent++;
        EnumChildWindows(hwnd, DeepTwinCATScan, 0);
        indent--;
    }
    
    return TRUE;
}

BOOL CALLBACK FindMainTwinCAT(HWND hwnd, LPARAM lParam) {
    char windowText[1024];
    GetWindowText(hwnd, windowText, sizeof(windowText));
    
    if (strstr(windowText, "TwinCAT PLC Control") != NULL) {
        printf("=== DEEP ANALYSIS OF MAIN TWINCAT WINDOW ===\n");
        printf("Main window: %s\n", windowText);
        printf("=== COMPLETE HIERARCHY ===\n");
        
        indent = 0;
        DeepTwinCATScan(hwnd, 0);
        
        return FALSE; // Stop enumeration
    }
    
    return TRUE;
}

int main() {
    printf("=== ULTRA DEEP TWINCAT WINDOW ANALYSIS ===\n");
    printf("Looking for project tree in main window...\n\n");
    
    EnumWindows(FindMainTwinCAT, 0);
    
    printf("\nAnalysis completed. Press Enter to exit...\n");
    getchar();
    return 0;
}
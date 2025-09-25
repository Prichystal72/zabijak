#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <commctrl.h>

int windowCount = 0;

// Scanner VŠECH oken včetně skrytých
BOOL CALLBACK ScanAllWindows(HWND hwnd, LPARAM lParam) {
    char className[256];
    char windowText[1024];
    RECT rect;
    
    GetClassName(hwnd, className, sizeof(className));
    GetWindowText(hwnd, windowText, sizeof(windowText));
    GetWindowRect(hwnd, &rect);
    
    // Hledáme VŠECHNA okna s TwinCAT/CoDeSys související textu
    if (strstr(windowText, "TwinCAT") != NULL || 
        strstr(className, "CoDeSys") != NULL ||
        strstr(windowText, "PLC") != NULL ||
        strstr(windowText, "Project") != NULL ||
        strstr(className, "TwinCAT") != NULL) {
        
        windowCount++;
        
        printf("[%d] Handle: %08p\n", windowCount, hwnd);
        printf("    Class: '%s'\n", className);
        printf("    Title: '%s'\n", windowText);
        printf("    Size: [%dx%d]\n", rect.right - rect.left, rect.bottom - rect.top);
        printf("    Visible: %s\n", IsWindowVisible(hwnd) ? "YES" : "NO");
        printf("    Enabled: %s\n", IsWindowEnabled(hwnd) ? "YES" : "NO");
        
        // Skenuj child okna na hledání TreeView
        printf("    === CHILD SCAN ===\n");
        HWND hChild = GetWindow(hwnd, GW_CHILD);
        int childNum = 0;
        while (hChild && childNum < 20) {
            char childClass[256];
            char childText[256];
            GetClassName(hChild, childClass, sizeof(childClass));
            GetWindowText(hChild, childText, sizeof(childText));
            
            printf("      [%d] %08p '%-20s'", childNum, hChild, childClass);
            if (strlen(childText) > 0) {
                printf(" '%s'", childText);
            }
            printf("\n");
            
            // Speciální pozornost pro TreeView
            if (strstr(childClass, "Tree") != NULL || strstr(childClass, "SysTree") != NULL) {
                printf("        >>> TREE VIEW FOUND! <<<\n");
                
                int itemCount = SendMessage(hChild, TVM_GETCOUNT, 0, 0);
                printf("        Item count: %d\n", itemCount);
                
                if (itemCount > 0) {
                    HTREEITEM hRoot = (HTREEITEM)SendMessage(hChild, TVM_GETNEXTITEM, TVGN_ROOT, 0);
                    if (hRoot) {
                        TVITEM tvi;
                        char buffer[256];
                        tvi.mask = TVIF_TEXT;
                        tvi.hItem = hRoot;
                        tvi.pszText = buffer;
                        tvi.cchTextMax = sizeof(buffer);
                        
                        if (SendMessage(hChild, TVM_GETITEM, 0, (LPARAM)&tvi)) {
                            printf("        Root text: '%s'\n", buffer);
                        }
                    }
                }
            }
            
            hChild = GetWindow(hChild, GW_HWNDNEXT);
            childNum++;
        }
        printf("    === END CHILD SCAN ===\n\n");
    }
    
    return TRUE;
}

int main() {
    printf("=== HLEDÁNÍ VŠECH TWINCAT/CODESYS OKEN ===\n");
    printf("Včetně skrytých a minimalizovaných...\n\n");
    
    windowCount = 0;
    EnumWindows(ScanAllWindows, 0);
    
    printf("Celkem nalezeno %d TwinCAT souvisejících oken.\n", windowCount);
    printf("\nStiskněte Enter pro ukončení...\n");
    getchar();
    return 0;
}
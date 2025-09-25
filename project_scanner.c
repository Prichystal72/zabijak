#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <commctrl.h>

char searchTitle[1024];
int scanLevel = 0;

// Rekurzivní scanner všech oken a ovládacích prvků
BOOL CALLBACK DeepScanWindows(HWND hwnd, LPARAM lParam) {
    char className[256];
    char windowText[1024];
    RECT rect;
    
    GetClassName(hwnd, className, sizeof(className));
    GetWindowText(hwnd, windowText, sizeof(windowText));
    GetWindowRect(hwnd, &rect);
    
    // Odsazení podle úrovně
    for (int i = 0; i < scanLevel; i++) {
        printf("  ");
    }
    
    printf("Handle: %08p, Class: '%-20s'", hwnd, className);
    
    // Zobrazit rozměry okna
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    printf(" [%dx%d]", width, height);
    
    if (strlen(windowText) > 0) {
        printf(" Text: '%.60s%s'", windowText, strlen(windowText) > 60 ? "..." : "");
    }
    printf("\n");
    
    // Speciální analýza pro TreeView
    if (strstr(className, "Tree") != NULL || strstr(className, "SysTree") != NULL) {
        printf("    >>> TREE VIEW DETECTED! <<<\n");
        
        // Zkusme získat počet itemů
        int itemCount = SendMessage(hwnd, TVM_GETCOUNT, 0, 0);
        printf("    Item count: %d\n", itemCount);
        
        if (itemCount > 0) {
            // Zkusme získat root item
            HTREEITEM hRoot = (HTREEITEM)SendMessage(hwnd, TVM_GETNEXTITEM, TVGN_ROOT, 0);
            if (hRoot) {
                printf("    Root item handle: %p\n", hRoot);
                
                // Zkusme získat text root itemu
                TVITEM tvi;
                char buffer[256];
                tvi.mask = TVIF_TEXT;
                tvi.hItem = hRoot;
                tvi.pszText = buffer;
                tvi.cchTextMax = sizeof(buffer);
                
                if (SendMessage(hwnd, TVM_GETITEM, 0, (LPARAM)&tvi)) {
                    printf("    Root text: '%s'\n", buffer);
                    
                    // Zkusme najít child itemy
                    HTREEITEM hChild = (HTREEITEM)SendMessage(hwnd, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)hRoot);
                    int childCount = 0;
                    while (hChild && childCount < 5) {
                        tvi.hItem = hChild;
                        if (SendMessage(hwnd, TVM_GETITEM, 0, (LPARAM)&tvi)) {
                            printf("    Child %d: '%s'\n", childCount, buffer);
                        }
                        hChild = (HTREEITEM)SendMessage(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hChild);
                        childCount++;
                    }
                }
            }
        }
    }
    
    // Speciální analýza pro ListView
    if (strstr(className, "List") != NULL && strstr(className, "SysListView") != NULL) {
        printf("    >>> LIST VIEW DETECTED! <<<\n");
        
        int itemCount = SendMessage(hwnd, LVM_GETITEMCOUNT, 0, 0);
        printf("    Item count: %d\n", itemCount);
        
        if (itemCount > 0 && itemCount < 20) {
            // Zobrazit prvních několik položek
            for (int i = 0; i < min(itemCount, 5); i++) {
                LVITEM lvi;
                char buffer[256];
                lvi.mask = LVIF_TEXT;
                lvi.iItem = i;
                lvi.iSubItem = 0;
                lvi.pszText = buffer;
                lvi.cchTextMax = sizeof(buffer);
                
                if (SendMessage(hwnd, LVM_GETITEMTEXT, i, (LPARAM)&lvi)) {
                    printf("    Item %d: '%s'\n", i, buffer);
                }
            }
        }
    }
    
    // Rekurzivně skenovat child okna (jen pokud není příliš velká hjerarchie)
    if (scanLevel < 10) {
        scanLevel++;
        EnumChildWindows(hwnd, DeepScanWindows, 0);
        scanLevel--;
    }
    
    return TRUE;
}

// Scanner všech top-level oken
BOOL CALLBACK ScanAllWindows(HWND hwnd, LPARAM lParam) {
    char className[256];
    char windowText[1024];
    
    GetClassName(hwnd, className, sizeof(className));
    GetWindowText(hwnd, windowText, sizeof(windowText));
    
    // Hledáme všechna okna která mohou obsahovat TwinCAT
    if (strstr(windowText, "TwinCAT") != NULL || 
        strstr(className, "CoDeSys") != NULL ||
        strstr(windowText, "PLC") != NULL) {
        
        printf("\n=== NALEZENO POTENCIÁLNÍ TWINCAT OKNO ===\n");
        printf("Handle: %08p\n", hwnd);
        printf("Class: '%s'\n", className);
        printf("Title: '%s'\n", windowText);
        printf("=== DEEP SCAN ===\n");
        
        scanLevel = 0;
        DeepScanWindows(hwnd, 0);
        
        printf("=== KONEC DEEP SCAN ===\n");
    }
    
    return TRUE;
}

int main() {
    printf("=== ROZŠÍŘENÝ SCANNER PROJEKTOVÝCH STROMŮ ===\n");
    printf("Hledáme TreeView, ListView a další ovládací prvky...\n\n");
    
    EnumWindows(ScanAllWindows, 0);
    
    printf("\nScan dokončen. Stiskněte Enter pro ukončení...\n");
    getchar();
    return 0;
}
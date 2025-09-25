#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <commctrl.h>

char targetUnit[256] = {0};
HWND mainTwinCatWindow = NULL;

// Rekurzivní scanner dokovaných panelů
BOOL CALLBACK ScanDockedPanels(HWND hwnd, LPARAM level) {
    char className[256];
    char windowText[1024];
    RECT rect;
    
    GetClassName(hwnd, className, sizeof(className));
    GetWindowText(hwnd, windowText, sizeof(windowText));
    GetWindowRect(hwnd, &rect);
    
    // Odsazení podle úrovně
    for (int i = 0; i < (int)level; i++) {
        printf("  ");
    }
    
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    printf("Handle: %08p [%dx%d] Class: '%s'", hwnd, width, height, className);
    if (strlen(windowText) > 0) {
        printf(" Text: '%.40s%s'", windowText, strlen(windowText) > 40 ? "..." : "");
    }
    printf("\n");
    
    // Speciální pozornost pro záložky (TabControl)
    if (strstr(className, "Tab") != NULL || strstr(className, "SysTab") != NULL) {
        for (int i = 0; i < (int)level; i++) printf("  ");
        printf("    >>> TAB CONTROL NALEZEN! <<<\n");
        
        int tabCount = SendMessage(hwnd, TCM_GETITEMCOUNT, 0, 0);
        for (int i = 0; i < (int)level; i++) printf("  ");
        printf("    Počet záložek: %d\n", tabCount);
        
        // Zobrazit názvy záložek
        for (int i = 0; i < tabCount && i < 10; i++) {
            TCITEM tci;
            char tabText[256];
            tci.mask = TCIF_TEXT;
            tci.pszText = tabText;
            tci.cchTextMax = sizeof(tabText);
            
            if (SendMessage(hwnd, TCM_GETITEM, i, (LPARAM)&tci)) {
                for (int j = 0; j < (int)level; j++) printf("  ");
                printf("    Záložka [%d]: '%s'\n", i, tabText);
            }
        }
        
        // Získat aktuálně aktivní záložku
        int activeTab = SendMessage(hwnd, TCM_GETCURSEL, 0, 0);
        for (int i = 0; i < (int)level; i++) printf("  ");
        printf("    Aktivní záložka: %d\n", activeTab);
    }
    
    // Speciální pozornost pro TreeView
    if (strstr(className, "Tree") != NULL || strstr(className, "SysTree") != NULL) {
        for (int i = 0; i < (int)level; i++) printf("  ");
        printf("    >>> TREE VIEW NALEZEN! <<<\n");
        
        int itemCount = SendMessage(hwnd, TVM_GETCOUNT, 0, 0);
        for (int i = 0; i < (int)level; i++) printf("  ");
        printf("    Počet položek: %d\n", itemCount);
        
        if (itemCount > 0 && itemCount < 500) {
            for (int i = 0; i < (int)level; i++) printf("  ");
            printf("    *** MOŽNÝ PROJEKTOVÝ STROM! ***\n");
            
            // Zkusme zobrazit první pár položek
            HTREEITEM hRoot = (HTREEITEM)SendMessage(hwnd, TVM_GETNEXTITEM, TVGN_ROOT, 0);
            if (hRoot) {
                TVITEM tvi;
                char buffer[256];
                tvi.mask = TVIF_TEXT;
                tvi.hItem = hRoot;
                tvi.pszText = buffer;
                tvi.cchTextMax = sizeof(buffer);
                
                if (SendMessage(hwnd, TVM_GETITEM, 0, (LPARAM)&tvi)) {
                    for (int i = 0; i < (int)level; i++) printf("  ");
                    printf("    Root item: '%s'\n", buffer);
                    
                    // Zobrazit první pár child itemů
                    HTREEITEM hChild = (HTREEITEM)SendMessage(hwnd, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)hRoot);
                    int childNum = 0;
                    while (hChild && childNum < 5) {
                        tvi.hItem = hChild;
                        if (SendMessage(hwnd, TVM_GETITEM, 0, (LPARAM)&tvi)) {
                            for (int i = 0; i < (int)level; i++) printf("  ");
                            printf("    Child [%d]: '%s'\n", childNum, buffer);
                            
                            // Zkontrolovat, zda obsahuje naši cílovou jednotku
                            if (strlen(targetUnit) > 0 && strstr(buffer, targetUnit) != NULL) {
                                for (int i = 0; i < (int)level; i++) printf("  ");
                                printf("    *** NAŠLI JSME CÍLOVOU JEDNOTKU! ***\n");
                            }
                        }
                        hChild = (HTREEITEM)SendMessage(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hChild);
                        childNum++;
                    }
                }
            }
        }
    }
    
    // Hledáme panely které mohou být dokované vlevo
    if (width > 100 && width < 600 && height > 300) { // Typické rozměry dokovaného panelu
        RECT mainRect;
        GetWindowRect(mainTwinCatWindow, &mainRect);
        
        // Je tento panel na levé straně hlavního okna?
        if (rect.left <= mainRect.left + 50) {
            for (int i = 0; i < (int)level; i++) printf("  ");
            printf("    -> MOŽNÝ LEVÝ DOKOVANÝ PANEL\n");
        }
    }
    
    // Rekurzivní scan child oken (omezujeme hloubku)
    if (level < 8) {
        HWND hChild = GetWindow(hwnd, GW_CHILD);
        while (hChild) {
            ScanDockedPanels(hChild, level + 1);
            hChild = GetWindow(hChild, GW_HWNDNEXT);
        }
    }
    
    return TRUE;
}

// Callback pro hledání hlavního TwinCAT okna
BOOL CALLBACK FindMainTwinCatWindow(HWND hwnd, LPARAM lParam) {
    char title[1024];
    GetWindowText(hwnd, title, sizeof(title));
    
    if (strstr(title, "TwinCAT PLC Control") != NULL && strstr(title, "[") != NULL) {
        printf("=== NALEZENO HLAVNÍ TWINCAT OKNO ===\n");
        printf("Titulek: %s\n", title);
        
        mainTwinCatWindow = hwnd;
        
        // Extrahuj cílovou jednotku
        char *start = strchr(title, '[');
        char *end = strchr(title, ']');
        if (start && end && end > start) {
            strncpy(targetUnit, start + 1, end - start - 1);
            targetUnit[end - start - 1] = '\0';
            printf("Cílová jednotka: '%s'\n", targetUnit);
        }
        
        RECT rect;
        GetWindowRect(hwnd, &rect);
        printf("Rozměry okna: [%dx%d]\n", rect.right - rect.left, rect.bottom - rect.top);
        
        printf("\n=== HLUBOKÝ SCAN DOKOVANÝCH PANELŮ ===\n");
        ScanDockedPanels(hwnd, 0);
        
        return FALSE; // Zastavit hledání
    }
    
    return TRUE;
}

int main() {
    printf("=== SCANNER DOKOVANÝCH PANELŮ A ZÁLOŽEK ===\n");
    printf("Hledáme dokovaný panel vlevo se záložkami obsahující projektový strom.\n\n");
    
    EnumWindows(FindMainTwinCatWindow, 0);
    
    if (mainTwinCatWindow == NULL) {
        printf("TwinCAT okno nebylo nalezeno!\n");
    }
    
    printf("\nScan dokončen. Stiskněte Enter pro ukončení...\n");
    getchar();
    return 0;
}
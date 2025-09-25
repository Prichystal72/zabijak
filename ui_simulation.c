#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <commctrl.h>

char targetUnit[256] = "ST03_Main";

// Funkce pro získání textu ze statusbaru
void GetStatusBarText(HWND mainWindow) {
    // Najdi statusbar v hlavním okně
    HWND statusBar = FindWindowEx(mainWindow, NULL, "msctls_statusbar32", NULL);
    if (statusBar) {
        char statusText[512];
        GetWindowText(statusBar, statusText, sizeof(statusText));
        if (strlen(statusText) > 0) {
            printf("   Status bar: '%s'\n", statusText);
            
            if (strstr(statusText, targetUnit)) {
                printf("   *** NALEZENA CÍLOVÁ JEDNOTKA VE STATUSBARU! ***\n");
            }
        }
        
        // Zkusit získat text z různých částí statusbaru
        int parts = SendMessage(statusBar, SB_GETPARTS, 0, 0);
        printf("   Status bar má %d částí\n", parts);
        
        for (int i = 0; i < parts && i < 5; i++) {
            char partText[256];
            SendMessage(statusBar, SB_GETTEXT, i, (LPARAM)partText);
            if (strlen(partText) > 0) {
                printf("   Část %d: '%s'\n", i, partText);
                if (strstr(partText, targetUnit)) {
                    printf("   *** NALEZENA CÍLOVÁ JEDNOTKA V ČÁSTI %d! ***\n", i);
                }
            }
        }
    }
}

// Funkce pro získání tooltip textu
void GetTooltipText(HWND listBox) {
    // Najdi tooltip control v parent okně
    HWND parent = GetParent(listBox);
    HWND tooltip = FindWindowEx(parent, NULL, "tooltips_class32", NULL);
    
    if (tooltip) {
        char tooltipText[256];
        GetWindowText(tooltip, tooltipText, sizeof(tooltipText));
        if (strlen(tooltipText) > 0) {
            printf("   Tooltip: '%s'\n", tooltipText);
            if (strstr(tooltipText, targetUnit)) {
                printf("   *** NALEZENA CÍLOVÁ JEDNOTKA V TOOLTIPU! ***\n");
            }
        }
    }
}

// Callback pro hledání TwinCAT okna
BOOL CALLBACK FindTwinCatWindow(HWND hwnd, LPARAM lParam) {
    char title[1024];
    GetWindowText(hwnd, title, sizeof(title));
    
    if (strstr(title, "TwinCAT PLC Control") && strstr(title, "[") && strstr(title, "]")) {
        printf("=== UI SIMULACE PRO HLEDÁNÍ JEDNOTKY ===\n");
        printf("Okno: %s\n", title);
        
        // Extrahuj cílovou jednotku z titulku
        char *start = strchr(title, '[');
        char *end = strchr(title, ']');
        if (start && end && end > start) {
            strncpy(targetUnit, start + 1, end - start - 1);
            targetUnit[end - start - 1] = '\0';
            printf("Hledáme: '%s'\n", targetUnit);
        }
        
        // Najdi dokovaný ListBox
        HWND hChild = GetWindow(hwnd, GW_CHILD);
        while (hChild) {
            char className[256];
            RECT rect;
            GetClassName(hChild, className, sizeof(className));
            GetWindowRect(hChild, &rect);
            
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;
            
            if (strcmp(className, "ListBox") == 0 && width > 500 && width < 700 && height > 1000) {
                printf("\nListBox nalezen: %08p [%dx%d]\n", hChild, width, height);
                
                int itemCount = SendMessage(hChild, LB_GETCOUNT, 0, 0);
                printf("Počet položek: %d\n", itemCount);
                
                printf("\n=== TESTOVÁNÍ UI INTERAKCE ===\n");
                
                // Projdi prvních 10 položek
                for (int i = 0; i < min(itemCount, 10); i++) {
                    printf("\n[%d] Testuji položku %d:\n", i, i);
                    
                    // Vyber položku
                    SendMessage(hChild, LB_SETCURSEL, i, 0);
                    Sleep(100); // Krátká pauza pro UI update
                    
                    // Zkontroluj, zda je vybrána
                    int selected = SendMessage(hChild, LB_GETCURSEL, 0, 0);
                    printf("   Vybraná položka: %d\n", selected);
                    
                    // Získej text ze statusbaru
                    GetStatusBarText(hwnd);
                    
                    // Získej tooltip
                    GetTooltipText(hChild);
                    
                    // Zkus poslat různé zprávy
                    LRESULT textLen = SendMessage(hChild, LB_GETTEXTLEN, i, 0);
                    printf("   LB_GETTEXTLEN: %ld\n", textLen);
                    
                    // Zkus simulovat double-click
                    RECT itemRect;
                    SendMessage(hChild, LB_GETITEMRECT, i, (LPARAM)&itemRect);
                    
                    // Pošli mouse zprávy
                    POINT center = {(itemRect.left + itemRect.right) / 2, (itemRect.top + itemRect.bottom) / 2};
                    PostMessage(hChild, WM_LBUTTONDOWN, 0, MAKELPARAM(center.x, center.y));
                    Sleep(50);
                    PostMessage(hChild, WM_LBUTTONUP, 0, MAKELPARAM(center.x, center.y));
                    Sleep(100);
                    
                    // Zkontroluj znovu statusbar po kliku
                    printf("   Po kliknutí:\n");
                    GetStatusBarText(hwnd);
                    
                    // Projdi všechna child windows a hledej text
                    printf("   Hledání textu ve všech child windows:\n");
                    HWND childWindow = GetWindow(hwnd, GW_CHILD);
                    int childCount = 0;
                    
                    while (childWindow && childCount < 20) {
                        char childClass[256];
                        char childText[512];
                        GetClassName(childWindow, childClass, sizeof(childClass));
                        GetWindowText(childWindow, childText, sizeof(childText));
                        
                        if (strlen(childText) > 0) {
                            printf("   Child %s: '%s'\n", childClass, childText);
                            if (strstr(childText, targetUnit) || strstr(childText, "ST03") || strstr(childText, "Main")) {
                                printf("   *** MOŽNÝ ZÁSAH V CHILD WINDOW! ***\n");
                            }
                        }
                        
                        childWindow = GetWindow(childWindow, GW_HWNDNEXT);
                        childCount++;
                    }
                    
                    // Zkus WM_GETTEXT na různé kontroly
                    printf("   Testování WM_GETTEXT zpráv:\n");
                    HWND testWindow = GetWindow(hwnd, GW_CHILD);
                    childCount = 0;
                    
                    while (testWindow && childCount < 15) {
                        char testText[256];
                        int textLen = SendMessage(testWindow, WM_GETTEXT, sizeof(testText), (LPARAM)testText);
                        
                        if (textLen > 0 && strlen(testText) > 3) {
                            char testClass[128];
                            GetClassName(testWindow, testClass, sizeof(testClass));
                            printf("   WM_GETTEXT %s: '%s'\n", testClass, testText);
                            
                            if (strstr(testText, targetUnit) || strstr(testText, "ST03") || strstr(testText, "Main")) {
                                printf("   *** MOŽNÝ ZÁSAH VE WM_GETTEXT! ***\n");
                            }
                        }
                        
                        testWindow = GetWindow(testWindow, GW_HWNDNEXT);
                        childCount++;
                    }
                }
                
                break;
            }
            
            hChild = GetWindow(hChild, GW_HWNDNEXT);
        }
        
        return FALSE;
    }
    
    return TRUE;
}

int main() {
    printf("=== UI SIMULACE PRO HLEDÁNÍ JEDNOTKY V PROJEKTOVÉM STROMU ===\n");
    printf("Testuje různé způsoby získání textu přes UI interakci...\n\n");
    
    EnumWindows(FindTwinCatWindow, 0);
    
    printf("\nAnalýza dokončena. Stiskněte Enter...\n");
    getchar();
    return 0;
}
#include <windows.h>
#include <stdio.h>
#include <string.h>

char targetUnit[256] = "ST03_Main";
DWORD twinCatProcessId = 0;

// Callback pro enumeraci všech oken procesu
BOOL CALLBACK EnumProcessWindows(HWND hwnd, LPARAM lParam) {
    DWORD windowProcessId;
    GetWindowThreadProcessId(hwnd, &windowProcessId);
    
    if (windowProcessId == twinCatProcessId) {
        char title[512];
        char className[256];
        GetWindowText(hwnd, title, sizeof(title));
        GetClassName(hwnd, className, sizeof(className));
        
        if (strlen(title) > 0) {
            printf("   PROCES OKNO: '%s' [%s]\n", title, className);
            
            if (strstr(title, targetUnit) || strstr(title, "ST03") || strstr(title, "Main")) {
                printf("   *** MOŽNÝ ZÁSAH V OKNĚ PROCESU! ***\n");
            }
        }
    }
    
    return TRUE;
}

BOOL CALLBACK FindTwinCatWindow(HWND hwnd, LPARAM lParam) {
    char title[1024];
    GetWindowText(hwnd, title, sizeof(title));
    
    if (strstr(title, "TwinCAT PLC Control") && strstr(title, "[") && strstr(title, "]")) {
        printf("=== DOUBLE-CLICK SIMULACE ===\n");
        printf("Okno: %s\n", title);
        
        // Získej process ID
        GetWindowThreadProcessId(hwnd, &twinCatProcessId);
        printf("Process ID: %d\n", twinCatProcessId);
        
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
                
                printf("\n=== DOUBLE-CLICK SIMULACE NA POLOŽKÁCH ===\n");
                
                // Testuj prvních 5 položek s double-clickem
                for (int i = 0; i < min(itemCount, 5); i++) {
                    printf("\n[%d] Double-click na položku %d:\n", i, i);
                    
                    // Získej pozici položky
                    RECT itemRect;
                    if (SendMessage(hChild, LB_GETITEMRECT, i, (LPARAM)&itemRect) != LB_ERR) {
                        // Spočítej střed v client coordinates
                        POINT center = {(itemRect.left + itemRect.right) / 2, (itemRect.top + itemRect.bottom) / 2};
                        
                        // Převeď na screen coordinates
                        ClientToScreen(hChild, &center);
                        
                        printf("   Klikám na pozici [%d,%d]\n", center.x, center.y);
                        
                        // Nastav kurzor a proveď double-click
                        SetCursorPos(center.x, center.y);
                        Sleep(100);
                        
                        // Simulate double-click
                        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                        Sleep(50);
                        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                        
                        Sleep(500); // Čekej na reakci UI
                        
                        // Projdi všechna okna procesu
                        printf("   Hledám změny ve všech oknech procesu:\n");
                        EnumWindows(EnumProcessWindows, 0);
                        
                        // Zkontroluj změny v původním okně
                        char newTitle[1024];
                        GetWindowText(hwnd, newTitle, sizeof(newTitle));
                        if (strcmp(title, newTitle) != 0) {
                            printf("   *** ZMĚNA V TITULKU HLAVNÍHO OKNA! ***\n");
                            printf("   Nový titulek: '%s'\n", newTitle);
                        }
                        
                        // Zkontroluj focus window
                        HWND focusWindow = GetForegroundWindow();
                        if (focusWindow != hwnd) {
                            char focusTitle[512];
                            char focusClass[256];
                            GetWindowText(focusWindow, focusTitle, sizeof(focusTitle));
                            GetClassName(focusWindow, focusClass, sizeof(focusClass));
                            
                            if (strlen(focusTitle) > 0) {
                                printf("   Focus přešel na: '%s' [%s]\n", focusTitle, focusClass);
                                
                                if (strstr(focusTitle, targetUnit) || strstr(focusTitle, "ST03") || strstr(focusTitle, "Main")) {
                                    printf("   *** MOŽNÝ ZÁSAH V NOVÉM FOCUS OKNĚ! ***\n");
                                }
                            }
                        }
                        
                        printf("\n");
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
    printf("=== DOUBLE-CLICK SIMULACE PRO HLEDÁNÍ JEDNOTKY ===\n");
    printf("Simuluje double-click na položky a sleduje změny v UI...\n\n");
    
    EnumWindows(FindTwinCatWindow, 0);
    
    printf("\nAnalýza dokončena. Stiskněte Enter...\n");
    getchar();
    return 0;
}
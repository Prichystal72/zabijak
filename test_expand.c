#include <windows.h>
#include <stdio.h>

int main() {
    printf("=== TEST ROZBALOVANI ===\n");
    
    // Najdi TwinCAT
    HWND hWnd = GetTopWindow(GetDesktopWindow());
    while (hWnd) {
        char windowTitle[512];
        if (GetWindowText(hWnd, windowTitle, sizeof(windowTitle))) {
            if (strstr(windowTitle, "TwinCAT") != NULL) {
                printf("TwinCAT: %s\n", windowTitle);
                break;
            }
        }
        hWnd = GetNextWindow(hWnd, GW_HWNDNEXT);
    }
    
    if (!hWnd) {
        printf("TwinCAT nenalezen!\n");
        return 1;
    }
    
    // Najdi ListBox
    HWND bestListBox = NULL;
    int bestScore = 0;
    
    HWND childWindow = GetWindow(hWnd, GW_CHILD);
    while (childWindow != NULL) {
        char className[256];
        GetClassName(childWindow, className, sizeof(className));
        if (strcmp(className, "ListBox") == 0) {
            int itemCount = SendMessage(childWindow, LB_GETCOUNT, 0, 0);
            if (itemCount > bestScore) {
                bestScore = itemCount;
                bestListBox = childWindow;
            }
        }
        
        HWND subChild = GetWindow(childWindow, GW_CHILD);
        while (subChild != NULL) {
            char subClassName[256];
            GetClassName(subChild, subClassName, sizeof(subClassName));
            if (strcmp(subClassName, "ListBox") == 0) {
                int itemCount = SendMessage(subChild, LB_GETCOUNT, 0, 0);
                if (itemCount > bestScore) {
                    bestScore = itemCount;
                    bestListBox = subChild;
                }
            }
            subChild = GetWindow(subChild, GW_HWNDNEXT);
        }
        childWindow = GetWindow(childWindow, GW_HWNDNEXT);
    }
    
    printf("ListBox: %p (%d polozek)\n", bestListBox, bestScore);
    
    // Test rozbalování na různých položkách
    int testItems[] = {0, 2, 10, 22, -1};  // -1 = konec
    
    for (int i = 0; testItems[i] != -1; i++) {
        int itemIndex = testItems[i];
        
        printf("\n=== TEST POLOZKY %d ===\n", itemIndex);
        
        // Počet před
        int itemsBefore = SendMessage(bestListBox, LB_GETCOUNT, 0, 0);
        printf("Polozek pred: %d\n", itemsBefore);
        
        // Nastav focus
        SendMessage(bestListBox, LB_SETCURSEL, itemIndex, 0);
        int currentSel = SendMessage(bestListBox, LB_GETCURSEL, 0, 0);
        printf("Focus nastaven na: %d\n", currentSel);
        
        if (currentSel == itemIndex) {
            // Aktivuj okno a pošli Enter
            SetForegroundWindow(hWnd);
            Sleep(200);
            
            // Zkus dvojklik místo Enter
            RECT itemRect;
            SendMessage(bestListBox, LB_GETITEMRECT, itemIndex, (LPARAM)&itemRect);
            
            int x = itemRect.left + 10;
            int y = itemRect.top + (itemRect.bottom - itemRect.top) / 2;
            
            SetCursorPos(x, y);
            
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            Sleep(50);
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            
            Sleep(500);  // Dej TwinCAT času
            
            // Počet po
            int itemsAfter = SendMessage(bestListBox, LB_GETCOUNT, 0, 0);
            printf("Polozek po: %d\n", itemsAfter);
            
            if (itemsAfter > itemsBefore) {
                printf("✅ ROZBALENO! (%d -> %d)\n", itemsBefore, itemsAfter);
            } else {
                printf("⚠️  Nerozbalilo se\n");
            }
        } else {
            printf("❌ Focus se nenastavil!\n");
        }
        
        printf("Pokracovat? (Enter)\n");
        getchar();
    }
    
    return 0;
}
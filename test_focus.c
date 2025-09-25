#include <windows.h>
#include <stdio.h>

int main() {
    printf("=== TEST FOCUS ===\n");
    
    // Najdi TwinCAT okno
    HWND hWnd = FindWindow(NULL, NULL);
    hWnd = GetTopWindow(GetDesktopWindow());
    
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
            printf("ListBox %p: %d polozek\n", childWindow, itemCount);
        }

        // Rekurzivně
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
                printf("  Sub-ListBox %p: %d polozek\n", subChild, itemCount);
            }
            subChild = GetWindow(subChild, GW_HWNDNEXT);
        }

        childWindow = GetWindow(childWindow, GW_HWNDNEXT);
    }
    
    if (!bestListBox) {
        printf("ListBox nenalezen!\n");
        return 1;
    }
    
    printf("Nejlepsi ListBox: %p (%d polozek)\n", bestListBox, bestScore);
    
    // TEST FOCUS
    printf("\nTEST 1: Aktualni selection\n");
    int currentSel = SendMessage(bestListBox, LB_GETCURSEL, 0, 0);
    printf("Aktualni selection: %d\n", currentSel);
    
    printf("\nTEST 2: Nastaveni focus na polozku 5\n");
    LRESULT result = SendMessage(bestListBox, LB_SETCURSEL, 5, 0);
    printf("LB_SETCURSEL result: %d\n", (int)result);
    
    Sleep(100);
    
    printf("\nTEST 3: Kontrola noveho selection\n");
    int newSel = SendMessage(bestListBox, LB_GETCURSEL, 0, 0);
    printf("Novy selection: %d\n", newSel);
    
    if (newSel == 5) {
        printf("✅ Focus nastaven uspesne!\n");
        
        printf("\nTEST 4: Poslani Enter\n");
        SetForegroundWindow(hWnd);
        Sleep(100);
        
        keybd_event(VK_RETURN, 0, 0, 0);
        keybd_event(VK_RETURN, 0, KEYEVENTF_KEYUP, 0);
        
        printf("Enter poslan!\n");
        
    } else {
        printf("❌ Focus se nenastavil!\n");
    }
    
    printf("\nStiskni Enter...\n");
    getchar();
    
    return 0;
}
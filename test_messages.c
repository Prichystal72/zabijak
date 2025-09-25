#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

int main() {
    printf("=== TEST ZPRAV ===\n");
    
    // Najdi TwinCAT a ListBox
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
    
    // Test na položce 22 (zLibrary)
    int testItem = 22;
    
    printf("\n=== TEST ZPRAV NA POLOZCE %d ===\n", testItem);
    
    // Nastav focus
    SendMessage(bestListBox, LB_SETCURSEL, testItem, 0);
    printf("Focus nastaven na: %d\n", SendMessage(bestListBox, LB_GETCURSEL, 0, 0));
    
    int itemsBefore = SendMessage(bestListBox, LB_GETCOUNT, 0, 0);
    printf("Polozek pred: %d\n", itemsBefore);
    
    // Zkus různé zprávy
    printf("\n1. WM_LBUTTONDBLCLK...\n");
    SendMessage(bestListBox, WM_LBUTTONDBLCLK, 0, MAKELPARAM(10, 10));
    Sleep(300);
    int items1 = SendMessage(bestListBox, LB_GETCOUNT, 0, 0);
    printf("Po WM_LBUTTONDBLCLK: %d polozek\n", items1);
    
    if (items1 == itemsBefore) {
        printf("\n2. WM_KEYDOWN (VK_RETURN)...\n");
        SendMessage(bestListBox, WM_KEYDOWN, VK_RETURN, 0);
        SendMessage(bestListBox, WM_KEYUP, VK_RETURN, 0);
        Sleep(300);
        int items2 = SendMessage(bestListBox, LB_GETCOUNT, 0, 0);
        printf("Po WM_KEYDOWN: %d polozek\n", items2);
        
        if (items2 == itemsBefore) {
            printf("\n3. WM_KEYDOWN (VK_RIGHT)...\n");
            SendMessage(bestListBox, WM_KEYDOWN, VK_RIGHT, 0);
            SendMessage(bestListBox, WM_KEYUP, VK_RIGHT, 0);
            Sleep(300);
            int items3 = SendMessage(bestListBox, LB_GETCOUNT, 0, 0);
            printf("Po VK_RIGHT: %d polozek\n", items3);
            
            if (items3 == itemsBefore) {
                printf("\n4. WM_KEYDOWN (VK_ADD - Plus)...\n");
                SendMessage(bestListBox, WM_KEYDOWN, VK_ADD, 0);
                SendMessage(bestListBox, WM_KEYUP, VK_ADD, 0);
                Sleep(300);
                int items4 = SendMessage(bestListBox, LB_GETCOUNT, 0, 0);
                printf("Po VK_ADD: %d polozek\n", items4);
                
                if (items4 == itemsBefore) {
                    printf("\n5. LB_INSERTSTRING (hack)...\n");
                    // Tohle je hack - zkusím "vložit" položku
                    LRESULT result = SendMessage(bestListBox, LB_INSERTSTRING, testItem + 1, (LPARAM)"TEST");
                    printf("LB_INSERTSTRING result: %d\n", (int)result);
                    Sleep(300);
                    int items5 = SendMessage(bestListBox, LB_GETCOUNT, 0, 0);
                    printf("Po LB_INSERTSTRING: %d polozek\n", items5);
                    
                    if (items5 > itemsBefore) {
                        printf("✅ LB_INSERTSTRING FUNGUJE!\n");
                        // Smaž testovací položku
                        SendMessage(bestListBox, LB_DELETESTRING, testItem + 1, 0);
                    }
                }
            }
        }
    }
    
    printf("\nStiskni Enter...\n");
    getchar();
    
    return 0;
}
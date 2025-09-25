#include <windows.h>
#include <stdio.h>
#include <commctrl.h>

// Najde TwinCAT okno
HWND FindTwinCatWindow() {
    HWND hwnd = FindWindow(NULL, NULL);
    char title[1024];
    
    while (hwnd != NULL) {
        GetWindowText(hwnd, title, sizeof(title));
        if (strstr(title, "TwinCAT PLC Control") != NULL) {
            printf("Nalezeno TwinCAT okno: %s\n", title);
            return hwnd;
        }
        hwnd = GetWindow(hwnd, GW_HWNDNEXT);
    }
    return NULL;
}

// Callback pro vyhledávání ListBox
BOOL CALLBACK EnumChildProc(HWND hwndChild, LPARAM lParam) {
    HWND* pListBox = (HWND*)lParam;
    char className[100];
    
    GetClassName(hwndChild, className, sizeof(className));
    if (strcmp(className, "ListBox") == 0) {
        LONG style = GetWindowLong(hwndChild, GWL_STYLE);
        if (style & LBS_OWNERDRAWFIXED) {
            printf("Nalezen ListBox pro project tree: %08x\n", hwndChild);
            *pListBox = hwndChild;
            return FALSE; // Zastavit enumeraci při nalezení prvního
        }
    }
    return TRUE; // Pokračovat v enumeraci
}

// Rozbalí všechny uzly v TreeView/ListBox
void ExpandAll(HWND hwnd, HWND hListBox) {
    printf("\n=== ZAČÍNÁM ROZBALOVÁNÍ VŠECH UZLŮ ===\n");
    
    // Ujisti se, že ListBox má focus
    SetFocus(hListBox);
    Sleep(100);
    
    int originalCount = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
    printf("Počáteční počet položek: %d\n", originalCount);
    
    // Projdi všechny položky a zkus je rozbalit
    for (int i = 0; i < 50; i++) { // Omez na 50 iterací
        SendMessage(hListBox, LB_SETCURSEL, i, 0);
        Sleep(50);
        
        // Zkus rozbalit pomocí šipky vpravo
        keybd_event(VK_RIGHT, 0, 0, 0);
        keybd_event(VK_RIGHT, 0, KEYEVENTF_KEYUP, 0);
        Sleep(50);
        
        // Zkus rozbalit pomocí Enter
        keybd_event(VK_RETURN, 0, 0, 0);
        keybd_event(VK_RETURN, 0, KEYEVENTF_KEYUP, 0);
        Sleep(50);
        
        int newCount = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
        if (newCount > originalCount) {
            printf("Rozbaleno - nový počet: %d (bylo %d)\n", newCount, originalCount);
            originalCount = newCount;
        }
        
        if (i % 10 == 0) {
            printf("Prošel %d položek...\n", i);
        }
    }
    
    int finalCount = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
    printf("=== ROZBALOVÁNÍ DOKONČENO - finální počet: %d ===\n", finalCount);
}

int main() {
    printf("=== TEST ROZBALOVÁNÍ STROMU ===\n");
    
    HWND hwnd = FindTwinCatWindow();
    if (!hwnd) {
        printf("TwinCAT okno nenalezeno!\n");
        return 1;
    }
    
    HWND hListBox = NULL;
    EnumChildWindows(hwnd, EnumChildProc, (LPARAM)&hListBox);
    
    if (!hListBox) {
        printf("ListBox nenalezen!\n");
        return 1;
    }
    
    ExpandAll(hwnd, hListBox);
    
    printf("Test dokončen. Zkontrolujte, zda se strom rozbalil.\n");
    return 0;
}
#include <windows.h>
#include <stdio.h>
#include <string.h>

char searchTitle[1024];

// Debug funkce pro výpis obsahu ListBoxu
void DebugListBoxItems(HWND hListBox, int maxItems) {
    int itemCount = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
    int limit = (maxItems < itemCount) ? maxItems : itemCount;
    
    printf("ListBox Handle: %p\n", hListBox);
    printf("Celkový počet položek: %d, zobrazuji prvních %d:\n", itemCount, limit);
    
    for (int i = 0; i < limit; i++) {
        char itemText[1024];
        int textLen = SendMessage(hListBox, LB_GETTEXT, i, (LPARAM)itemText);
        
        if (textLen > 0) {
            printf("[%d]: '%s'\n", i, itemText);
        } else {
            printf("[%d]: (prázdný nebo chyba - délka: %d)\n", i, textLen);
        }
    }
}

BOOL CALLBACK FindTwinCatWindow(HWND hwnd, LPARAM lParam) {
    GetWindowText(hwnd, searchTitle, 1024);
    
    if (strstr(searchTitle, "TwinCAT PLC Control") != NULL && strstr(searchTitle, "[") != NULL) {
        printf("Nalezeno okno: %s\n", searchTitle);
        
        // Najdeme všechny ListBoxy
        HWND hListBox1 = FindWindowEx(hwnd, NULL, "ListBox", NULL);
        HWND hListBox2 = FindWindowEx(hwnd, hListBox1, "ListBox", NULL);
        
        if (hListBox1) {
            printf("\n=== ListBox 1 ===\n");
            DebugListBoxItems(hListBox1, 10);
        }
        
        if (hListBox2) {
            printf("\n=== ListBox 2 ===\n");
            DebugListBoxItems(hListBox2, 10);
        }
        
        return FALSE; // Zastavit hledání
    }
    
    return TRUE;
}

int main() {
    printf("=== DEBUG TEST: OBSAH LISTBOXŮ ===\n");
    EnumWindows(FindTwinCatWindow, 0);
    
    printf("\nTest dokončen. Stiskněte Enter pro ukončení...\n");
    getchar();
    return 0;
}
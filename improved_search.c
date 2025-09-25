#include <windows.h>
#include <stdio.h>
#include <string.h>

// Funkce pro přímé čtení všech položek z ListBoxu
BOOL ReadAllListBoxItems(HWND hListBox, char searchText[]) {
    int itemCount = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
    printf("Celkový počet položek: %d\n", itemCount);
    
    for (int i = 0; i < itemCount; i++) {
        char itemText[1024];
        int textLen = SendMessage(hListBox, LB_GETTEXT, i, (LPARAM)itemText);
        
        if (textLen > 0) {
            printf("Položka %d: %s\n", i, itemText);
            
            // Porovnání s hledaným textem
            if (strstr(itemText, searchText) != NULL) {
                printf("NALEZENO na pozici %d: %s\n", i, itemText);
                // Nastavit výběr na tuto položku
                SendMessage(hListBox, LB_SETCURSEL, i, 0);
                return TRUE;
            }
        }
    }
    return FALSE;
}

// Funkce pro hierarchické procházení (pokud je ListBox ve stromové struktuře)
BOOL SearchInTreeStructure(HWND hListBox, char searchText[]) {
    int itemCount = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
    
    for (int i = 0; i < itemCount; i++) {
        char itemText[1024];
        SendMessage(hListBox, LB_GETTEXT, i, (LPARAM)itemText);
        
        // Zkontrolovat, zda obsahuje hledaný text
        if (strstr(itemText, searchText) != NULL) {
            // Nastavit focus na položku
            SendMessage(hListBox, LB_SETCURSEL, i, 0);
            
            // Poslat notifikaci, že byla položka vybrána
            HWND parentWnd = GetParent(hListBox);
            SendMessage(parentWnd, WM_COMMAND, 
                       MAKEWPARAM(GetDlgCtrlID(hListBox), LBN_SELCHANGE), 
                       (LPARAM)hListBox);
            
            return TRUE;
        }
    }
    return FALSE;
}
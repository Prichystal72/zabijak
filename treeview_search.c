#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

// Rekurzivní procházení TreeView uzlů
HTREEITEM FindTreeItemByText(HWND hTreeView, HTREEITEM hItem, const char* searchText) {
    TVITEM item;
    char itemText[1024];
    
    // Pokud hItem je NULL, začneme od kořene
    if (hItem == NULL) {
        hItem = (HTREEITEM)SendMessage(hTreeView, TVM_GETNEXTITEM, TVGN_ROOT, 0);
    }
    
    while (hItem != NULL) {
        // Získat text aktuálního uzlu
        item.mask = TVIF_TEXT | TVIF_HANDLE;
        item.hItem = hItem;
        item.pszText = itemText;
        item.cchTextMax = sizeof(itemText);
        
        if (SendMessage(hTreeView, TVM_GETITEM, 0, (LPARAM)&item)) {
            printf("Zkouším uzel: %s\n", itemText);
            
            if (strstr(itemText, searchText) != NULL) {
                printf("NALEZEN uzel: %s\n", itemText);
                return hItem;
            }
        }
        
        // Rekurzivně prohledat potomky
        HTREEITEM hChild = (HTREEITEM)SendMessage(hTreeView, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)hItem);
        if (hChild != NULL) {
            HTREEITEM found = FindTreeItemByText(hTreeView, hChild, searchText);
            if (found != NULL) return found;
        }
        
        // Přejít na další sourozenecký uzel
        hItem = (HTREEITEM)SendMessage(hTreeView, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hItem);
    }
    
    return NULL;
}

// Funkce pro výběr a rozbalení uzlu
BOOL SelectAndExpandTreeItem(HWND hTreeView, const char* searchText) {
    HTREEITEM hFoundItem = FindTreeItemByText(hTreeView, NULL, searchText);
    
    if (hFoundItem != NULL) {
        // Rozbalit uzel
        SendMessage(hTreeView, TVM_EXPAND, TVE_EXPAND, (LPARAM)hFoundItem);
        
        // Vybrat uzel
        SendMessage(hTreeView, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hFoundItem);
        
        // Zajistit viditelnost
        SendMessage(hTreeView, TVM_ENSUREVISIBLE, 0, (LPARAM)hFoundItem);
        
        return TRUE;
    }
    
    return FALSE;
}
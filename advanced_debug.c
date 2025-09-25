#include <windows.h>
#include <stdio.h>
#include <string.h>

char searchTitle[1024];

// Debug funkce pro analýzu konkrétního ListBoxu
void AnalyzeListBox(HWND hListBox, const char* name) {
    int itemCount = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
    
    printf("\n=== %s ===\n", name);
    printf("Handle: %p\n", hListBox);
    printf("Celkový počet položek: %d\n", itemCount);
    
    // Zobrazit všechny položky
    for (int i = 0; i < itemCount; i++) {
        char itemText[1024];
        int textLen = SendMessage(hListBox, LB_GETTEXT, i, (LPARAM)itemText);
        
        if (textLen > 0) {
            printf("[%d]: '%s'\n", i, itemText);
        } else {
            printf("[%d]: (chyba - délka: %d)\n", i, textLen);
        }
    }
}

// Test vyhledávání v konkrétním ListBoxu
void TestSearchInListBox(HWND hListBox, const char* targetUnit, const char* name) {
    printf("\n=== TEST VYHLEDÁVÁNÍ V %s ===\n", name);
    printf("Hledám: '%s'\n", targetUnit);
    
    int itemCount = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
    BOOL found = FALSE;
    
    for (int i = 0; i < itemCount; i++) {
        char itemText[1024];
        int textLen = SendMessage(hListBox, LB_GETTEXT, i, (LPARAM)itemText);
        
        if (textLen > 0) {
            // Test různých způsobů vyhledávání
            if (strstr(itemText, targetUnit) != NULL) {
                printf("*** NALEZENO na pozici %d: '%s' ***\n", i, itemText);
                found = TRUE;
            }
        }
    }
    
    if (!found) {
        printf("Jednotka '%s' nebyla nalezena.\n", targetUnit);
    }
}

BOOL CALLBACK FindTwinCatWindow(HWND hwnd, LPARAM lParam) {
    GetWindowText(hwnd, searchTitle, 1024);
    
    if (strstr(searchTitle, "TwinCAT PLC Control") != NULL && strstr(searchTitle, "[") != NULL) {
        printf("Nalezeno okno: %s\n", searchTitle);
        
        // Extrahujeme cílovou jednotku
        char *start = strchr(searchTitle, '[');
        char *end = strchr(searchTitle, ']');
        
        if (start != NULL && end != NULL && end > start) {
            char targetUnit[256];
            strncpy(targetUnit, start + 1, end - start - 1);
            targetUnit[end - start - 1] = '\0';
            printf("Cílová jednotka: '%s'\n", targetUnit);
            
            // Najdeme ListBoxy
            HWND hListBox1 = FindWindowEx(hwnd, NULL, "ListBox", NULL);
            HWND hListBox2 = FindWindowEx(hwnd, hListBox1, "ListBox", NULL);
            
            if (hListBox1) {
                AnalyzeListBox(hListBox1, "ListBox 1");
                TestSearchInListBox(hListBox1, targetUnit, "ListBox 1");
            }
            
            if (hListBox2) {
                AnalyzeListBox(hListBox2, "ListBox 2 (jen prvních 5)");
                // Pro ListBox 2 zobrazit jen prvních 5 kvůli poškozeným datům
                int itemCount = SendMessage(hListBox2, LB_GETCOUNT, 0, 0);
                if (itemCount > 5) {
                    printf("(Zobrazeno jen prvních 5 z %d položek kvůli poškozeným datům)\n", itemCount);
                }
                TestSearchInListBox(hListBox2, targetUnit, "ListBox 2");
            }
        }
        
        return FALSE; // Zastavit hledání
    }
    
    return TRUE;
}

int main() {
    printf("=== ROZŠÍŘENÝ DEBUG TEST ===\n");
    EnumWindows(FindTwinCatWindow, 0);
    
    printf("\nTest dokončen. Stiskněte Enter pro ukončení...\n");
    getchar();
    return 0;
}
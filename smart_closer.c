#include <windows.h>
#include <stdio.h>
#include <string.h>

char currentUnit[256];
HWND mainWindow;
int closedWindows = 0;

// Funkce pro extrakci názvu jednotky z titulku okna
BOOL ExtractUnitFromTitle(const char* title, char* unit, int maxLen) {
    char *start = strchr(title, '[');
    char *end = strchr(title, ']');
    
    if (start != NULL && end != NULL && end > start) {
        int len = end - start - 1;
        if (len < maxLen) {
            strncpy(unit, start + 1, len);
            unit[len] = '\0';
            return TRUE;
        }
    }
    return FALSE;
}

// Callback pro hledání a zavírání editor oken
BOOL CALLBACK CloseEditorWindows(HWND hwnd, LPARAM lParam) {
    char className[256];
    char windowText[1024];
    
    GetClassName(hwnd, className, sizeof(className));
    GetWindowText(hwnd, windowText, sizeof(windowText));
    
    // Hledáme CoDeSys_DoubleEd okna
    if (strcmp(className, "CoDeSys_DoubleEd") == 0) {
        printf("Nalezeno editor okno: '%s'\n", windowText);
        
        // Porovnáme s aktuální jednotkou
        if (strcmp(windowText, currentUnit) != 0) {
            printf("  -> ZAVÍRÁM (neodpovídá aktuální jednotce '%s')\n", currentUnit);
            
            // Zavřít okno
            SendMessage(hwnd, WM_CLOSE, 0, 0);
            closedWindows++;
        } else {
            printf("  -> PONECHÁVÁM (odpovídá aktuální jednotce)\n");
        }
    }
    
    return TRUE;
}

// Callback pro hledání TwinCAT okna
BOOL CALLBACK FindTwinCatWindow(HWND hwnd, LPARAM lParam) {
    char title[1024];
    GetWindowText(hwnd, title, sizeof(title));
    
    if (strstr(title, "TwinCAT PLC Control") != NULL && strstr(title, "[") != NULL) {
        printf("=== NALEZENO TWINCAT OKNO ===\n");
        printf("Titulek: %s\n", title);
        
        mainWindow = hwnd;
        
        // Extrahuj aktuální jednotku
        if (ExtractUnitFromTitle(title, currentUnit, sizeof(currentUnit))) {
            printf("Aktuální jednotka: '%s'\n", currentUnit);
            
            printf("\n=== HLEDÁNÍ A ZAVÍRÁNÍ EDITOR OKEN ===\n");
            closedWindows = 0;
            
            // Prohledat všechna child okna
            EnumChildWindows(hwnd, CloseEditorWindows, 0);
            
            printf("\n=== VÝSLEDEK ===\n");
            printf("Zavřeno oken: %d\n", closedWindows);
            
        } else {
            printf("CHYBA: Nepodařilo se extrahovat aktuální jednotku!\n");
        }
        
        return FALSE; // Zastavit hledání
    }
    
    return TRUE;
}

int main() {
    printf("=== NOVÝ PŘÍSTUP: PŘÍMÉ ZAVÍRÁNÍ EDITOR OKEN ===\n");
    printf("Tento program najde aktuálně editovanou jednotku a zavře\n");
    printf("všechna ostatní otevřená editor okna.\n\n");
    
    EnumWindows(FindTwinCatWindow, 0);
    
    printf("\nProgram dokončen. Stiskněte Enter pro ukončení...\n");
    getchar();
    return 0;
}
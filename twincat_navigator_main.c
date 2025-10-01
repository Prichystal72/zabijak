// TwinCAT Smart Navigator - Clean Version
// Uses external path finder module

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commctrl.h>
#include "lib/twincat_navigator.h"
#include "twincat_project_parser.h"
#include "twincat_path_finder.h"

#define MAX_PATH_LEN 512
#define MAX_POUS 1000

typedef struct {
    char name[256];
    char type[64];
    int level;
    char parent[256];
} POUItem;

POUItem pous[MAX_POUS];
int pou_count = 0;

// Export complete project structure to file
BOOL ExportCompleteStructure(TCProjectParser* parser, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        printf("❌ Nelze vytvořit soubor %s\n", filename);
        return FALSE;
    }
    
    fprintf(f, "=== KOMPLETNÍ STRUKTURA PROJEKTU ===\n");
    fprintf(f, "Soubor: %s\n", parser->project_path);
    fprintf(f, "Celkem elementů: %d\n\n", tc_get_element_count(parser));
    
    int element_count = tc_get_element_count(parser);
    for (int i = 0; i < element_count; i++) {
        TCElement* element = &parser->element_list.elements[i];
        fprintf(f, "[%s] %s\n", tc_element_type_string(element->type), element->name);
        if (strlen(element->path) > 0) {
            fprintf(f, "   Cesta: %s\n", element->path);
        }
        fprintf(f, "   Pozice: %d\n", element->position);
        fprintf(f, "\n");
    }
    
    fclose(f);
    printf("✅ Kompletní struktura exportována do: %s\n", filename);
    return TRUE;
}

// Export current TreeView state to file using proper memory reading
BOOL ExportTreeViewState(HWND listbox, HWND twincat_window, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        printf("❌ Nelze vytvořit soubor %s\n", filename);
        return FALSE;
    }
    
    fprintf(f, "=== AKTUÁLNÍ STAV LISTBOX (SPRÁVNÁ METODA) ===\n");
    fprintf(f, "ListBox Handle: 0x%p\n", (void*)listbox);
    fprintf(f, "TwinCAT Window: 0x%p\n\n", (void*)twincat_window);
    
    // Get process handle
    DWORD pid;
    GetWindowThreadProcessId(twincat_window, &pid);
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, pid);
    
    if (!hProcess) {
        fprintf(f, "❌ Nelze otevřít proces PID: %d\n", pid);
        fclose(f);
        return FALSE;
    }
    
    // Get ListBox item count using proper method
    int item_count = (int)SendMessage(listbox, LB_GETCOUNT, 0, 0);
    fprintf(f, "Celkem položek v ListBox: %d\n\n", item_count);
    
    if (item_count == 0 || item_count == LB_ERR) {
        fprintf(f, "❌ ListBox je prázdný nebo chyba při čtení\n");
        CloseHandle(hProcess);
        fclose(f);
        return FALSE;
    }
    
    // Extract items using proper memory reading
    int extracted_count = 0;
    for (int i = 0; i < item_count && i < 100; i++) { // Limit to prevent issues
        TreeItem item;
        if (ExtractTreeItem(hProcess, listbox, i, &item)) {
            extracted_count++;
            fprintf(f, "%3d. %s\n", i + 1, item.text);
            fprintf(f, "     ItemData: 0x%p\n", (void*)item.itemData);
            fprintf(f, "     Úroveň: %d\n", item.level);
            if (item.parentIndex >= 0) {
                fprintf(f, "     Rodič: %d\n", item.parentIndex);
            }
            fprintf(f, "\n");
        } else {
            fprintf(f, "%3d. [CHYBA PRI ČTENÍ]\n\n", i + 1);
        }
    }
    
    fprintf(f, "Úspěšně načteno položek: %d / %d\n", extracted_count, item_count);
    
    CloseHandle(hProcess);
    fclose(f);
    printf("✅ Stav ListBox exportován do: %s (%d položek)\n", filename, extracted_count);
    return TRUE;
}

// Main smart navigation function
int main() {
    printf("=== TwinCAT Smart Navigator - Clean Version ===\n\n");
    
    // Step 1: Find TwinCAT window
    printf("🔍 Hledám TwinCAT okno...\n");
    HWND twincat_window = NULL;
    char filename[256] = "";
    
    HWND hwnd = NULL;
    while ((hwnd = FindWindowEx(NULL, hwnd, NULL, NULL)) != NULL) {
        char window_title[512];
        int title_len = GetWindowText(hwnd, window_title, sizeof(window_title));
        
        if (title_len > 0 && strstr(window_title, "TwinCAT")) {
            printf("   🪟 TwinCAT okno: '%s'\n", window_title);
            twincat_window = hwnd;
            
            // Extract filename from title
            char* pro_pos = strstr(window_title, ".pro");
            if (pro_pos) {
                char* start = pro_pos;
                while (start > window_title && *(start-1) != ' ' && *(start-1) != '-' && 
                       *(start-1) != '\\' && *(start-1) != '/') {
                    start--;
                }
                
                int len = (pro_pos + 4) - start;
                strncpy(filename, start, len);
                filename[len] = '\0';
                printf("   📄 Název souboru: '%s'\n", filename);
                break;
            }
        }
    }
    
    if (!twincat_window) {
        printf("❌ TwinCAT okno nenalezeno\n");
        return 1;
    }
    
    // Step 2: Find project file path using external module
    printf("\n🔍 Hledám cestu k projektu...\n");
    char project_path[MAX_PATH_LEN];
    
    if (!FindTwinCATProjectPath(filename, project_path, sizeof(project_path))) {
        printf("❌ Nelze najít cestu k projektu\n");
        return 1;
    }
    
    printf("✅ Projekt nalezen: %s\n", project_path);
    
    // Step 3: Parse project structure
    printf("\n📋 Parsuju strukturu projektu...\n");
    TCProjectParser* parser = tc_parser_create();
    
    if (!parser) {
        printf("❌ Nelze vytvořit parser\n");
        return 1;
    }
    
    if (!tc_parser_load_project(parser, project_path)) {
        printf("❌ Nelze načíst projekt\n");
        tc_parser_destroy(parser);
        return 1;
    }
    
    if (!tc_parser_parse(parser)) {
        printf("❌ Chyba při parsování projektu\n");
        tc_parser_destroy(parser);
        return 1;
    }
    
    int element_count = tc_get_element_count(parser);
    printf("✅ Projekt naparsován: %d elementů\n", element_count);
    
    // Step 4: Find ListBox window in TwinCAT (not TreeView!)
    printf("\n📋 Hledám ListBox okno...\n");
    HWND listbox = FindProjectListBox(twincat_window);
    
    if (!listbox) {
        printf("❌ ListBox nenalezen\n");
        tc_parser_destroy(parser);
        return 1;
    }
    
    printf("✅ ListBox nalezen: 0x%p\n", (void*)listbox);
    
    // Export structures to text files
    printf("\n📝 Exportuji struktury do souborů...\n");
    
    // 1. Complete structure from file
    char complete_file[MAX_PATH];
    snprintf(complete_file, sizeof(complete_file), "%s_complete_structure.txt", filename);
    ExportCompleteStructure(parser, complete_file);
    
    // 2. Current ListBox state using proper memory reading
    char listbox_file[MAX_PATH];
    snprintf(listbox_file, sizeof(listbox_file), "%s_listbox_state.txt", filename);
    ExportTreeViewState(listbox, twincat_window, listbox_file);
    
    // Step 5: Display menu for navigation
    printf("\n🎯 === SMART NAVIGATION MENU ===\n");
    printf("Dostupné elementy pro navigaci:\n\n");
    
    int display_count = element_count < 20 ? element_count : 20;
    
    for (int i = 0; i < display_count; i++) {
        TCElement* element = &parser->element_list.elements[i];
        printf("%2d. [%s] %s\n", i + 1, 
               tc_element_type_string(element->type), 
               element->name);
    }
    
    if (element_count > 20) {
        printf("... a dalších %d elementů\n", element_count - 20);
    }
    
    printf("\nZadejte číslo elementu pro navigaci (0 = konec): ");
    
    int choice;
    if (scanf("%d", &choice) == 1 && choice > 0 && choice <= element_count) {
        TCElement* target = &parser->element_list.elements[choice - 1];
        
        printf("\n🎯 Navigujeme na: [%s] %s\n", 
               tc_element_type_string(target->type), target->name);
        
        // Step 6: Perform smart navigation - find item index first
        // For now, just use choice-1 as index
        if (FocusOnItem(listbox, choice - 1)) {
            printf("✅ Navigace úspěšná!\n");
        } else {
            printf("❌ Navigace se nezdařila\n");
        }
    }
    
    // Cleanup
    tc_parser_destroy(parser);
    
    printf("\nStiskněte Enter pro ukončení...");
    getchar(); // Clear input buffer
    getchar(); // Wait for Enter
    
    return 0;
}
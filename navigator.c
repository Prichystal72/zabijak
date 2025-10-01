#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <psapi.h>
#include "lib/twincat_navigator.h"
#include "twincat_project_parser.h"
#include "twincat_path_finder.h"

#define MAX_PATH_LEN 512
#define MAX_LINE_LEN 2048
#define MAX_POUS 1000

typedef struct {
    char name[256];
    char type[64];
    int level;
    char parent[256];
} POUItem;

POUItem pous[MAX_POUS];
int pou_count = 0;

// Hledani cesty k souboru v pameti procesu - RYCHLA VERZE
BOOL FindFilePathInMemory(HANDLE hProcess, const char* filename, char* full_path) {
    MEMORY_BASIC_INFORMATION mbi;
    char* addr = 0;
    char buffer[4096]; // Mensi buffer pro rychlost
    int regions_scanned = 0;
    const int MAX_REGIONS = 1000; // Zvětšeno pro větší pokrytí
    
    printf("Hledam cestu k souboru '%s' v pameti procesu (rychla verze, max %d oblasti)...\n", filename, MAX_REGIONS);
    
    // Pripravime ruzne varianty nazvu souboru
    char filename_no_ext[256];
    strcpy(filename_no_ext, filename);
    char* dot = strrchr(filename_no_ext, '.');
    if (dot) *dot = '\0'; // Odstranime priponu
    
    int found_paths = 0;
    
    while (VirtualQueryEx(hProcess, addr, &mbi, sizeof(mbi)) && regions_scanned < MAX_REGIONS) {
        regions_scanned++;
        
        // Debug: ukáž pokrok každých 100 oblastí
        if (regions_scanned % 100 == 0) {
            printf("    -> Oblast %d: adresa 0x%p\n", regions_scanned, mbi.BaseAddress);
        }
        
        if (mbi.State == MEM_COMMIT && 
            (mbi.Protect & PAGE_READWRITE || mbi.Protect & PAGE_READONLY)) {
            
            SIZE_T bytesRead;
            if (ReadProcessMemory(hProcess, mbi.BaseAddress, buffer, 
                                 min(sizeof(buffer), mbi.RegionSize), &bytesRead)) {
                
                // METODA 1: Hledame ASCII stringy obsahujici nazev souboru
                for (SIZE_T i = 0; i < bytesRead - 10; i++) {
                    // Kontrola ASCII textu (printable znaky)
                    if (buffer[i] >= 32 && buffer[i] <= 126) {
                        // Najdeme delku ASCII stringu
                        SIZE_T str_len = 0;
                        while (i + str_len < bytesRead && 
                               buffer[i + str_len] >= 32 && buffer[i + str_len] <= 126 &&
                               str_len < 500) {
                            str_len++;
                        }
                        
                        if (str_len > 10) { // Zajimave jen delsi stringy
                            char ascii_string[501];
                            strncpy(ascii_string, &buffer[i], str_len);
                            ascii_string[str_len] = '\0';
                            
                            // Debug: ukáž .pro stringy s cestou
                            if (strstr(ascii_string, ".pro") && 
                                (strstr(ascii_string, ":\\") || strstr(ascii_string, "/"))) {
                                printf("    -> Debug cesta .pro: '%s'\n", ascii_string);
                                found_paths++;
                            }
                            
                            // Kontrola zda obsahuje nas soubor nebo jeho cast
                            if (strstr(ascii_string, filename) || 
                                strstr(ascii_string, filename_no_ext) ||
                                (strstr(ascii_string, ".pro") && 
                                 (strstr(ascii_string, ":\\") || strstr(ascii_string, "/")))) {
                                
                                printf("    -> ASCII kandidat %d: '%s'\n", ++found_paths, ascii_string);
                                
                                // Pokusime se vyextrahovat cistou cestu
                                char clean_path[MAX_PATH_LEN];
                                char* path_start = strstr(ascii_string, ":\\");
                                if (path_start) {
                                    // Jdeme zpet a najdeme drive letter
                                    while (path_start > ascii_string && *(path_start-1) >= 'A' && *(path_start-1) <= 'Z') {
                                        path_start--;
                                    }
                                    strcpy(clean_path, path_start);
                                } else {
                                    strcpy(clean_path, ascii_string);
                                }
                                
                                // Test existence
                                FILE* test = fopen(clean_path, "r");
                                if (test) {
                                    fclose(test);
                                    printf("    -> *** PLATNA ASCII CESTA: '%s' ***\n", clean_path);
                                    strcpy(full_path, clean_path);
                                    return TRUE;
                                } else if (strcmp(clean_path, ascii_string) != 0) {
                                    // Zkusime i puvodni cestu
                                    test = fopen(ascii_string, "r");
                                    if (test) {
                                        fclose(test);
                                        printf("    -> *** PLATNA ASCII CESTA: '%s' ***\n", ascii_string);
                                        strcpy(full_path, ascii_string);
                                        return TRUE;
                                    }
                                }
                            }
                        }
                        i += str_len; // Preskocime zpracovany string
                    }
                }
                
                // METODA 2: Hledame Unicode stringy (kazdy druhy byte = 0)
                for (SIZE_T i = 0; i < bytesRead - 20; i += 2) {
                    if (buffer[i] >= 32 && buffer[i] <= 126 && buffer[i+1] == 0) {
                        // Mozny Unicode string
                        char unicode_string[501];
                        SIZE_T unicode_len = 0;
                        
                        while (i + unicode_len * 2 + 1 < bytesRead && 
                               buffer[i + unicode_len * 2] >= 32 && 
                               buffer[i + unicode_len * 2] <= 126 &&
                               buffer[i + unicode_len * 2 + 1] == 0 &&
                               unicode_len < 250) {
                            unicode_string[unicode_len] = buffer[i + unicode_len * 2];
                            unicode_len++;
                        }
                        unicode_string[unicode_len] = '\0';
                        
                        if (unicode_len > 10 && 
                            (strstr(unicode_string, filename) || 
                             strstr(unicode_string, filename_no_ext) ||
                             (strstr(unicode_string, ".pro") && strstr(unicode_string, ":\\")))) {
                            
                            printf("    -> Unicode kandidat %d: '%s'\n", ++found_paths, unicode_string);
                            
                            FILE* test = fopen(unicode_string, "r");
                            if (test) {
                                fclose(test);
                                printf("    -> *** PLATNA UNICODE CESTA: '%s' ***\n", unicode_string);
                                strcpy(full_path, unicode_string);
                                return TRUE;
                            }
                        }
                        i += unicode_len * 2; // Preskocime Unicode string
                    }
                }
            }
        }
        
        addr = (char*)mbi.BaseAddress + mbi.RegionSize;
    }
    
    printf("    -> Celkem nalezeno %d kandidatu, zadny neni platny\n", found_paths);
    return FALSE;
}

// Ziskani cesty k aktualnimu souboru z TwinCAT okna a pameti
BOOL GetTwinCATCurrentFile(char* filepath) {
    HWND hwnd = NULL;
    HWND twincat_window = NULL;
    int window_count = 0;
    char filename[256] = "";
    
    printf("Hledam TwinCAT okno...\n");
    
    // Najdeme TwinCAT okno
    while ((hwnd = FindWindowEx(NULL, hwnd, NULL, NULL)) != NULL) {
        char window_title[512];
        int title_len = GetWindowText(hwnd, window_title, sizeof(window_title));
        
        if (title_len > 0 && strstr(window_title, "TwinCAT")) {
            printf("    -> TwinCAT okno: '%s'\n", window_title);
            twincat_window = hwnd;
            
            // Extrahujeme nazev souboru z titulku
            char* pro_pos = strstr(window_title, ".pro");
            char* tsproj_pos = strstr(window_title, ".tsproj");
            
            if (pro_pos || tsproj_pos) {
                char* file_pos = pro_pos ? pro_pos : tsproj_pos;
                char* start = file_pos;
                
                // Jdeme zpet k zacatku nazvu souboru
                while (start > window_title && *(start-1) != ' ' && *(start-1) != '-' && 
                       *(start-1) != '\\' && *(start-1) != '/') {
                    start--;
                }
                
                // Zkopirujeme nazev souboru
                char* end = pro_pos ? pro_pos + 4 : tsproj_pos + 7;
                int len = end - start;
                strncpy(filename, start, len);
                filename[len] = '\0';
                
                printf("    -> Nazev souboru: '%s'\n", filename);
                break;
            }
        }
        
        window_count++;
        if (window_count > 100) break; // Omezeni
    }
    
    if (strlen(filename) == 0) {
        printf("Nazev souboru nenalezen v titulku okna.\n");
        return FALSE;
    }
    
    // Ziskame PID procesu TwinCAT okna
    DWORD pid;
    GetWindowThreadProcessId(twincat_window, &pid);
    printf("TwinCAT PID: %d\n", pid);
    
    // Otevreme proces pro cteni pameti
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProcess) {
        printf("Nelze otevrit TwinCAT proces pro cteni pameti.\n");
        return FALSE;
    }
    
    // Hledame cestu v pameti
    BOOL found = FindFilePathInMemory(hProcess, filename, filepath);
    
    CloseHandle(hProcess);
    return found;
}

// Urceni urovne vnoreni na zaklade odsazeni
int GetIndentationLevel(const char* line) {
    int level = 0;
    for (int i = 0; line[i] == ' ' || line[i] == '\t'; i++) {
        if (line[i] == '\t') level += 4;
        else level++;
    }
    return level / 2; // predpokladame 2 mezery na uroven
}

// Extrakce nazvu a typu z radku
void ExtractNameAndType(const char* line, char* name, char* type) {
    char clean_line[1024];
    
    // Odstraneni leading whitespace
    const char* start = line;
    while (*start == ' ' || *start == '\t') start++;
    
    strcpy(clean_line, start);
    
    // Odstraneni trailing whitespace a newline
    char* end = clean_line + strlen(clean_line) - 1;
    while (end >= clean_line && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
    
    // Hledani typu v zavorkach
    char* bracket_start = strchr(clean_line, '(');
    char* bracket_end = strchr(clean_line, ')');
    
    if (bracket_start && bracket_end && bracket_end > bracket_start) {
        // Extrakce nazvu (pred zavorkou)
        strncpy(name, clean_line, bracket_start - clean_line);
        name[bracket_start - clean_line] = '\0';
        
        // Trim nazev
        end = name + strlen(name) - 1;
        while (end >= name && (*end == ' ' || *end == '\t')) {
            *end = '\0';
            end--;
        }
        
        // Extrakce typu (v zavorkach)
        strncpy(type, bracket_start + 1, bracket_end - bracket_start - 1);
        type[bracket_end - bracket_start - 1] = '\0';
    } else {
        // Zadne zavorky - cely radek je nazev
        strcpy(name, clean_line);
        strcpy(type, "folder");
    }
}

// Nalezeni rodice na zaklade urovne
void FindParent(int current_level, char* parent) {
    strcpy(parent, "");
    
    for (int i = pou_count - 1; i >= 0; i--) {
        if (pous[i].level < current_level) {
            strcpy(parent, pous[i].name);
            break;
        }
    }
}

// Kontrola, zda radek oznacuje konec POUs sekce
BOOL IsEndOfPOUsSection(const char* line, int current_level) {
    // Trim line
    char trimmed[MAX_LINE_LEN];
    const char* start = line;
    while (*start == ' ' || *start == '\t') start++;
    strcpy(trimmed, start);
    
    char* end = trimmed + strlen(trimmed) - 1;
    while (end >= trimmed && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
    
    // Prazdny radek na urovni 0 nebo nizsi = konec sekce
    if (strlen(trimmed) == 0 && current_level <= 0) {
        return TRUE;
    }
    
    // Radek zacinajici velkym pismenem bez odsazeni (nova sekce)
    if (current_level == 0 && strlen(trimmed) > 0 && 
        trimmed[0] >= 'A' && trimmed[0] <= 'Z' &&
        !strstr(trimmed, "(") && !strstr(trimmed, ")")) {
        // Ale ne pokud obsahuje POUs nebo zname klicove slova
        if (!strstr(trimmed, "POUs") && 
            !strstr(trimmed, "Program") && 
            !strstr(trimmed, "Function") &&
            !strstr(trimmed, "FB_") &&
            !strstr(trimmed, "ST_") &&
            !strstr(trimmed, "PRG_")) {
            return TRUE;
        }
    }
    
    return FALSE;
}

// Analyza binarniho souboru .pro pro POUs strukturu pomocí vyladěného parseru
BOOL AnalyzePOUsStructure(const char* filepath) {
    printf("🔍 Používám vyladěný TwinCAT parser...\n");
    
    // Vytvoř parser
    TCProjectParser* parser = tc_parser_create();
    if (!parser) {
        printf("❌ Nelze vytvořit parser\n");
        return FALSE;
    }
    
    // Načti projekt
    if (!tc_parser_load_project(parser, filepath)) {
        printf("❌ Nelze načíst projekt: %s\n", filepath);
        tc_parser_destroy(parser);
        return FALSE;
    }
    
    // Parsuj strukturu
    if (!tc_parser_parse(parser)) {
        printf("❌ Chyba při parsování projektu\n");
        tc_parser_destroy(parser);
        return FALSE;
    }
    
    // Převeď data z parseru do našeho formátu
    int element_count = tc_get_element_count(parser);
    pou_count = 0;
    
    printf("✅ Parser našel %d elementů, převádím do interního formátu...\n", element_count);
    
    for (int i = 0; i < element_count && pou_count < MAX_POUS; i++) {
        TCElement* elem = &parser->element_list.elements[i];
        if (!elem) continue;
        
        // Zkopíruj základní data
        strncpy(pous[pou_count].name, elem->name, sizeof(pous[pou_count].name) - 1);
        pous[pou_count].name[sizeof(pous[pou_count].name) - 1] = '\0';
        
        // Převeď typ
        switch (elem->type) {
            case TC_ELEMENT_FB:
                strcpy(pous[pou_count].type, "FB");
                break;
            case TC_ELEMENT_PRG:
                strcpy(pous[pou_count].type, "PRG");
                break;
            case TC_ELEMENT_FOLDER:
                strcpy(pous[pou_count].type, "folder");
                break;
            case TC_ELEMENT_ACTION:
                strcpy(pous[pou_count].type, "action");
                break;
            case TC_ELEMENT_STRUCT:
                strcpy(pous[pou_count].type, "struct");
                break;
            case TC_ELEMENT_ENUM:
                strcpy(pous[pou_count].type, "enum");
                break;
            default:
                strcpy(pous[pou_count].type, "item");
                break;
        }
        
        // Základní hierarchie (zjednodušeně)
        if (elem->is_folder) {
            strcpy(pous[pou_count].parent, "");
            pous[pou_count].level = 0;
        } else {
            strcpy(pous[pou_count].parent, "POUs");
            pous[pou_count].level = 1;
        }
        
        printf("    → Převeden: %s (%s)\n", pous[pou_count].name, pous[pou_count].type);
        pou_count++;
    }
    
    // Vyčisti parser
    tc_parser_destroy(parser);
    
}

// Zápis do XML souboru
BOOL WriteToXML(const char* output_file) {
    FILE* xml = fopen(output_file, "w");
    if (!xml) {
        printf("Nelze vytvorit XML soubor: %s\n", output_file);
        return FALSE;
    }
    
    fprintf(xml, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(xml, "<ProjectExplorer>\n");
    fprintf(xml, "  <POUs>\n");
    
    for (int i = 0; i < pou_count; i++) {
        fprintf(xml, "    <Item>\n");
        fprintf(xml, "      <Name>%s</Name>\n", pous[i].name);
        fprintf(xml, "      <Type>%s</Type>\n", pous[i].type);
        fprintf(xml, "      <Level>%d</Level>\n", pous[i].level);
        fprintf(xml, "      <Parent>%s</Parent>\n", pous[i].parent);
        fprintf(xml, "    </Item>\n");
    }
    
    fprintf(xml, "  </POUs>\n");
    fprintf(xml, "</ProjectExplorer>\n");
    
    fclose(xml);
    return TRUE;
}

// ===============================================
// NOVE FUNKCE PRO CHYTRY NAVIGATION
// ===============================================

// Struktura pro projekt hierarchii
typedef struct ProjectItem {
    char name[256];
    char type[64];
    char full_path[512];
    int level;
    struct ProjectItem* parent;
    struct ProjectItem* children[50];
    int child_count;
} ProjectItem;

ProjectItem* project_root = NULL;
ProjectItem all_items[1000];
int total_items = 0;

// Nacteni cele struktury projektu z POUs dat
BOOL LoadProjectStructure() {
    printf("📁 Načítám celou strukturu projektu...\n");
    
    if (pou_count == 0) {
        printf("❌ Nejsou dostupná POUs data\n");
        return FALSE;
    }
    
    // Vytvorime root
    project_root = &all_items[0];
    strcpy(project_root->name, "POUs");
    strcpy(project_root->type, "folder");
    strcpy(project_root->full_path, "POUs");
    project_root->level = 0;
    project_root->parent = NULL;
    project_root->child_count = 0;
    total_items = 1;
    
    // Prevedeme pous[] array na hierarchicky strom
    for (int i = 0; i < pou_count; i++) {
        if (strcmp(pous[i].name, "POUs") == 0) continue; // Preskocime root
        
        ProjectItem* item = &all_items[total_items];
        strcpy(item->name, pous[i].name);
        strcpy(item->type, pous[i].type);
        item->level = pous[i].level;
        item->child_count = 0;
        
        // Najdeme parent
        item->parent = NULL;
        for (int j = 0; j < total_items; j++) {
            if (strcmp(all_items[j].name, pous[i].parent) == 0) {
                item->parent = &all_items[j];
                // Pridame do parent children
                if (all_items[j].child_count < 50) {
                    all_items[j].children[all_items[j].child_count] = item;
                    all_items[j].child_count++;
                }
                break;
            }
        }
        
        // Vytvorime full_path
        if (item->parent) {
            sprintf(item->full_path, "%s → %s", item->parent->full_path, item->name);
        } else {
            strcpy(item->full_path, item->name);
        }
        
        total_items++;
    }
    
    printf("✅ Načteno %d položek projektu\n", total_items);
    return TRUE;
}

// Najdi cilovou polozku podle nazvu z titulku okna
ProjectItem* FindTargetItem(const char* window_title) {
    printf("🎯 Hledám cílovou položku z titulku: '%s'\n", window_title);
    
    // Extrahuj nazev z titulku (napr. "[ST02_CallFBs (PRG-ST)]" → "ST02_CallFBs")
    char target_name[256] = "";
    char* bracket_start = strchr(window_title, '[');
    char* bracket_end = strchr(window_title, ']');
    
    if (bracket_start && bracket_end) {
        bracket_start++; // Preskocime [
        char* space_or_paren = strpbrk(bracket_start, " (");
        if (space_or_paren && space_or_paren < bracket_end) {
            int len = space_or_paren - bracket_start;
            strncpy(target_name, bracket_start, len);
            target_name[len] = '\0';
        }
    }
    
    if (strlen(target_name) == 0) {
        printf("❌ Nelze extrahovat název z titulku\n");
        return NULL;
    }
    
    printf("   → Hledám: '%s'\n", target_name);
    
    // Najdeme polozku ve structure
    for (int i = 0; i < total_items; i++) {
        if (strcmp(all_items[i].name, target_name) == 0) {
            printf("✅ Nalezeno: %s (cesta: %s)\n", all_items[i].name, all_items[i].full_path);
            return &all_items[i];
        }
    }
    
    printf("❌ Položka '%s' nenalezena ve struktuře projektu\n", target_name);
    return NULL;
}

// Vypis cesty k cilovemu objektu
void PrintNavigationPath(ProjectItem* target) {
    if (!target) return;
    
    printf("🚀 Cesta k objektu: %s\n", target->full_path);
    
    // Sestavime cestu od root k target
    ProjectItem* path[20];
    int path_length = 0;
    
    ProjectItem* current = target;
    while (current && path_length < 20) {
        path[path_length] = current;
        path_length++;
        current = current->parent;
    }
    
    // Obratme cestu (od root k target)
    printf("📍 Kroky k otevření:\n");
    for (int i = path_length - 1; i >= 0; i--) {
        printf("   %d. %s (%s)\n", path_length - i, path[i]->name, path[i]->type);
    }
}

// ===============================================
// FUNKCE PRO PRACI S DVA SEZNAMY SOUČASNĚ
// ===============================================

// Nacteni aktualniho stavu TreeView z RAM pameti
TreeItem current_tree[MAX_ITEMS];
int current_tree_count = 0;

BOOL LoadCurrentTreeFromRAM(HWND hTwinCAT) {
    printf("📚 Načítám aktuální stav TreeView z RAM paměti...\n");
    
    // Najdeme TreeView/ListBox
    HWND hListBox = FindProjectListBox(hTwinCAT);
    if (!hListBox) {
        printf("❌ TreeView/ListBox nenalezen\n");
        return FALSE;
    }
    
    // Otevreme proces pro cteni pameti
    HANDLE hProcess = OpenTwinCatProcess(hListBox);
    if (!hProcess) {
        printf("❌ Nelze otevřít TwinCAT proces\n");
        return FALSE;
    }
    
    // Ziskame pocet polozek
    current_tree_count = GetListBoxItemCount(hListBox);
    if (current_tree_count > MAX_ITEMS) current_tree_count = MAX_ITEMS;
    
    printf("   → Nalezeno %d položek v TreeView\n", current_tree_count);
    
    // Extrahujeme vsechny polozky
    int extracted = 0;
    for (int i = 0; i < current_tree_count; i++) {
        if (ExtractTreeItem(hProcess, hListBox, i, &current_tree[i])) {
            extracted++;
        }
    }
    
    CloseHandle(hProcess);
    
    printf("✅ Extrahováno %d/%d položek z TreeView\n", extracted, current_tree_count);
    return TRUE;
}

// ===============================================
// CHYTRÁ NAVIGAČNÍ FUNKCE
// ===============================================

// Chytra navigace k cilovemu objektu - klikne jen na potrebne slozky
BOOL SmartNavigateToTarget(ProjectItem* target, HWND hTwinCAT) {
    if (!target) {
        printf("❌ Žádný cílový objekt\n");
        return FALSE;
    }
    
    printf("\n🎯 === CHYTRÁ NAVIGACE === 🎯\n");
    printf("Cíl: %s\n", target->name);
    printf("Plná cesta: %s\n", target->full_path);
    
    // Najdeme TreeView/ListBox
    HWND hListBox = FindProjectListBox(hTwinCAT);
    if (!hListBox) {
        printf("❌ TreeView nenalezen\n");
        return FALSE;
    }
    
    // Otevreme proces
    HANDLE hProcess = OpenTwinCatProcess(hListBox);
    if (!hProcess) {
        printf("❌ Nelze otevřít proces\n");
        return FALSE;
    }
    
    // Rozdelime plnou cestu na kroky
    char path_copy[512];
    strcpy(path_copy, target->full_path);
    
    char* steps[10];
    int step_count = 0;
    
    char* token = strtok(path_copy, " → ");
    while (token && step_count < 10) {
        steps[step_count] = token;
        step_count++;
        token = strtok(NULL, " → ");
    }
    
    printf("📍 Kroky navigace (%d):\n", step_count);
    for (int i = 0; i < step_count; i++) {
        printf("   %d. %s\n", i + 1, steps[i]);
    }
    
    // Projdeme jednotlive kroky a rozbalneme potrebne slozky
    for (int step = 0; step < step_count - 1; step++) { // -1 protoze posledni je cil, ne slozka
        char* folder_name = steps[step];
        printf("\n🔍 Krok %d: Hledám složku '%s'\n", step + 1, folder_name);
        
        // Aktualizuj stav TreeView
        int current_count = GetListBoxItemCount(hListBox);
        printf("   → Aktuální počet položek: %d\n", current_count);
        
        // Najdi slozku v aktualnim stavu
        int folder_index = -1;
        for (int i = 0; i < current_count && i < MAX_ITEMS; i++) {
            TreeItem item;
            if (ExtractTreeItem(hProcess, hListBox, i, &item)) {
                // Kontrola shody nazvu (presne nebo obsahuje)
                if (strcmp(item.text, folder_name) == 0 || strstr(item.text, folder_name)) {
                    folder_index = i;
                    printf("   ✅ Složka nalezena na pozici %d: '%s'\n", i, item.text);
                    break;
                }
            }
        }
        
        if (folder_index == -1) {
            printf("   ❌ Složka '%s' není viditelná - možná je nadřazená složka nerozbalená\n", folder_name);
            CloseHandle(hProcess);
            return FALSE;
        }
        
        // Zkontroluj zda je slozka rozbalna (typ folder/special)
        TreeItem folder_item;
        if (!ExtractTreeItem(hProcess, hListBox, folder_index, &folder_item)) {
            printf("   ❌ Nelze načíst detaily složky\n");
            continue;
        }
        
        printf("   → Typ složky: %s, Flags: 0x%X\n", folder_item.type, folder_item.flags);
        
        // Pokud je to slozka, zkus ji rozbalit
        if (folder_item.flags == FLAG_FOLDER || folder_item.flags == FLAG_SPECIAL) {
            printf("   🔓 Rozbaluji složku '%s'...\n", folder_name);
            
            // Fokusuj na slozku
            if (!FocusOnItem(hListBox, folder_index)) {
                printf("   ❌ Nelze fokusovat na složku\n");
                continue;
            }
            
            // Aktivuj okno
            SetForegroundWindow(GetParent(hListBox));
            Sleep(100);
            
            // Kontrola pred rozbalenim
            int items_before = GetListBoxItemCount(hListBox);
            
            // Dvojklik pro rozbaleni
            SendMessage(hListBox, WM_LBUTTONDBLCLK, 0, MAKELPARAM(10, 10));
            Sleep(200); // Pockej na rozbaleni
            
            // Kontrola po rozbaleni
            int items_after = GetListBoxItemCount(hListBox);
            
            if (items_after > items_before) {
                printf("   ✅ Rozbaleno! (%d → %d položek)\n", items_before, items_after);
            } else {
                printf("   ⚠️ Možná už byla rozbalená nebo nemá podpoložky\n");
            }
        } else {
            printf("   → Není to rozbalitelná složka, pokračuji\n");
        }
    }
    
    // Finalni krok - najdi a fokusuj cilovy objekt
    printf("\n🎯 Finální krok: Hledám cílový objekt '%s'\n", target->name);
    
    int final_count = GetListBoxItemCount(hListBox);
    printf("   → Finální počet položek: %d\n", final_count);
    
    for (int i = 0; i < final_count && i < MAX_ITEMS; i++) {
        TreeItem item;
        if (ExtractTreeItem(hProcess, hListBox, i, &item)) {
            if (strstr(item.text, target->name)) {
                printf("   ✅ Cílový objekt nalezen na pozici %d: '%s'\n", i, item.text);
                
                // Fokusuj na cilovy objekt
                if (FocusOnItem(hListBox, i)) {
                    printf("   🎯 ÚSPĚCH: Fokusováno na cílový objekt!\n");
                    CloseHandle(hProcess);
                    return TRUE;
                } else {
                    printf("   ❌ Nelze fokusovat na cílový objekt\n");
                }
                break;
            }
        }
    }
    
    printf("   ❌ Cílový objekt nebyl nalezen ani po navigaci\n");
    CloseHandle(hProcess);
    return FALSE;
}

// Porovnani kompletniho seznamu ze souboru s aktualnim stavem z RAM
void CompareFileVsRAM(const char* target_name, HWND hTwinCAT) {
    printf("\n🔍 === POROVNÁNÍ SOUBOR vs RAM === 🔍\n");
    
    // Najdeme cilovy objekt v kompletnim seznamu (ze souboru)
    ProjectItem* file_target = NULL;
    for (int i = 0; i < total_items; i++) {
        if (strcmp(all_items[i].name, target_name) == 0) {
            file_target = &all_items[i];
            break;
        }
    }
    
    if (file_target) {
        printf("📁 SOUBOR: Objekt '%s' nalezen - cesta: %s\n", target_name, file_target->full_path);
    } else {
        printf("❌ SOUBOR: Objekt '%s' NENALEZEN v kompletní struktuře\n", target_name);
    }
    
    // Najdeme cilovy objekt v aktualnim TreeView (z RAM)
    TreeItem* ram_target = NULL;
    for (int i = 0; i < current_tree_count; i++) {
        if (strstr(current_tree[i].text, target_name)) {
            ram_target = &current_tree[i];
            break;
        }
    }
    
    if (ram_target) {
        printf("💾 RAM: Objekt '%s' VIDITELNÝ v TreeView na pozici %d\n", target_name, ram_target->index);
        printf("   → Text: '%s'\n", ram_target->text);
        printf("   → Typ: %s, Úroveň: %d\n", ram_target->type, ram_target->level);
    } else {
        printf("❌ RAM: Objekt '%s' NENÍ VIDITELNÝ v aktuálním TreeView\n", target_name);
        printf("   → Potřeba rozbalit složky pro zobrazení\n");
    }
    
    // Analyza co je potreba udelat
    if (file_target && !ram_target) {
        printf("\n🎯 AKCE: Objekt existuje, ale není viditelný - spouštím chytrou navigaci!\n");
        PrintNavigationPath(file_target);
        
        // Spustime chytrou navigaci
        printf("\n🚀 Spouštím chytrou navigaci...\n");
        if (SmartNavigateToTarget(file_target, hTwinCAT)) {
            printf("🎉 NAVIGACE ÚSPĚŠNÁ!\n");
        } else {
            printf("❌ Navigace selhala\n");
        }
    } else if (file_target && ram_target) {
        printf("\n✅ ÚSPĚCH: Objekt je dostupný a viditelný\n");
        printf("💡 Můžeme přímo fokusovat na pozici %d\n", ram_target->index);
    } else {
        printf("\n❌ PROBLÉM: Objekt nenalezen v kompletní struktuře\n");
    }
}

int main() {
    char filepath[MAX_PATH_LEN];
    char output_file[MAX_PATH_LEN];
    char window_title[512];
    
    printf("=== TwinCAT Smart Navigator ===\n\n");
    
    // Ziskani cesty k aktualnimu TwinCAT souboru (pouzijeme existujici funkci)
    if (GetTwinCATCurrentFile(filepath)) {
        printf("Nalezen TwinCAT soubor: %s\n", filepath);
        
        // Ziskame titulek okna znovu pro analyzu
        HWND hwnd = NULL;
        while ((hwnd = FindWindowEx(NULL, hwnd, NULL, NULL)) != NULL) {
            int title_len = GetWindowText(hwnd, window_title, sizeof(window_title));
            if (title_len > 0 && strstr(window_title, "TwinCAT")) {
                break;
            }
        }
        printf("📱 TwinCAT okno: %s\n", window_title);
    } else {
        printf("❌ TwinCAT okno nenalezeno\n");
        printf("Stisknete Enter pro ukonceni...");
        getchar();
        return 1;
    }
    
    // Finalni kontrola existence souboru
    FILE* test = fopen(filepath, "r");
    if (!test) {
        printf("CHYBA: Soubor neexistuje nebo neni pristupny: %s\n", filepath);
        printf("Stisknete Enter pro ukonceni...");
        getchar();
        return 1;
    }
    fclose(test);
    
    // Analyza POUs struktury
    if (!AnalyzePOUsStructure(filepath)) {
        printf("CHYBA: Nepodarilo se analyzovat POUs strukturu\n");
        printf("Stisknete Enter pro ukonceni...");
        getchar();
        return 1;
    }
    
    // === NOVA FUNKCIONALITA: DVA SEZNAMY SOUČASNĚ ===
    printf("\n🧠 === CHYTRÝ NAVIGATION S DVĚMA SEZNAMY === 🧠\n");
    
    // 1. KOMPLETNÍ STRUKTURA ZE SOUBORU
    printf("\n📁 === KOMPLETNÍ STRUKTURA ZE SOUBORU ===\n");
    if (!LoadProjectStructure()) {
        printf("❌ Chyba při načítání struktury ze souboru\n");
        getchar();
        return 1;
    }
    
    // 2. AKTUÁLNÍ STAV Z RAM PAMĚTI
    printf("\n💾 === AKTUÁLNÍ STAV Z RAM ===\n");
    HWND hTwinCAT = NULL;
    HWND hwnd = NULL;
    while ((hwnd = FindWindowEx(NULL, hwnd, NULL, NULL)) != NULL) {
        int title_len = GetWindowText(hwnd, window_title, sizeof(window_title));
        if (title_len > 0 && strstr(window_title, "TwinCAT")) {
            hTwinCAT = hwnd;
            break;
        }
    }
    
    if (!LoadCurrentTreeFromRAM(hTwinCAT)) {
        printf("❌ Chyba při načítání stromu z RAM\n");
        getchar();
        return 1;
    }
    
    // 3. EXTRAKCE CÍLOVÉHO OBJEKTU Z TITULKU
    char target_name[256] = "";
    if (!ExtractTargetFromTitle(window_title, target_name, sizeof(target_name))) {
        printf("❌ Nelze extrahovat cílový objekt z titulku\n");
        getchar();
        return 1;
    }
    
    // 4. POROVNÁNÍ A ANALÝZA
    CompareFileVsRAM(target_name, hTwinCAT);
    
    // Puvodni funkcionalita - XML zapis
    sprintf(output_file, "project_explorer_structure.xml");
    
    if (WriteToXML(output_file)) {
        printf("\n=== HOTOVO ===\n");
        printf("POUs struktura byla uspesne analyzovana a zapsana do: %s\n", output_file);
        printf("Nalezeno celkem %d polozek\n", pou_count);
    } else {
        printf("CHYBA: Nepodarilo se zapsat XML soubor\n");
    }
    
    printf("\nStisknete Enter pro ukonceni...");
    getchar();
    return 0;
}
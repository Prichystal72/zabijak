#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <psapi.h>

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

// Hledani cesty k souboru v pameti procesu
BOOL FindFilePathInMemory(HANDLE hProcess, const char* filename, char* full_path) {
    MEMORY_BASIC_INFORMATION mbi;
    char* addr = 0;
    char buffer[4096];
    
    printf("Hledam cestu k souboru '%s' v pameti procesu...\n", filename);
    
    while (VirtualQueryEx(hProcess, addr, &mbi, sizeof(mbi))) {
        if (mbi.State == MEM_COMMIT && 
            (mbi.Protect & PAGE_READWRITE || mbi.Protect & PAGE_READONLY)) {
            
            SIZE_T bytesRead;
            if (ReadProcessMemory(hProcess, mbi.BaseAddress, buffer, 
                                 min(sizeof(buffer), mbi.RegionSize), &bytesRead)) {
                
                // Hledame retezce obsahujici nazev souboru
                for (SIZE_T i = 0; i < bytesRead - strlen(filename); i++) {
                    if (strncmp(&buffer[i], filename, strlen(filename)) == 0) {
                        // Nasli jsme nazev souboru, najdeme zacatek cesty
                        SIZE_T start = i;
                        while (start > 0 && buffer[start-1] != '\0' && 
                               (buffer[start-1] == '\\' || buffer[start-1] == '/' || 
                                isalnum(buffer[start-1]) || buffer[start-1] == ':' || 
                                buffer[start-1] == '.' || buffer[start-1] == '_')) {
                            start--;
                        }
                        
                        // Najdeme konec cesty
                        SIZE_T end = i + strlen(filename);
                        while (end < bytesRead && buffer[end] != '\0' && buffer[end] != '\n' && buffer[end] != '\r') {
                            end++;
                        }
                        
                        // Zkopirujeme cestu
                        SIZE_T path_len = end - start;
                        if (path_len > 0 && path_len < MAX_PATH_LEN) {
                            strncpy(full_path, &buffer[start], path_len);
                            full_path[path_len] = '\0';
                            
                            // Overime ze to vypada jako validni cesta
                            if ((full_path[1] == ':' || full_path[0] == '\\') && 
                                (strstr(full_path, ".pro") || strstr(full_path, ".tsproj"))) {
                                printf("    -> Nalezena cesta: '%s'\n", full_path);
                                
                                // Test existence
                                FILE* test = fopen(full_path, "r");
                                if (test) {
                                    fclose(test);
                                    printf("    -> Cesta overena!\n");
                                    return TRUE;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        addr = (char*)mbi.BaseAddress + mbi.RegionSize;
    }
    
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

// Analyza binarniho souboru .pro pro POUs strukturu
BOOL AnalyzePOUsStructure(const char* filepath) {
    FILE* file = fopen(filepath, "rb"); // Binary mode
    if (!file) {
        printf("Nelze otevrit soubor: %s\n", filepath);
        return FALSE;
    }
    
    // Ziskame velikost souboru
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    printf("Analyzuji binarni soubor .pro (velikost: %ld bytes)...\n", file_size);
    
    // Nacteme soubor po chunkach
    char buffer[8192];
    size_t total_read = 0;
    BOOL found_pous = FALSE;
    
    // Vzory pro hledani
    char* search_patterns[] = {
        "POUs",
        "ST02_PRGs", 
        "FB_Schwenken",
        "ST02_CallFBs", 
        "PushData",
        "PROGRAM",
        "FUNCTION_BLOCK",
        "FUNCTION",
        NULL
    };
    
    while (!feof(file) && total_read < file_size) {
        size_t bytes_read = fread(buffer, 1, sizeof(buffer), file);
        if (bytes_read == 0) break;
        
        total_read += bytes_read;
        
        // Hledame textove retezce v binarnim obsahu
        for (size_t i = 0; i < bytes_read - 50; i++) {
            // Kontrola zda zacina ASCII text
            if (isalpha(buffer[i]) || buffer[i] == '_') {
                // Extrahujeme textovy retezec
                char extracted_text[256];
                size_t text_len = 0;
                
                // Cteme text dokud jsou validni znaky
                while (i + text_len < bytes_read && text_len < 255 &&
                       (isalnum(buffer[i + text_len]) || 
                        buffer[i + text_len] == '_' ||
                        buffer[i + text_len] == '-' ||
                        buffer[i + text_len] == '.' ||
                        buffer[i + text_len] == ' ')) {
                    extracted_text[text_len] = buffer[i + text_len];
                    text_len++;
                }
                extracted_text[text_len] = '\0';
                
                // Filtrujeme zajimave texty
                if (text_len > 3) {
                    for (int pattern_idx = 0; search_patterns[pattern_idx] != NULL; pattern_idx++) {
                        if (strstr(extracted_text, search_patterns[pattern_idx])) {
                            printf("    -> Nalezen text: '%s' na pozici %zu\n", extracted_text, total_read - bytes_read + i);
                            
                            // Zpracujeme nalezeny text
                            char clean_name[256];
                            strcpy(clean_name, extracted_text);
                            
                            // Ocistime nazev
                            char* space = strchr(clean_name, ' ');
                            if (space) *space = '\0';
                            
                            // Kontrola duplikatu
                            BOOL exists = FALSE;
                            for (int k = 0; k < pou_count; k++) {
                                if (strcmp(pous[k].name, clean_name) == 0) {
                                    exists = TRUE;
                                    break;
                                }
                            }
                            
                            if (!exists && pou_count < MAX_POUS && strlen(clean_name) > 3) {
                                strcpy(pous[pou_count].name, clean_name);
                                
                                // Urceni typu podle nazvu
                                if (strstr(clean_name, "FB_")) {
                                    strcpy(pous[pou_count].type, "FB");
                                } else if (strstr(clean_name, "CallFB") || strstr(clean_name, "_PRG")) {
                                    strcpy(pous[pou_count].type, "PRG");
                                } else if (strstr(clean_name, "Push") && strstr(clean_name, "Data")) {
                                    strcpy(pous[pou_count].type, "FUN");
                                } else if (strstr(clean_name, "POUs")) {
                                    strcpy(pous[pou_count].type, "folder");
                                    found_pous = TRUE;
                                } else if (strstr(clean_name, "ST02") && strstr(clean_name, "PRG")) {
                                    strcpy(pous[pou_count].type, "folder");
                                } else {
                                    strcpy(pous[pou_count].type, "item");
                                }
                                
                                // Jednoducha hierarchie
                                if (strstr(clean_name, "POUs")) {
                                    strcpy(pous[pou_count].parent, "");
                                    pous[pou_count].level = 0;
                                } else if (strstr(clean_name, "ST02_PRG")) {
                                    strcpy(pous[pou_count].parent, "POUs");
                                    pous[pou_count].level = 1;
                                } else {
                                    strcpy(pous[pou_count].parent, "ST02_PRGs");
                                    pous[pou_count].level = 2;
                                }
                                
                                printf("      -> Pridan: [%d] %s (%s) - parent: %s\n", 
                                       pous[pou_count].level, clean_name, 
                                       pous[pou_count].type, pous[pou_count].parent);
                                       
                                pou_count++;
                            }
                            break;
                        }
                    }
                }
                
                i += text_len; // Preskocime zpracovany text
            }
        }
        
        // Progress indikator
        if (total_read % 100000 == 0) {
            printf("    Zpracovano: %zu/%ld bytes (%.1f%%)\n", 
                   total_read, file_size, (float)total_read * 100 / file_size);
        }
    }
    
    fclose(file);
    
    if (!found_pous && pou_count == 0) {
        printf("VAROVANI: POUs struktura nebyla nalezena v souboru!\n");
        return FALSE;
    }
    
    printf("Analyzovano %d POUs polozek z binarniho souboru\n", pou_count);
    return TRUE;
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

int main() {
    char filepath[MAX_PATH_LEN];
    char output_file[MAX_PATH_LEN];
    
    printf("=== TwinCAT POUs Structure Analyzer ===\n\n");
    
    // Ziskani cesty k aktualnimu TwinCAT souboru
    if (GetTwinCATCurrentFile(filepath)) {
        printf("Nalezen TwinCAT soubor: %s\n", filepath);
        
        // Kontrola existence souboru
        FILE* test = fopen(filepath, "r");
        if (!test) {
            printf("Soubor neni dostupny na predpokladane ceste.\n");
            printf("Zadejte spravnou cestu k souboru %s: ", strrchr(filepath, '\\') ? strrchr(filepath, '\\') + 1 : filepath);
            
            char new_path[MAX_PATH_LEN];
            fgets(new_path, sizeof(new_path), stdin);
            
            // Odstraneni newline
            char* newline = strchr(new_path, '\n');
            if (newline) *newline = '\0';
            
            strcpy(filepath, new_path);
        } else {
            fclose(test);
        }
    } else {
        // Fallback - zeptame se uzivatele
        printf("TwinCAT okno nenalezeno. Zadejte cestu k souboru: ");
        fgets(filepath, sizeof(filepath), stdin);
        
        // Odstraneni newline
        char* newline = strchr(filepath, '\n');
        if (newline) *newline = '\0';
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
    
    // Vytvoreni nazvu vystupniho souboru
    sprintf(output_file, "project_explorer_structure.xml");
    
    // Zapis do XML
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
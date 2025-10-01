// twincat_project_parser.c
// Clean TwinCAT Project Parser Implementation
// Version: 1.0

#include "twincat_project_parser.h"

// ==============================================
// INTERNAL HELPER FUNCTIONS
// ==============================================

static BOOL tc_parser_parse_structured(TCProjectParser* parser, int pous_pos);
static BOOL tc_parser_parse_fallback(TCProjectParser* parser);

static int find_string_in_data(BYTE* data, DWORD size, const char* str, int start_pos) {
    int str_len = strlen(str);
    for (DWORD i = start_pos; i < size - str_len; i++) {
        if (memcmp(data + i, str, str_len) == 0) {
            return i;
        }
    }
    return -1;
}

static TCElementType determine_element_type(const char* name, BOOL is_folder) {
    if (is_folder) return TC_ELEMENT_FOLDER;
    
    if (strncmp(name, "FB_", 3) == 0 || strncmp(name, "fb_", 3) == 0) {
        return TC_ELEMENT_FB;
    }
    
    if (strncmp(name, "ST_", 3) == 0 || strstr(name, "_PRG") != NULL) {
        return TC_ELEMENT_PRG;
    }
    
    if (strncmp(name, "struct_", 7) == 0) {
        return TC_ELEMENT_STRUCT;
    }
    
    if (strncmp(name, "enum_", 5) == 0) {
        return TC_ELEMENT_ENUM;
    }
    
    // sData, sStation etc. are STRUCTs
    if (name[0] == 's' && strlen(name) > 1 && name[1] >= 'A' && name[1] <= 'Z') {
        return TC_ELEMENT_STRUCT;
    }
    
    // Check for Actions
    if (strstr(name, "action") != NULL || strstr(name, "Action") != NULL) {
        return TC_ELEMENT_ACTION;
    }
    
    return TC_ELEMENT_PRG; // default
}

// ==============================================
// PUBLIC API IMPLEMENTATION
// ==============================================

TCProjectParser* tc_parser_create(void) {
    TCProjectParser* parser = malloc(sizeof(TCProjectParser));
    if (!parser) return NULL;
    
    memset(parser, 0, sizeof(TCProjectParser));
    
    // Initialize element list
    parser->element_list.capacity = 4096;
    parser->element_list.elements = malloc(parser->element_list.capacity * sizeof(TCElement));
    parser->element_list.count = 0;
    
    if (!parser->element_list.elements) {
        free(parser);
        return NULL;
    }
    
    return parser;
}

void tc_parser_destroy(TCProjectParser* parser) {
    if (!parser) return;
    
    if (parser->file_data) {
        free(parser->file_data);
    }
    
    if (parser->element_list.elements) {
        free(parser->element_list.elements);
    }
    
    free(parser);
}

BOOL tc_parser_load_project(TCProjectParser* parser, const char* project_file) {
    if (!parser || !project_file) return FALSE;
    
    HANDLE hFile = CreateFileA(project_file, GENERIC_READ, FILE_SHARE_READ, 
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    
    parser->file_size = GetFileSize(hFile, NULL);
    if (parser->file_size == INVALID_FILE_SIZE) {
        CloseHandle(hFile);
        return FALSE;
    }
    
    parser->file_data = malloc(parser->file_size);
    if (!parser->file_data) {
        CloseHandle(hFile);
        return FALSE;
    }
    
    DWORD bytes_read;
    if (!ReadFile(hFile, parser->file_data, parser->file_size, &bytes_read, NULL) ||
        bytes_read != parser->file_size) {
        CloseHandle(hFile);
        free(parser->file_data);
        parser->file_data = NULL;
        return FALSE;
    }
    
    CloseHandle(hFile);
    
    // Store project path
    strncpy_s(parser->project_path, sizeof(parser->project_path), project_file, _TRUNCATE);
    
    return TRUE;
}

BOOL tc_parser_parse(TCProjectParser* parser) {
    if (!parser || !parser->file_data) return FALSE;
    
    // Find POUs position
    int pous_pos = find_string_in_data(parser->file_data, parser->file_size, "POUs", 0);
    
    if (pous_pos != -1) {
        // Mode 1: Použij strukturovaný parser (novější TwinCAT)
        printf("Detekovan novejsi TwinCAT format (s 'POUs'), pouzivam strukturovany parser\n");
        return tc_parser_parse_structured(parser, pous_pos);
    } else {
        // Mode 2: Použij fallback parser (starší TwinCAT)
        printf("Detekovan starsi TwinCAT format (bez 'POUs'), pouzivam fallback parser\n");
        return tc_parser_parse_fallback(parser);
    }
}

// Strukturovaný parser pro novější TwinCAT formát (s "POUs")
static BOOL tc_parser_parse_structured(TCProjectParser* parser, int pous_pos) {
    // Start 8 bytes before POUs
    int start_pos = (pous_pos >= 8) ? pous_pos - 8 : 0;
    BYTE* section = parser->file_data + start_pos;
    DWORD section_size = parser->file_size - start_pos;
    
    // Reset element count
    parser->element_list.count = 0;
    
    // Parse elements with metadata
    for (DWORD i = 8; i < section_size - 20; i++) {
        // Look for ASCII text (length 3-50 chars)
        int text_len = 0;
        
        while (text_len < 50 && 
               i + text_len < section_size &&
               section[i + text_len] >= 0x20 && 
               section[i + text_len] <= 0x7E) {
            text_len++;
        }
        
        if (text_len >= 3 && text_len <= 50) {
            // Check metadata 8 bytes before text
            BYTE* metadata = section + i - 8;
            
            // Pattern: [FLAG, 00, 01, 00, LENGTH, 00, 00, 00]
            if (metadata[1] == 0 && metadata[2] == 1 && metadata[3] == 0 &&
                metadata[4] == text_len && 
                metadata[5] == 0 && metadata[6] == 0 && metadata[7] == 0) {
                
                // Extract name
                char name[256];
                memcpy(name, section + i, text_len);
                name[text_len] = '\0';
                
                // Check for end of POUs structure
                if (strcmp(name, "Data types") == 0) {
                    break;
                }
                
                // Validate name
                if (strlen(name) >= 3 && 
                    strcmp(name, "POUs") != 0 &&
                    strstr(name, "..") == NULL) {
                    
                    BYTE flag = metadata[0];
                    BOOL is_folder = (flag == 1);
                    
                    // Add to element list
                    if (parser->element_list.count < parser->element_list.capacity) {
                        TCElement* elem = &parser->element_list.elements[parser->element_list.count];
                        strncpy_s(elem->name, sizeof(elem->name), name, _TRUNCATE);
                        elem->type = determine_element_type(name, is_folder);
                        elem->is_folder = is_folder;
                        elem->position = i;
                        
                        // Simple path (just name for now)
                        strncpy_s(elem->path, sizeof(elem->path), name, _TRUNCATE);
                        
                        parser->element_list.count++;
                    }
                }
            }
            
            // Skip found text
            i += text_len - 1;
        }
    }
    
    return TRUE;
}

// Fallback parser pro starší TwinCAT formát (bez "POUs")
static BOOL tc_parser_parse_fallback(TCProjectParser* parser) {
    printf("Implementuji fallback parser pro starsi TwinCAT format...\n");
    
    // Reset element count
    parser->element_list.count = 0;
    
    // Hledáme přímo textové vzory v celém souboru
    char* search_patterns[] = {
        "PROGRAM ",
        "FUNCTION_BLOCK ",
        "FUNCTION ",
        NULL
    };
    
    for (DWORD i = 0; i < parser->file_size - 100; i++) {
        // Kontrola na ASCII text
        if (parser->file_data[i] >= 0x20 && parser->file_data[i] <= 0x7E) {
            
            // Zkusíme najít pattern
            for (int p = 0; search_patterns[p] != NULL; p++) {
                int pattern_len = strlen(search_patterns[p]);
                
                if (i + pattern_len < parser->file_size &&
                    memcmp(parser->file_data + i, search_patterns[p], pattern_len) == 0) {
                    
                    // Našli jsme pattern, extrahujeme název
                    char name[256];
                    int name_start = i + pattern_len;
                    int name_len = 0;
                    
                    // Čteme název až do mezery nebo neplatného znaku
                    while (name_start + name_len < parser->file_size && 
                           name_len < 255 &&
                           ((parser->file_data[name_start + name_len] >= 'A' && 
                             parser->file_data[name_start + name_len] <= 'Z') ||
                            (parser->file_data[name_start + name_len] >= 'a' && 
                             parser->file_data[name_start + name_len] <= 'z') ||
                            (parser->file_data[name_start + name_len] >= '0' && 
                             parser->file_data[name_start + name_len] <= '9') ||
                            parser->file_data[name_start + name_len] == '_')) {
                        name_len++;
                    }
                    
                    if (name_len >= 3) {
                        memcpy(name, parser->file_data + name_start, name_len);
                        name[name_len] = '\0';
                        
                        // Kontrola duplikátů
                        BOOL exists = FALSE;
                        for (int k = 0; k < parser->element_list.count; k++) {
                            if (strcmp(parser->element_list.elements[k].name, name) == 0) {
                                exists = TRUE;
                                break;
                            }
                        }
                        
                        if (!exists && parser->element_list.count < parser->element_list.capacity) {
                            TCElement* elem = &parser->element_list.elements[parser->element_list.count];
                            strncpy_s(elem->name, sizeof(elem->name), name, _TRUNCATE);
                            
                            // Určení typu podle patternu
                            if (strstr(search_patterns[p], "PROGRAM")) {
                                elem->type = TC_ELEMENT_PRG;
                            } else if (strstr(search_patterns[p], "FUNCTION_BLOCK")) {
                                elem->type = TC_ELEMENT_FB;
                            } else {
                                elem->type = TC_ELEMENT_PRG; // default
                            }
                            
                            elem->is_folder = FALSE;
                            elem->position = i;
                            strncpy_s(elem->path, sizeof(elem->path), name, _TRUNCATE);
                            
                            parser->element_list.count++;
                            printf("   -> Nalezen: %s (%s)\n", name, 
                                   elem->type == TC_ELEMENT_PRG ? "PRG" : 
                                   elem->type == TC_ELEMENT_FB ? "FB" : "OTHER");
                        }
                    }
                    
                    i += pattern_len + name_len - 1; // Skip processed text
                    break;
                }
            }
        }
    }
    
    printf("Fallback parser nasel %d elementu\n", parser->element_list.count);
    return TRUE;
}

TCElement* tc_find_element(TCProjectParser* parser, const char* name) {
    if (!parser || !name) return NULL;
    
    for (int i = 0; i < parser->element_list.count; i++) {
        if (strcmp(parser->element_list.elements[i].name, name) == 0) {
            return &parser->element_list.elements[i];
        }
    }
    return NULL;
}

TCElement* tc_find_element_by_type(TCProjectParser* parser, const char* name, TCElementType type) {
    if (!parser || !name) return NULL;
    
    for (int i = 0; i < parser->element_list.count; i++) {
        TCElement* elem = &parser->element_list.elements[i];
        if (elem->type == type && strcmp(elem->name, name) == 0) {
            return elem;
        }
    }
    return NULL;
}

int tc_find_all_elements(TCProjectParser* parser, const char* name, TCElement** results, int max_results) {
    if (!parser || !name || !results) return 0;
    
    int count = 0;
    for (int i = 0; i < parser->element_list.count && count < max_results; i++) {
        if (strstr(parser->element_list.elements[i].name, name) != NULL) {
            results[count++] = &parser->element_list.elements[i];
        }
    }
    return count;
}

const char* tc_element_type_string(TCElementType type) {
    switch (type) {
        case TC_ELEMENT_FOLDER: return "";
        case TC_ELEMENT_FB: return " (FB)";
        case TC_ELEMENT_PRG: return " (PRG)";
        case TC_ELEMENT_ACTION: return " (ACTION)";
        case TC_ELEMENT_STRUCT: return " (STRUCT)";
        case TC_ELEMENT_ENUM: return " (ENUM)";
        default: return " (UNKNOWN)";
    }
}

int tc_get_element_count(TCProjectParser* parser) {
    return parser ? parser->element_list.count : 0;
}

BOOL tc_save_element_list(TCProjectParser* parser, const char* output_file) {
    if (!parser || !output_file) return FALSE;
    
    FILE* file = fopen(output_file, "w");
    if (!file) return FALSE;
    
    for (int i = 0; i < parser->element_list.count; i++) {
        TCElement* elem = &parser->element_list.elements[i];
        fprintf(file, "%s%s\n", elem->name, tc_element_type_string(elem->type));
    }
    
    fclose(file);
    return TRUE;
}

// Quick access functions
TCElement* tc_find_fb(TCProjectParser* parser, const char* fb_name) {
    return tc_find_element_by_type(parser, fb_name, TC_ELEMENT_FB);
}

TCElement* tc_find_prg(TCProjectParser* parser, const char* prg_name) {
    return tc_find_element_by_type(parser, prg_name, TC_ELEMENT_PRG);
}

TCElement* tc_find_action(TCProjectParser* parser, const char* action_name) {
    return tc_find_element_by_type(parser, action_name, TC_ELEMENT_ACTION);
}
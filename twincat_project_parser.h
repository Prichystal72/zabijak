// twincat_project_parser.h
// Clean TwinCAT Project Parser for TC2 Integration
// Version: 1.0

#ifndef TWINCAT_PROJECT_PARSER_H
#define TWINCAT_PROJECT_PARSER_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Element types
typedef enum {
    TC_ELEMENT_FOLDER = 0,
    TC_ELEMENT_FB = 1,
    TC_ELEMENT_PRG = 2,
    TC_ELEMENT_ACTION = 3,
    TC_ELEMENT_STRUCT = 4,
    TC_ELEMENT_ENUM = 5
} TCElementType;

// Simple element structure for fast searching
typedef struct {
    char name[256];                     // Element name
    char path[1024];                    // Full path (e.g. "POUs/pBasic/PLC_PRG")
    TCElementType type;                 // Element type
    BOOL is_folder;                     // Is folder flag
    int position;                       // Position in file
} TCElement;

// Element list for fast searching
typedef struct {
    TCElement* elements;                // Array of elements
    int count;                         // Number of elements
    int capacity;                      // Array capacity
} TCElementList;

// Main parser structure
typedef struct {
    BYTE* file_data;                   // File data
    DWORD file_size;                   // File size
    TCElementList element_list;        // Flat list for fast search
    char project_path[512];            // Project file path
} TCProjectParser;

// ==============================================
// CORE API FUNCTIONS
// ==============================================

// Parser lifecycle
TCProjectParser* tc_parser_create(void);
void tc_parser_destroy(TCProjectParser* parser);
BOOL tc_parser_load_project(TCProjectParser* parser, const char* project_file);
BOOL tc_parser_parse(TCProjectParser* parser);

// Element search functions
TCElement* tc_find_element(TCProjectParser* parser, const char* name);
TCElement* tc_find_element_by_type(TCProjectParser* parser, const char* name, TCElementType type);
int tc_find_all_elements(TCProjectParser* parser, const char* name, TCElement** results, int max_results);

// Utility functions
const char* tc_element_type_string(TCElementType type);
int tc_get_element_count(TCProjectParser* parser);
BOOL tc_save_element_list(TCProjectParser* parser, const char* output_file);

// Quick access functions for common use cases
TCElement* tc_find_fb(TCProjectParser* parser, const char* fb_name);
TCElement* tc_find_prg(TCProjectParser* parser, const char* prg_name);
TCElement* tc_find_action(TCProjectParser* parser, const char* action_name);

#endif // TWINCAT_PROJECT_PARSER_H
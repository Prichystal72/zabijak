# TwinCAT Project Parser - API Documentation

## Overview
Clean, fast parser for TwinCAT .pro files designed for integration into TC2 control programs.

## Key Features
- **Fast parsing**: ~299 elements in milliseconds
- **Simple API**: Easy integration into existing code
- **Memory efficient**: Flat list structure
- **Type detection**: FB, PRG, ACTION, STRUCT, ENUM
- **No dependencies**: Only standard Windows API

## Quick Start

### 1. Include files in your project:
```c
#include "twincat_project_parser.h"
```

### 2. Link with:
```
twincat_project_parser.c
```

### 3. Basic usage:
```c
// Create parser
TCProjectParser* parser = tc_parser_create();

// Load project
tc_parser_load_project(parser, "MyProject.pro");

// Parse structure
tc_parser_parse(parser);

// Search elements
TCElement* elem = tc_find_element(parser, "PLC_PRG");
if (elem) {
    printf("Found: %s%s\n", elem->name, tc_element_type_string(elem->type));
}

// Cleanup
tc_parser_destroy(parser);
```

## API Reference

### Core Functions

#### `TCProjectParser* tc_parser_create(void)`
Creates new parser instance.
- **Returns**: Parser instance or NULL on failure

#### `void tc_parser_destroy(TCProjectParser* parser)`
Destroys parser and frees memory.
- **Parameters**: parser - Parser instance

#### `BOOL tc_parser_load_project(TCProjectParser* parser, const char* project_file)`
Loads TwinCAT .pro file into memory.
- **Parameters**: 
  - parser - Parser instance
  - project_file - Path to .pro file
- **Returns**: TRUE on success, FALSE on failure

#### `BOOL tc_parser_parse(TCProjectParser* parser)`
Parses loaded project and builds element list.
- **Parameters**: parser - Parser instance
- **Returns**: TRUE on success, FALSE on failure

### Search Functions

#### `TCElement* tc_find_element(TCProjectParser* parser, const char* name)`
Finds first element by exact name match.
- **Returns**: Element pointer or NULL if not found

#### `TCElement* tc_find_fb(TCProjectParser* parser, const char* fb_name)`
Finds Function Block by name.

#### `TCElement* tc_find_prg(TCProjectParser* parser, const char* prg_name)`
Finds Program by name.

#### `TCElement* tc_find_action(TCProjectParser* parser, const char* action_name)`
Finds Action by name.

### Utility Functions

#### `int tc_get_element_count(TCProjectParser* parser)`
Returns total number of parsed elements.

#### `const char* tc_element_type_string(TCElementType type)`
Converts element type to string representation.

## Data Structures

### `TCElement`
```c
typedef struct {
    char name[256];          // Element name
    char path[1024];         // Full path  
    TCElementType type;      // Element type
    BOOL is_folder;          // Folder flag
    int position;            // File position
} TCElement;
```

### `TCElementType`
```c
typedef enum {
    TC_ELEMENT_FOLDER = 0,
    TC_ELEMENT_FB = 1,       // Function Block
    TC_ELEMENT_PRG = 2,      // Program
    TC_ELEMENT_ACTION = 3,   // Action
    TC_ELEMENT_STRUCT = 4,   // Structure
    TC_ELEMENT_ENUM = 5      // Enumeration
} TCElementType;
```

## Integration with TC2

### Example: Focus element in Project Explorer
```c
BOOL focus_element_in_tc2(const char* element_name) {
    TCElement* elem = tc_find_element(parser, element_name);
    if (!elem) return FALSE;
    
    // Your TC2 Project Explorer manipulation code here
    // Example: SendMessage to TreeView control
    
    return TRUE;
}
```

### Example: List all Function Blocks
```c
void list_all_fbs(TCProjectParser* parser) {
    for (int i = 0; i < parser->element_list.count; i++) {
        TCElement* elem = &parser->element_list.elements[i];
        if (elem->type == TC_ELEMENT_FB) {
            printf("FB: %s\n", elem->name);
        }
    }
}
```

## Performance Notes
- Parsing ~299 elements: < 100ms
- Memory usage: ~150KB for typical project
- Search time: O(n) linear search (fast enough for 299 elements)

## Error Handling
- All functions return BOOL (TRUE/FALSE) or NULL on failure
- Always check return values
- Clean up with `tc_parser_destroy()`
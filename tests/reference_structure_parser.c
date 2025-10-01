// Parser for CELA.EXP export files to create reference structure
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define MAX_PATH_LENGTH 512
#define MAX_NAME_LENGTH 256
#define MAX_ITEMS 1000

typedef struct {
    char path[MAX_PATH_LENGTH];
    char name[MAX_NAME_LENGTH];
    char type[32]; // PROGRAM, FUNCTION_BLOCK, TYPE, etc.
    int level;     // hierarchy level (0 = root)
} ReferenceItem;

typedef struct {
    ReferenceItem items[MAX_ITEMS];
    int count;
} ReferenceStructure;

// Parse path into hierarchy levels
int GetHierarchyLevel(const char* path) {
    if (strlen(path) == 0) return 0;
    
    int level = 0;
    for (int i = 0; path[i]; i++) {
        if (path[i] == '/') level++;
    }
    return level;
}

// Extract item name from declaration line
void ExtractItemName(const char* line, char* name, char* type) {
    name[0] = '\0';
    type[0] = '\0';
    
    // Look for patterns like "PROGRAM MainProgram", "FUNCTION_BLOCK MyFB", etc.
    if (strstr(line, "PROGRAM ")) {
        strcpy(type, "PRG");
        char* start = strstr(line, "PROGRAM ") + 8;
        char* end = strchr(start, '\n');
        if (!end) end = strchr(start, '\r');
        if (!end) end = start + strlen(start);
        
        int len = end - start;
        if (len > 0 && len < MAX_NAME_LENGTH - 1) {
            strncpy(name, start, len);
            name[len] = '\0';
            
            // Remove trailing whitespace including newlines
            while (len > 0 && (name[len-1] == ' ' || name[len-1] == '\t' || 
                              name[len-1] == '\n' || name[len-1] == '\r')) {
                name[--len] = '\0';
            }
        }
    }
    else if (strstr(line, "FUNCTION_BLOCK ")) {
        strcpy(type, "FB");
        char* start = strstr(line, "FUNCTION_BLOCK ") + 15;
        char* end = strchr(start, '\n');
        if (!end) end = strchr(start, '\r');
        if (!end) end = start + strlen(start);
        
        int len = end - start;
        if (len > 0 && len < MAX_NAME_LENGTH - 1) {
            strncpy(name, start, len);
            name[len] = '\0';
            
            // Remove trailing whitespace including newlines
            while (len > 0 && (name[len-1] == ' ' || name[len-1] == '\t' || 
                              name[len-1] == '\n' || name[len-1] == '\r')) {
                name[--len] = '\0';
            }
        }
    }
    else if (strstr(line, "FUNCTION ")) {
        strcpy(type, "FUN");
        char* start = strstr(line, "FUNCTION ") + 9;
        char* end = strchr(start, '\n');
        if (!end) end = strchr(start, '\r');
        if (!end) end = start + strlen(start);
        
        int len = end - start;
        if (len > 0 && len < MAX_NAME_LENGTH - 1) {
            strncpy(name, start, len);
            name[len] = '\0';
            
            // Remove trailing whitespace including newlines
            while (len > 0 && (name[len-1] == ' ' || name[len-1] == '\t' || 
                              name[len-1] == '\n' || name[len-1] == '\r')) {
                name[--len] = '\0';
            }
        }
    }
    else if (strstr(line, "TYPE ")) {
        strcpy(type, "DUT");
        char* start = strstr(line, "TYPE ") + 5;
        char* end = strchr(start, ':');
        if (!end) end = strchr(start, '\n');
        if (!end) end = strchr(start, '\r');
        if (!end) end = start + strlen(start);
        
        int len = end - start;
        if (len > 0 && len < MAX_NAME_LENGTH - 1) {
            strncpy(name, start, len);
            name[len] = '\0';
            
            // Remove trailing whitespace including newlines
            while (len > 0 && (name[len-1] == ' ' || name[len-1] == '\t' || 
                              name[len-1] == '\n' || name[len-1] == '\r')) {
                name[--len] = '\0';
            }
        }
    }
}

// Parse CELA.EXP and extract structure
ReferenceStructure* ParseCELAExport(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Cannot open file %s\n", filename);
        return NULL;
    }
    
    ReferenceStructure* ref = malloc(sizeof(ReferenceStructure));
    ref->count = 0;
    
    char line[1024];
    char current_path[MAX_PATH_LENGTH] = "";
    int in_declaration = 0;
    
    printf("=== Parsing CELA.EXP Export ===\n");
    
    while (fgets(line, sizeof(line), file)) {
        // Look for PATH declarations
        if (strstr(line, "@PATH := '")) {
            char* start = strstr(line, "@PATH := '") + 10;
            char* end = strstr(start, "'");
            if (end) {
                int len = end - start;
                if (len < MAX_PATH_LENGTH) {
                    strncpy(current_path, start, len);
                    current_path[len] = '\0';
                    
                    // Convert \/ to /
                    char* pos = current_path;
                    while ((pos = strstr(pos, "\\/")) != NULL) {
                        memmove(pos, pos + 1, strlen(pos));
                    }
                    
                    printf("Found PATH: '%s'\n", current_path);
                }
            }
        }
        
        // Look for item declarations
        char name[MAX_NAME_LENGTH];
        char type[32];
        ExtractItemName(line, name, type);
        
        // Only include executable POUs (PRG, FB, FUN), ignore data types (DUT)
        if (strlen(name) > 0 && ref->count < MAX_ITEMS && 
            (strcmp(type, "PRG") == 0 || strcmp(type, "FB") == 0 || strcmp(type, "FUN") == 0)) {
            strcpy(ref->items[ref->count].path, current_path);
            strcpy(ref->items[ref->count].name, name);
            strcpy(ref->items[ref->count].type, type);
            ref->items[ref->count].level = GetHierarchyLevel(current_path);
            
            printf("  -> Added POU: %s (%s) in '%s' (level %d)\n", 
                   name, type, current_path, ref->items[ref->count].level);
            
            ref->count++;
        } else if (strlen(name) > 0 && strcmp(type, "DUT") == 0) {
            printf("  -> Skipped DUT: %s in '%s'\n", name, current_path);
        }
    }
    
    fclose(file);
    printf("\nTotal items found: %d\n", ref->count);
    return ref;
}

// Save reference structure to XML
void SaveReferenceStructureXML(ReferenceStructure* ref, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        printf("Error: Cannot create file %s\n", filename);
        return;
    }
    
    fprintf(file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(file, "<TwinCATProject>\n");
    fprintf(file, "  <ReferenceStructure source=\"CELA.EXP\" pous_only=\"true\" items=\"%d\">\n", ref->count);
    
    for (int i = 0; i < ref->count; i++) {
        fprintf(file, "    <Item>\n");
        fprintf(file, "      <Name>%s</Name>\n", ref->items[i].name);
        fprintf(file, "      <Type>%s</Type>\n", ref->items[i].type);
        fprintf(file, "      <Path>%s</Path>\n", ref->items[i].path);
        fprintf(file, "      <Level>%d</Level>\n", ref->items[i].level);
        fprintf(file, "    </Item>\n");
    }
    
    fprintf(file, "  </ReferenceStructure>\n");
    fprintf(file, "</TwinCATProject>\n");
    
    fclose(file);
    printf("Reference structure saved to: %s\n", filename);
}

// Compare structures and show differences
void CompareStructures(ReferenceStructure* reference, ReferenceStructure* parsed, const char* parsed_source) {
    printf("\n=== COMPARISON: Reference vs %s ===\n", parsed_source);
    printf("Reference items: %d\n", reference->count);
    printf("Parsed items: %d\n", parsed->count);
    
    int matches = 0;
    int missing_in_parsed = 0;
    int extra_in_parsed = 0;
    
    // Check items in reference that are missing in parsed
    for (int i = 0; i < reference->count; i++) {
        int found = 0;
        for (int j = 0; j < parsed->count; j++) {
            if (strcmp(reference->items[i].name, parsed->items[j].name) == 0) {
                found = 1;
                matches++;
                break;
            }
        }
        
        if (!found) {
            printf("  MISSING: %s (%s) from path '%s'\n", 
                   reference->items[i].name, reference->items[i].type, reference->items[i].path);
            missing_in_parsed++;
        }
    }
    
    // Check items in parsed that are not in reference
    for (int i = 0; i < parsed->count; i++) {
        int found = 0;
        for (int j = 0; j < reference->count; j++) {
            if (strcmp(parsed->items[i].name, reference->items[j].name) == 0) {
                found = 1;
                break;
            }
        }
        
        if (!found) {
            printf("  EXTRA: %s (%s) found by parser\n", 
                   parsed->items[i].name, parsed->items[i].type);
            extra_in_parsed++;
        }
    }
    
    printf("\nSUMMARY:\n");
    printf("  Matches: %d\n", matches);
    printf("  Missing in parsed: %d\n", missing_in_parsed);
    printf("  Extra in parsed: %d\n", extra_in_parsed);
    printf("  Accuracy: %.1f%%\n", reference->count > 0 ? (100.0 * matches / reference->count) : 0.0);
}

int main() {
    printf("=== TwinCAT Reference Structure Parser ===\n\n");
    
    // Parse CELA.EXP to create reference structure
    ReferenceStructure* reference = ParseCELAExport("cela/CELA.EXP");
    if (!reference) {
        printf("Failed to parse CELA.EXP\n");
        return 1;
    }
    
    // Save reference structure
    SaveReferenceStructureXML(reference, "reference_structure_older_format.xml");
    
    printf("\nReference structure created successfully!\n");
    printf("Use this as baseline for comparing parser output.\n");
    
    free(reference);
    return 0;
}
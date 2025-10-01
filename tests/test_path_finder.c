// Test external path finder module
#include <stdio.h>
#include <windows.h>
#include "twincat_path_finder.h"

int main() {
    printf("=== TEST EXTERNAL PATH FINDER ===\n\n");
    
    char filename[] = "BA17xx.pro";
    char result_path[MAX_PATH];
    
    if (FindTwinCATProjectPath(filename, result_path, sizeof(result_path))) {
        printf("\n🎉 ÚSPĚCH: %s\n", result_path);
        
        // Test parsování souboru
        printf("\n📋 Test parsování...\n");
        FILE* f = fopen(result_path, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            printf("   📏 Velikost souboru: %ld bytes\n", size);
            fclose(f);
        }
        
        return 0;
    } else {
        printf("\n❌ NEÚSPĚCH: Soubor nenalezen\n");
        return 1;
    }
}
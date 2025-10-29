#include <windows.h>
#include <stdio.h>
#include "../lib/twincat_navigator.h"

/**
 * Test: Extrakce názvu programu z titulku TwinCAT okna
 */

void test_extraction(const char* title, const char* expected) {
    char result[256];
    bool success = ExtractTargetFromTitle(title, result, sizeof(result));
    
    printf("Input:    '%s'\n", title);
    printf("Expected: '%s'\n", expected);
    printf("Result:   '%s'\n", result);
    printf("Status:   %s\n", success && strcmp(result, expected) == 0 ? "✓ OK" : "✗ FAIL");
    printf("\n");
}

int main() {
    printf("===================================================\n");
    printf("  TEST: ExtractTargetFromTitle()\n");
    printf("===================================================\n\n");
    
    // Test 1: Projekt s " - POUs"
    test_extraction(
        "TwinCAT PLC Control - [CELA.pro - POUs]",
        "CELA"
    );
    
    // Test 2: Program s příponou .EXP
    test_extraction(
        "TwinCAT PLC Control - [ST00_PRG.EXP]",
        "ST00_PRG"
    );
    
    // Test 3: Program s celou cestou
    test_extraction(
        "TwinCAT PLC Control - [Stations\\ST_00\\ST00_PRGs\\ST00_CallPRGs.EXP]",
        "ST00_CallPRGs"
    );
    
    // Test 4: Program s lomítky (Linux styl)
    test_extraction(
        "TwinCAT PLC Control - [Stations/ST_00/ST00_PRGs/MachineStates.EXP]",
        "MachineStates"
    );
    
    // Test 5: Projekt bez přípony
    test_extraction(
        "TwinCAT PLC Control - [Palettierer]",
        "Palettierer"
    );
    
    // Test 6: Reálné TwinCAT okno
    printf("===================================================\n");
    printf("  TEST: Reálné TwinCAT okno\n");
    printf("===================================================\n\n");
    
    HWND twincatWindow = FindTwinCatWindow();
    if (twincatWindow) {
        char windowTitle[512];
        GetWindowText(twincatWindow, windowTitle, sizeof(windowTitle));
        
        printf("Nalezeno TwinCAT okno:\n");
        printf("Titulek: '%s'\n\n", windowTitle);
        
        char target[256];
        if (ExtractTargetFromTitle(windowTitle, target, sizeof(target))) {
            printf("✓ Extrahovaný název: '%s'\n", target);
        } else {
            printf("✗ Nepodařilo se extrahovat název z titulku\n");
        }
    } else {
        printf("✗ TwinCAT okno nenalezeno\n");
    }
    
    return 0;
}

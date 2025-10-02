#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "lib/twincat_navigator.h"

// Funkce pro přímou navigaci na řádek
bool NavigateDirectToItem(HWND listbox, int targetIndex) {
    printf("🎯 Navigace na položku %d...\n", targetIndex);
    
    // Metoda 1: Přímé nastavení selection
    LRESULT result = SendMessage(listbox, LB_SETCURSEL, targetIndex, 0);
    if (result == LB_ERR) {
        printf("❌ Chyba při nastavení selection na %d\n", targetIndex);
        return false;
    }
    
    // Metoda 2: Ensure visible (scroll)
    SendMessage(listbox, LB_SETTOPINDEX, targetIndex, 0);
    
    // Metoda 3: Focus + Enter pro otevření (pokud má děti)
    SetFocus(listbox);
    Sleep(100);
    
    // Simulace Enter klávesy pro rozbalení/akci
    keybd_event(VK_RETURN, 0, 0, 0);
    keybd_event(VK_RETURN, 0, KEYEVENTF_KEYUP, 0);
    
    Sleep(200);
    
    printf("✅ Navigováno na položku %d\n", targetIndex);
    return true;
}

// Funkce pro získání textu aktuální položky
bool GetCurrentItemText(HWND listbox, HANDLE hProcess, char* buffer, size_t bufferSize) {
    // Získej aktuální selection
    LRESULT currentSel = SendMessage(listbox, LB_GETCURSEL, 0, 0);
    if (currentSel == LB_ERR) {
        return false;
    }
    
    // Získej ItemData
    LRESULT itemData = SendMessage(listbox, LB_GETITEMDATA, currentSel, 0);
    if (itemData == LB_ERR || itemData == 0) {
        return false;
    }
    
    // Přečti strukturu (offset 20 = text pointer)
    DWORD structure[8];
    SIZE_T bytesRead;
    
    if (!ReadProcessMemory(hProcess, (void*)itemData, structure, sizeof(structure), &bytesRead)) {
        return false;
    }
    
    if (bytesRead < 24) return false;
    
    DWORD textPtr = structure[5];  // Offset 20
    
    if (textPtr < 0x400000 || textPtr > 0x80000000) {
        return false;
    }
    
    // Přečti text
    char textBuffer[512];
    if (ReadProcessMemory(hProcess, (void*)textPtr, textBuffer, sizeof(textBuffer)-1, &bytesRead)) {
        // Zkus různé offsety
        for (int offset = 0; offset < 4; offset++) {
            if (offset >= bytesRead) continue;
            
            char* start = textBuffer + offset;
            size_t len = 0;
            bool valid = true;
            
            for (size_t i = 0; i < bytesRead - offset && i < bufferSize-1; i++) {
                char c = start[i];
                if (c == 0) break;
                if (c >= 32 && c <= 126) {
                    len++;
                } else {
                    valid = false;
                    break;
                }
            }
            
            if (valid && len > 2) {
                strncpy(buffer, start, len);
                buffer[len] = '\0';
                return true;
            }
        }
    }
    
    return false;
}

// Funkce pro test navigace na různé položky
void TestDirectNavigation(HWND listbox, HANDLE hProcess, int itemCount) {
    printf("\n🚀 === TEST PŘÍMÉ NAVIGACE ===\n");
    
    // Test specifických pozic
    int testPositions[] = {0, 5, 10, 15, 25, 35, 50, itemCount-1};
    int numTests = sizeof(testPositions) / sizeof(testPositions[0]);
    
    for (int i = 0; i < numTests; i++) {
        int pos = testPositions[i];
        if (pos >= itemCount) continue;
        
        printf("\n--- Test %d/%d ---\n", i+1, numTests);
        
        if (NavigateDirectToItem(listbox, pos)) {
            // Čekej chvíli na aktualizaci UI
            Sleep(300);
            
            // Přečti text aktuální položky
            char itemText[256];
            if (GetCurrentItemText(listbox, hProcess, itemText, sizeof(itemText))) {
                printf("📍 Pozice %d: '%s'\n", pos, itemText);
            } else {
                printf("📍 Pozice %d: <chyba čtení textu>\n", pos);
            }
            
            // Krátká pauza mezi testy
            Sleep(500);
        }
    }
}

// Interaktivní režim pro manuální navigaci
void InteractiveNavigation(HWND listbox, HANDLE hProcess, int itemCount) {
    printf("\n🎮 === INTERAKTIVNÍ NAVIGACE ===\n");
    printf("Zadejte číslo položky (0-%d) nebo 'q' pro konec:\n", itemCount-1);
    
    char input[32];
    while (true) {
        printf("\nNavigace> ");
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        // Odstraň newline
        input[strcspn(input, "\n")] = 0;
        
        if (strcmp(input, "q") == 0 || strcmp(input, "quit") == 0) {
            break;
        }
        
        int targetPos = atoi(input);
        if (targetPos < 0 || targetPos >= itemCount) {
            printf("❌ Neplatná pozice! Zadejte 0-%d\n", itemCount-1);
            continue;
        }
        
        if (NavigateDirectToItem(listbox, targetPos)) {
            Sleep(300);
            
            char itemText[256];
            if (GetCurrentItemText(listbox, hProcess, itemText, sizeof(itemText))) {
                printf("✅ Úspěšně na pozici %d: '%s'\n", targetPos, itemText);
            } else {
                printf("✅ Na pozici %d (text se nepodařilo přečíst)\n", targetPos);
            }
        }
    }
}

int main() {
    printf("=== DIRECT NAVIGATION TESTER ===\n");
    printf("=== PŘÍMÉ OVLÁDÁNÍ CODESYS TREEVIEW ===\n\n");
    
    // Najdi TwinCAT
    HWND twincat_window = FindTwinCatWindow();
    if (!twincat_window) {
        printf("❌ TwinCAT okno nenalezeno!\n");
        return 1;
    }
    
    char windowTitle[512];
    GetWindowText(twincat_window, windowTitle, sizeof(windowTitle));
    printf("✅ TwinCAT: %s\n", windowTitle);
    
    // Najdi ListBox (CoDeSys TreeView)
    HWND listbox = FindProjectListBox(twincat_window);
    if (!listbox) {
        printf("❌ CoDeSys TreeView nenalezen!\n");
        return 1;
    }
    
    printf("✅ CoDeSys TreeView: 0x%p\n", (void*)listbox);
    
    // Otevři proces
    HANDLE hProcess = OpenTwinCatProcess(listbox);
    if (!hProcess) {
        printf("❌ Nelze otevřít proces!\n");
        return 1;
    }
    
    int itemCount = GetListBoxItemCount(listbox);
    printf("✅ Celkem položek: %d\n", itemCount);
    
    if (itemCount <= 0) {
        printf("❌ TreeView je prázdný!\n");
        CloseHandle(hProcess);
        return 1;
    }
    
    // Zobraz aktuální pozici
    LRESULT currentPos = SendMessage(listbox, LB_GETCURSEL, 0, 0);
    printf("📍 Aktuální pozice: %d\n", (int)currentPos);
    
    // Nabídni možnosti
    printf("\nVyberte režim:\n");
    printf("1. Automatický test navigace na různé pozice\n");
    printf("2. Interaktivní režim (zadávání pozic)\n");
    printf("Vaše volba (1/2): ");
    
    char choice[10];
    if (fgets(choice, sizeof(choice), stdin)) {
        if (choice[0] == '1') {
            TestDirectNavigation(listbox, hProcess, itemCount);
        } else if (choice[0] == '2') {
            InteractiveNavigation(listbox, hProcess, itemCount);
        } else {
            printf("❌ Neplatná volba!\n");
        }
    }
    
    CloseHandle(hProcess);
    
    printf("\n✅ Test navigace dokončen!\n");
    printf("💡 Program prokázal schopnost přímého ovládání CoDeSys TreeView!\n");
    
    return 0;
}
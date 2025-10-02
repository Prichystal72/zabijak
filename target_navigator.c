#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <psapi.h>
#include "lib/twincat_navigator.h"

#define TARGET_ITEM "ST_Markiere_WT_NIO"

// Funkce pro čtení textu z TreeView položky
bool ReadTreeViewItemText(HWND listbox, HANDLE hProcess, int index, char* buffer, size_t bufferSize) {
    LRESULT itemData = SendMessage(listbox, LB_GETITEMDATA, index, 0);
    if (itemData == LB_ERR || itemData == 0) {
        return false;
    }
    
    DWORD structure[8];
    SIZE_T bytesRead;
    
    if (!ReadProcessMemory(hProcess, (void*)itemData, structure, sizeof(structure), &bytesRead)) {
        return false;
    }
    
    if (bytesRead < 24) return false;
    
    DWORD textPtr = structure[5]; // Offset 20
    
    if (textPtr < 0x400000 || textPtr > 0x80000000) {
        return false;
    }
    
    char textBuffer[512];
    if (ReadProcessMemory(hProcess, (void*)textPtr, textBuffer, sizeof(textBuffer)-1, &bytesRead)) {
        for (int offset = 0; offset < 4; offset++) {
            if (offset >= bytesRead) continue;
            
            char* start = textBuffer + offset;
            size_t len = 0;
            
            while (len < bufferSize-1 && offset + len < bytesRead) {
                char c = start[len];
                if (c >= 32 && c <= 126) {
                    buffer[len] = c;
                    len++;
                } else {
                    break;
                }
            }
            
            if (len > 3) {
                buffer[len] = '\0';
                return true;
            }
        }
    }
    
    return false;
}

// Rozbalí systematicky všechny možné složky
void ExpandAllPossibleFolders(HWND listbox, HANDLE hProcess) {
    printf("🌳 Rozbaluji systematicky všechny složky...\n");
    
    int maxRounds = 20; // Maximálně 20 kol rozbalování
    int totalExpanded = 0;
    
    for (int round = 1; round <= maxRounds; round++) {
        printf("\n--- Kolo %d ---\n", round);
        
        int itemCount = SendMessage(listbox, LB_GETCOUNT, 0, 0);
        printf("📊 Aktuální počet položek: %d\n", itemCount);
        
        int expandedThisRound = 0;
        
        // Projdi všechny položky a zkus rozbalit každou
        for (int i = 0; i < itemCount; i++) {
            char itemText[256];
            if (ReadTreeViewItemText(listbox, hProcess, i, itemText, sizeof(itemText))) {
                // Zkus rozbalit každou položku (složky se rozbalí, soubory ne)
                SendMessage(listbox, LB_SETCURSEL, i, 0);
                Sleep(50);
                PostMessage(listbox, WM_KEYDOWN, VK_RETURN, 0);
                PostMessage(listbox, WM_KEYUP, VK_RETURN, 0);
                Sleep(100);
                
                // Zkontroluj, jestli se počet změnil
                int newCount = SendMessage(listbox, LB_GETCOUNT, 0, 0);
                if (newCount > itemCount) {
                    printf("  📁 Rozbaleno '%s': %d → %d (+%d)\n", 
                           itemText, itemCount, newCount, newCount - itemCount);
                    expandedThisRound++;
                    totalExpanded++;
                    itemCount = newCount; // Aktualizuj počet pro další iteraci
                }
                
                // Přerušení, pokud se struktura výrazně změnila
                if (newCount > itemCount + 10) {
                    printf("  ⚡ Velká změna struktury, restartuji kolo\n");
                    break;
                }
            }
        }
        
        printf("Kolo %d: rozbaleno %d složek\n", round, expandedThisRound);
        
        // Pokud se nic nerozbalilo, jsme hotovi
        if (expandedThisRound == 0) {
            printf("✅ Všechny složky rozbaleny! (celkem %d rozbalení)\n", totalExpanded);
            break;
        }
        
        Sleep(500); // Pauza mezi koly
    }
    
    int finalCount = SendMessage(listbox, LB_GETCOUNT, 0, 0);
    printf("🎯 FINÁLNÍ POČET POLOŽEK: %d\n", finalCount);
    printf("📊 Rozbaleno celkem: %d složek\n", totalExpanded);
}

// Uloží aktuální TreeView strukturu do souboru
void SaveTreeViewStructure(HWND listbox, HANDLE hProcess) {
    printf("💾 Ukládám TreeView strukturu...\n");
    
    FILE* file = fopen("full_expanded_structure.txt", "w");
    if (!file) {
        printf("❌ Nelze vytvořit soubor full_expanded_structure.txt\n");
        return;
    }
    
    int itemCount = SendMessage(listbox, LB_GETCOUNT, 0, 0);
    
    fprintf(file, "=== PLNĚ ROZBALENÁ TREEVIEW STRUKTURA ===\n");
    fprintf(file, "Celkem položek: %d\n\n", itemCount);
    
    for (int i = 0; i < itemCount; i++) {
        char itemText[256];
        if (ReadTreeViewItemText(listbox, hProcess, i, itemText, sizeof(itemText))) {
            fprintf(file, "[%04d] %s\n", i, itemText);
        } else {
            fprintf(file, "[%04d] <nelze přečíst>\n", i);
        }
    }
    
    fclose(file);
    printf("✅ Plná struktura uložena do: full_expanded_structure.txt\n");
}

// Jednoduché hledání v už plně rozbalené struktuře
bool SmartNavigateToTarget(HWND listbox, HANDLE hProcess) {
    printf("🎯 === HLEDÁNÍ V PLNÉ STRUKTUŘE ===\n");
    
    int itemCount = SendMessage(listbox, LB_GETCOUNT, 0, 0);
    printf("📊 Prohledávám %d položek...\n", itemCount);
    
    // Projdi všechny položky a hledej target
    for (int i = 0; i < itemCount; i++) {
        char itemText[256];
        if (ReadTreeViewItemText(listbox, hProcess, i, itemText, sizeof(itemText))) {
            if (strcmp(itemText, TARGET_ITEM) == 0) {
                printf("🎉 NALEZEN! Pozice %d: '%s'\n", i, itemText);
                
                // Naviguj na target
                SetFocus(listbox);
                Sleep(100);
                SendMessage(listbox, LB_SETCURSEL, i, 0);
                Sleep(100);
                
                // Scroll do pohledu
                if (i > 10) {
                    SendMessage(listbox, LB_SETTOPINDEX, i - 5, 0);
                }
                Sleep(100);
                
                // Aktivuj target
                PostMessage(listbox, WM_KEYDOWN, VK_RETURN, 0);
                PostMessage(listbox, WM_KEYUP, VK_RETURN, 0);
                
                printf("✅ ÚSPĚCH! Navigace na '%s' dokončena\n", TARGET_ITEM);
                printf("🎯 Pozice: %d ze %d\n", i, itemCount);
                return true;
            }
            
            // Debug: zobraz všechny ST_ položky
            if (strstr(itemText, "ST_") == itemText) {
                printf("  [%d] %s\n", i, itemText);
            }
        }
    }
    
    printf("❌ Target '%s' nenalezen ani v plně rozbalené struktuře!\n", TARGET_ITEM);
    return false;
}

int main() {
    printf("=== COMPLETE TREE EXPANDER: '%s' ===\n", TARGET_ITEM);
    printf("=== ROZBALENÍ VŠECH SLOŽEK A NAVIGACE ===\n\n");
    
    // Najdi TwinCAT okno
    HWND twincatWindow = FindTwinCatWindow();
    if (!twincatWindow) {
        printf("❌ TwinCAT okno nenalezeno!\n");
        return 1;
    }

    char windowTitle[256];
    GetWindowText(twincatWindow, windowTitle, sizeof(windowTitle));
    printf("✅ TwinCAT: %s\n", windowTitle);

    // Získej handle procesu
    DWORD processId;
    GetWindowThreadProcessId(twincatWindow, &processId);
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) {
        printf("❌ Nelze otevřít proces pro čtení paměti!\n");
        return 1;
    }

    // FÁZE 0: Kompletní analýza paměti (nezávisle na ListBoxu)
    printf("\n🧠 === FÁZE 0: PLNÝ DUMP PAMĚTI ===\n");
    AnalyzeFullMemoryStructure(hProcess, "full_memory_dump.txt");

    // (Původní ListBox logika může zůstat, nebo ji lze zakomentovat)
    //HWND listbox = FindProjectListBox(twincatWindow);
    //if (!listbox) {
    //    printf("❌ Project explorer ListBox nenalezen!\n");
    //    return 1;
    //}
    //printf("✅ CoDeSys TreeView: 0x%p\n", (void*)listbox);
    //ExpandAllPossibleFolders(listbox, hProcess);
    //SaveTreeViewStructure(listbox, hProcess);
    //bool success = SmartNavigateToTarget(listbox, hProcess);

    CloseHandle(hProcess);

    printf("\n📁 === VYTVOŘENÉ SOUBORY ===\n");
    printf("✅ full_memory_dump.txt - Kompletní dump paměti (všechny ST_ položky)\n");

    printf("\n🎉 === HOTOVO! ===\n");
    return 0;
}
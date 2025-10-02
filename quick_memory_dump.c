#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "lib/twincat_navigator.h"

// Jednoduchá verzia pre rýchly hex dump
int main() {
    printf("=== RYCHLY MEMORY HEX DUMP ===\n");
    
    // Najdi TwinCAT okno
    HWND twincat_window = FindTwinCatWindow();
    if (!twincat_window) {
        printf("❌ TwinCAT okno nenalezeno!\n");
        return 1;
    }
    
    char windowTitle[512];
    GetWindowText(twincat_window, windowTitle, sizeof(windowTitle));
    printf("✅ TwinCAT: '%s'\n", windowTitle);
    
    // Najdi ListBox
    HWND listbox = FindProjectListBox(twincat_window);
    if (!listbox) {
        printf("❌ ListBox nenalezen!\n");
        return 1;
    }
    
    printf("✅ ListBox: 0x%p\n", (void*)listbox);
    
    // Otevři proces
    HANDLE hProcess = OpenTwinCatProcess(listbox);
    if (!hProcess) {
        printf("❌ Nelze otevřít proces!\n");
        return 1;
    }
    
    int itemCount = GetListBoxItemCount(listbox);
    printf("✅ Položek: %d\n", itemCount);
    
    // Vytvoř hex dump file
    FILE* hexFile = fopen("quick_memory_dump.hex", "w");
    if (!hexFile) {
        printf("❌ Nelze vytvořit hex soubor!\n");
        CloseHandle(hProcess);
        return 1;
    }
    
    fprintf(hexFile, "=== RYCHLY MEMORY HEX DUMP ===\n");
    fprintf(hexFile, "TwinCAT: %s\n", windowTitle);
    fprintf(hexFile, "ListBox: 0x%p\n", (void*)listbox);
    fprintf(hexFile, "Položek: %d\n\n", itemCount);
    
    // Analyzuj jen prvních 3 položky
    int analyzeCount = (itemCount > 3) ? 3 : itemCount;
    
    for (int i = 0; i < analyzeCount; i++) {
        printf("📍 Položka %d...\n", i);
        
        LRESULT itemData = SendMessage(listbox, LB_GETITEMDATA, i, 0);
        if (itemData == LB_ERR || itemData == 0) {
            fprintf(hexFile, "--- Položka %d: CHYBA ---\n\n", i);
            continue;
        }
        
        fprintf(hexFile, "--- Položka %d: ItemData 0x%p ---\n", i, (void*)itemData);
        
        // Přečti 128 bytů z ItemData
        unsigned char buffer[128];
        SIZE_T bytesRead;
        
        if (ReadProcessMemory(hProcess, (void*)itemData, buffer, sizeof(buffer), &bytesRead)) {
            fprintf(hexFile, "Přečteno: %zu bytů\n", bytesRead);
            
            // Hex dump
            for (size_t offset = 0; offset < bytesRead; offset += 16) {
                fprintf(hexFile, "%08X: ", (unsigned int)(itemData + offset));
                
                // Hex hodnoty
                for (int j = 0; j < 16; j++) {
                    if (offset + j < bytesRead) {
                        fprintf(hexFile, "%02X ", buffer[offset + j]);
                    } else {
                        fprintf(hexFile, "   ");
                    }
                }
                
                fprintf(hexFile, " | ");
                
                // ASCII
                for (int j = 0; j < 16 && offset + j < bytesRead; j++) {
                    unsigned char c = buffer[offset + j];
                    fprintf(hexFile, "%c", (c >= 32 && c <= 126) ? c : '.');
                }
                
                fprintf(hexFile, "\n");
            }
            
            // Hledej možné text pointery na offset 20
            if (bytesRead >= 24) {
                DWORD* ptr = (DWORD*)(buffer + 20);
                DWORD textAddr = *ptr;
                
                fprintf(hexFile, "\nText pointer na offset 20: 0x%08X\n", textAddr);
                
                if (textAddr > 0x400000 && textAddr < 0x80000000) {
                    char textBuffer[256] = {0};
                    SIZE_T textRead;
                    
                    if (ReadProcessMemory(hProcess, (void*)textAddr, textBuffer, sizeof(textBuffer)-1, &textRead)) {
                        fprintf(hexFile, "Text obsah: ");
                        for (size_t t = 0; t < textRead && t < 50; t++) {
                            if (textBuffer[t] == 0) break;
                            if (textBuffer[t] >= 32 && textBuffer[t] <= 126) {
                                fprintf(hexFile, "%c", textBuffer[t]);
                            } else {
                                fprintf(hexFile, ".");
                            }
                        }
                        fprintf(hexFile, "\n");
                    }
                }
            }
        } else {
            fprintf(hexFile, "Chyba při čtení paměti\n");
        }
        
        fprintf(hexFile, "\n");
    }
    
    fclose(hexFile);
    CloseHandle(hProcess);
    
    printf("✅ Hex dump vytvořen: quick_memory_dump.hex\n");
    return 0;
}
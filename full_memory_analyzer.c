#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "lib/twincat_navigator.h"

// Struktura pro úplný memory dump
typedef struct {
    DWORD_PTR address;
    size_t size;
    unsigned char* data;
    size_t actualRead;
    char description[256];
} FullMemoryBlock;

// Funkce pro hex dump celé memory oblasti
void WriteFullHexDump(FILE* f, FullMemoryBlock* block) {
    fprintf(f, "\n=== %s ===\n", block->description);
    fprintf(f, "Adresa: 0x%p\n", (void*)block->address);
    fprintf(f, "Velikost: %zu bytů\n", block->actualRead);
    fprintf(f, "Požadováno: %zu bytů\n\n", block->size);
    
    for (size_t offset = 0; offset < block->actualRead; offset += 16) {
        // Adresa řádku
        fprintf(f, "%08X: ", (unsigned int)(block->address + offset));
        
        // Hex hodnoty (16 bytů na řádek)
        for (int i = 0; i < 16; i++) {
            if (offset + i < block->actualRead) {
                fprintf(f, "%02X ", block->data[offset + i]);
            } else {
                fprintf(f, "   ");
            }
            
            // Mezera po 8 bytech pro lepší čitelnost
            if (i == 7) fprintf(f, " ");
        }
        
        fprintf(f, " | ");
        
        // ASCII reprezentace
        for (int i = 0; i < 16 && offset + i < block->actualRead; i++) {
            unsigned char c = block->data[offset + i];
            fprintf(f, "%c", (c >= 32 && c <= 126) ? c : '.');
        }
        
        fprintf(f, "\n");
    }
    
    fprintf(f, "\n");
}

// Funkce pro analýzu všech možných pointerů ve struktuře
void AnalyzeAllPointers(FILE* f, unsigned char* data, size_t size, HANDLE hProcess, DWORD_PTR baseAddr) {
    fprintf(f, "=== ANALÝZA VŠECH MOŽNÝCH POINTERŮ ===\n");
    
    size_t dwordCount = size / 4;
    DWORD* dwords = (DWORD*)data;
    
    for (size_t i = 0; i < dwordCount; i++) {
        DWORD value = dwords[i];
        
        // Zkontroluj, jestli to může být pointer
        if (value > 0x400000 && value < 0x80000000) {
            fprintf(f, "Offset %02zu (0x%02X): 0x%08X - možný pointer\n", 
                   i*4, (unsigned int)(i*4), value);
            
            // Pokus se přečíst z této adresy
            unsigned char testBuffer[64];
            SIZE_T testRead;
            
            if (ReadProcessMemory(hProcess, (void*)value, testBuffer, sizeof(testBuffer), &testRead)) {
                fprintf(f, "  -> Úspěšně přečteno %zu bytů z 0x%08X\n", testRead, value);
                
                // Zobraz prvních 32 bytů
                fprintf(f, "  -> Hex: ");
                for (size_t j = 0; j < testRead && j < 32; j++) {
                    fprintf(f, "%02X ", testBuffer[j]);
                }
                fprintf(f, "\n");
                
                // Pokus se najít text
                fprintf(f, "  -> ASCII: ");
                for (size_t j = 0; j < testRead && j < 32; j++) {
                    unsigned char c = testBuffer[j];
                    if (c >= 32 && c <= 126) {
                        fprintf(f, "%c", c);
                    } else if (c == 0) {
                        fprintf(f, "\\0");
                        break;
                    } else {
                        fprintf(f, ".");
                    }
                }
                fprintf(f, "\n\n");
            } else {
                fprintf(f, "  -> Chyba při čtení z adresy 0x%08X\n\n", value);
            }
        }
    }
    
    fprintf(f, "=== KONEC ANALÝZY POINTERŮ ===\n\n");
}

int main() {
    printf("=== ÚPLNÁ MEMORY ANALÝZA A HEX DUMP ===\n");
    printf("=== KOMPLETNÍ ZÁPIS PAMĚTI PRO RUČNÍ ANALÝZU ===\n\n");
    
    // Najdi TwinCAT
    HWND twincat_window = FindTwinCatWindow();
    if (!twincat_window) {
        printf("❌ TwinCAT okno nenalezeno!\n");
        return 1;
    }
    
    char windowTitle[512];
    GetWindowText(twincat_window, windowTitle, sizeof(windowTitle));
    printf("✅ TwinCAT: %s\n", windowTitle);
    
    // Najdi ListBox
    HWND listbox = FindProjectListBox(twincat_window);
    if (!listbox) {
        printf("❌ ListBox nenalezen!\n");
        return 1;
    }
    
    printf("✅ ListBox: 0x%p\n", (void*)listbox);
    
    // Ověř owner-drawn
    LONG style = GetWindowLong(listbox, GWL_STYLE);
    printf("✅ Window Style: 0x%lX", style);
    if (style & LBS_OWNERDRAWFIXED) {
        printf(" (OWNER-DRAWN FIXED potvrzeno)\n");
    } else {
        printf(" (varování: není owner-drawn fixed)\n");
    }
    
    // Otevři proces
    HANDLE hProcess = OpenTwinCatProcess(listbox);
    if (!hProcess) {
        printf("❌ Nelze otevřít proces!\n");
        return 1;
    }
    
    int itemCount = GetListBoxItemCount(listbox);
    printf("✅ Celkem položek: %d\n", itemCount);
    
    if (itemCount <= 0) {
        printf("❌ ListBox je prázdný!\n");
        CloseHandle(hProcess);
        return 1;
    }
    
    // Vytvoř hlavní hex dump soubor
    FILE* hexFile = fopen("complete_memory_dump.hex", "w");
    if (!hexFile) {
        printf("❌ Nelze vytvořit complete_memory_dump.hex!\n");
        CloseHandle(hProcess);
        return 1;
    }
    
    fprintf(hexFile, "=== ÚPLNÁ MEMORY ANALÝZA OWNER-DRAWN LISTBOX ===\n");
    fprintf(hexFile, "Program: TwinCAT Memory Analyzer\n");
    fprintf(hexFile, "Datum: %s", __DATE__);
    fprintf(hexFile, " %s\n", __TIME__);
    fprintf(hexFile, "TwinCAT okno: %s\n", windowTitle);
    fprintf(hexFile, "ListBox handle: 0x%p\n", (void*)listbox);
    fprintf(hexFile, "Window Style: 0x%lX\n", style);
    fprintf(hexFile, "Celkem položek: %d\n", itemCount);
    fprintf(hexFile, "========================================\n\n");
    
    // Analyzuj všechny položky (bez omezení)
    printf("\n🔍 Zahajuji úplnou analýzu %d položek...\n", itemCount);
    
    for (int i = 0; i < itemCount; i++) {
        printf("📍 Analyzuji položku %d/%d...\n", i+1, itemCount);
        
        // Získej ItemData pointer
        LRESULT itemData = SendMessage(listbox, LB_GETITEMDATA, i, 0);
        
        if (itemData == LB_ERR || itemData == 0) {
            fprintf(hexFile, "--- POLOŽKA %d: CHYBA ITEMDATA ---\n\n", i);
            continue;
        }
        
        // Vytvoř popis pro tuto položku
        FullMemoryBlock block;
        block.address = (DWORD_PTR)itemData;
        block.size = 512;  // Větší blok pro úplnou analýzu
        block.data = malloc(block.size);
        
        if (!block.data) {
            fprintf(hexFile, "--- POLOŽKA %d: CHYBA ALOKACE PAMĚTI ---\n\n", i);
            continue;
        }
        
        snprintf(block.description, sizeof(block.description), 
                "POLOŽKA %d - ItemData Structure", i);
        
        // Přečti ItemData strukturu
        SIZE_T bytesRead;
        if (ReadProcessMemory(hProcess, (void*)itemData, block.data, block.size, &bytesRead)) {
            block.actualRead = bytesRead;
            
            fprintf(hexFile, "###############################################\n");
            fprintf(hexFile, "### POLOŽKA %d - ITEMDATA 0x%p ###\n", i, (void*)itemData);
            fprintf(hexFile, "###############################################\n");
            
            // Zapíš hex dump ItemData
            WriteFullHexDump(hexFile, &block);
            
            // Analyzuj všechny možné pointery v této struktuře
            AnalyzeAllPointers(hexFile, block.data, bytesRead, hProcess, block.address);
            
            fprintf(hexFile, "=== DALŠÍ MEMORY OBLASTI PRO POLOŽKU %d ===\n", i);
            
            // Hledej a čti z dalších možných adres
            DWORD* dwords = (DWORD*)block.data;
            size_t dwordCount = bytesRead / 4;
            
            for (size_t j = 0; j < dwordCount; j++) {
                DWORD possibleAddr = dwords[j];
                
                // Pokud vypadá jako validní adresa, přečti z ní více dat
                if (possibleAddr > 0x400000 && possibleAddr < 0x80000000) {
                    FullMemoryBlock extraBlock;
                    extraBlock.address = possibleAddr;
                    extraBlock.size = 256;
                    extraBlock.data = malloc(extraBlock.size);
                    
                    if (extraBlock.data) {
                        SIZE_T extraRead;
                        if (ReadProcessMemory(hProcess, (void*)possibleAddr, 
                                            extraBlock.data, extraBlock.size, &extraRead)) {
                            extraBlock.actualRead = extraRead;
                            snprintf(extraBlock.description, sizeof(extraBlock.description),
                                    "POLOŽKA %d - Memory z pointeru na offset %zu (0x%08X)", 
                                    i, j*4, possibleAddr);
                            
                            WriteFullHexDump(hexFile, &extraBlock);
                        }
                        free(extraBlock.data);
                    }
                }
            }
            
        } else {
            fprintf(hexFile, "--- POLOŽKA %d: CHYBA PŘI ČTENÍ ITEMDATA ---\n", i);
            fprintf(hexFile, "ItemData pointer: 0x%p\n", (void*)itemData);
            fprintf(hexFile, "GetLastError: %lu\n\n", GetLastError());
        }
        
        free(block.data);
        
        // Progress update každých 10 položek
        if ((i + 1) % 10 == 0) {
            printf("📊 Zpracováno %d/%d položek...\n", i+1, itemCount);
        }
    }
    
    fprintf(hexFile, "\n=== KONEC ÚPLNÉ MEMORY ANALÝZY ===\n");
    fprintf(hexFile, "Celkem analyzováno: %d položek\n", itemCount);
    
    fclose(hexFile);
    CloseHandle(hProcess);
    
    printf("\n✅ ÚPLNÁ ANALÝZA DOKONČENA!\n");
    printf("📄 Výsledek: complete_memory_dump.hex\n");
    printf("📊 Analyzováno: %d položek\n", itemCount);
    printf("💡 Tip: Hledejte texty ručně v hex dump souboru\n");
    
    return 0;
}
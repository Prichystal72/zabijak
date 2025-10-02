#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "lib/twincat_navigator.h"

// Konstanty pro memory analyzér
#define MAX_MEMORY_DUMP 4096
#define HEX_BYTES_PER_LINE 16
#define MAX_STRUCTURE_SIZE 256

// Struktura pro surové memory data
typedef struct {
    DWORD_PTR address;
    size_t size;
    unsigned char data[MAX_MEMORY_DUMP];
    size_t actualRead;
} MemoryBlock;

// Struktura pro analýzu ItemData
typedef struct {
    DWORD_PTR itemDataPtr;
    DWORD structure[64];  // Rozšířená struktura pro detailní analýzu
    size_t structureSize;
    DWORD_PTR textPointers[10];  // Možné text pointery
    int textPointerCount;
    char extractedTexts[10][256];  // Extrahované texty
    int validTextCount;
} ItemDataAnalysis;

// Prototypy funkcí
void DumpMemoryToHex(const char* filename, MemoryBlock* blocks, int blockCount);
void HexDumpBlock(FILE* f, MemoryBlock* block, const char* description);
bool AnalyzeItemData(HANDLE hProcess, DWORD_PTR itemDataPtr, ItemDataAnalysis* analysis);
bool ExtractTextFromPointer(HANDLE hProcess, DWORD_PTR textPtr, char* buffer, size_t bufferSize);
void PrintAnalysisSummary(ItemDataAnalysis* analyses, int count);
bool IsValidTextPointer(DWORD_PTR ptr);
void AnalyzeStructurePattern(DWORD* structure, size_t size, FILE* logFile);

// Hlavní funkce pro analýzu paměti ListBox komponenty
int main() {
    printf("=== MEMORY ANALYZER PRO OWNER-DRAWN FIXED LISTBOX ===\n");
    printf("=== ANALÝZA PAMĚTI A HEX DUMP ===\n\n");
    
    // Krok 1: Najdi TwinCAT okno
    printf("🔍 Hledám TwinCAT okno...\n");
    HWND twincat_window = FindTwinCatWindow();
    
    if (!twincat_window) {
        printf("❌ TwinCAT okno nenalezeno!\n");
        return 1;
    }
    
    char windowTitle[512];
    GetWindowText(twincat_window, windowTitle, sizeof(windowTitle));
    printf("✅ TwinCAT okno: '%s'\n", windowTitle);
    
    // Krok 2: Najdi ListBox
    printf("\n🔍 Hledám owner-drawn fixed ListBox...\n");
    HWND listbox = FindProjectListBox(twincat_window);
    
    if (!listbox) {
        printf("❌ ListBox nenalezen!\n");
        return 1;
    }
    
    printf("✅ ListBox nalezen: 0x%p\n", (void*)listbox);
    
    // Ověř, že je to owner-drawn
    LONG style = GetWindowLong(listbox, GWL_STYLE);
    if (style & LBS_OWNERDRAWFIXED) {
        printf("✅ Potvrzeno: Owner-drawn fixed ListBox\n");
    } else {
        printf("⚠️ Varování: ListBox není owner-drawn fixed (style=0x%lX)\n", style);
    }
    
    // Krok 3: Otevři proces pro čtení paměti
    printf("\n🔓 Otevírám proces pro čtení paměti...\n");
    HANDLE hProcess = OpenTwinCatProcess(listbox);
    
    if (!hProcess) {
        printf("❌ Nelze otevřít proces!\n");
        return 1;
    }
    
    printf("✅ Proces otevřen pro čtení paměti\n");
    
    // Krok 4: Analyzuj ListBox struktury
    int itemCount = GetListBoxItemCount(listbox);
    printf("\n📊 Celkem položek v ListBox: %d\n", itemCount);
    
    if (itemCount <= 0) {
        printf("❌ ListBox je prázdný nebo chyba!\n");
        CloseHandle(hProcess);
        return 1;
    }
    
    // Omez počet položek pro analýzu (zmenšeno pro rychlejší test)
    int analyzeCount = (itemCount > 5) ? 5 : itemCount;
    printf("📋 Analyzuji prvních %d položek...\n\n", analyzeCount);
    
    // Krok 5: Proveď detailní analýzu paměti pro každou položku
    ItemDataAnalysis* analyses = malloc(analyzeCount * sizeof(ItemDataAnalysis));
    MemoryBlock* memoryBlocks = malloc(analyzeCount * 2 * sizeof(MemoryBlock)); // ItemData + možné texty
    int blockCount = 0;
    
    for (int i = 0; i < analyzeCount; i++) {
        printf("🔍 Analyzuji položku %d...\n", i);
        
        // Získej ItemData pointer
        LRESULT itemData = SendMessage(listbox, LB_GETITEMDATA, i, 0);
        
        if (itemData == LB_ERR || itemData == 0) {
            printf("   ❌ Chyba při získávání ItemData\n");
            continue;
        }
        
        printf("   📍 ItemData pointer: 0x%p\n", (void*)itemData);
        
        // Analyzuj ItemData strukturu
        if (AnalyzeItemData(hProcess, (DWORD_PTR)itemData, &analyses[i])) {
            printf("   ✅ Struktura analyzována (%zu bytů)\n", analyses[i].structureSize);
            printf("   🔗 Nalezeno %d možných text pointerů\n", analyses[i].textPointerCount);
            printf("   📝 Extrahováno %d platných textů\n", analyses[i].validTextCount);
            
            // Přidej memory block pro ItemData
            memoryBlocks[blockCount].address = (DWORD_PTR)itemData;
            memoryBlocks[blockCount].size = analyses[i].structureSize;
            memcpy(memoryBlocks[blockCount].data, analyses[i].structure, analyses[i].structureSize);
            memoryBlocks[blockCount].actualRead = analyses[i].structureSize;
            blockCount++;
            
            // Přidej memory bloky pro nalezené texty
            for (int t = 0; t < analyses[i].textPointerCount && blockCount < analyzeCount * 2; t++) {
                MemoryBlock textBlock = {0};
                textBlock.address = analyses[i].textPointers[t];
                textBlock.size = 256;  // Standardní velikost pro text
                
                SIZE_T bytesRead;
                if (ReadProcessMemory(hProcess, (void*)analyses[i].textPointers[t], 
                                    textBlock.data, textBlock.size, &bytesRead)) {
                    textBlock.actualRead = bytesRead;
                    memoryBlocks[blockCount] = textBlock;
                    blockCount++;
                }
            }
        } else {
            printf("   ❌ Chyba při analýze struktury\n");
        }
        
        printf("\n");
    }
    
    // Krok 6: Vytvoř hex dump soubory
    printf("💾 Vytvářím hex dump soubory...\n");
    DumpMemoryToHex("memory_dump.hex", memoryBlocks, blockCount);
    
    // Krok 7: Vytvoř analýzu přehled
    PrintAnalysisSummary(analyses, analyzeCount);
    
    // Uklid
    free(analyses);
    free(memoryBlocks);
    CloseHandle(hProcess);
    
    printf("\n✅ Analýza dokončena!\n");
    printf("📄 Výsledky:\n");
    printf("   - memory_dump.hex      (hex dump všech analyzovaných bloků)\n");
    printf("   - analysis_summary.txt (přehled nalezených struktur)\n");
    
    return 0;
}

// Analyzuje ItemData strukturu a hledá text pointery
bool AnalyzeItemData(HANDLE hProcess, DWORD_PTR itemDataPtr, ItemDataAnalysis* analysis) {
    if (!analysis) return false;
    
    memset(analysis, 0, sizeof(ItemDataAnalysis));
    analysis->itemDataPtr = itemDataPtr;
    
    // Přečti větší blok paměti pro analýzu
    analysis->structureSize = sizeof(analysis->structure);
    SIZE_T bytesRead;
    
    if (!ReadProcessMemory(hProcess, (void*)itemDataPtr, analysis->structure, 
                          analysis->structureSize, &bytesRead)) {
        return false;
    }
    
    analysis->structureSize = bytesRead;
    
    // Hledej možné text pointery ve struktuře
    size_t dwordCount = bytesRead / sizeof(DWORD);
    
    for (size_t i = 0; i < dwordCount && analysis->textPointerCount < 10; i++) {
        DWORD value = analysis->structure[i];
        
        if (IsValidTextPointer(value)) {
            analysis->textPointers[analysis->textPointerCount] = value;
            
            // Pokus se extrahovat text
            if (ExtractTextFromPointer(hProcess, value, 
                                     analysis->extractedTexts[analysis->validTextCount], 
                                     sizeof(analysis->extractedTexts[0]))) {
                analysis->validTextCount++;
            }
            
            analysis->textPointerCount++;
        }
    }
    
    return true;
}

// Extrahuje text z memory pointeru
bool ExtractTextFromPointer(HANDLE hProcess, DWORD_PTR textPtr, char* buffer, size_t bufferSize) {
    if (!buffer || bufferSize == 0) return false;
    
    memset(buffer, 0, bufferSize);
    
    char tempBuffer[512];
    SIZE_T bytesRead;
    
    if (!ReadProcessMemory(hProcess, (void*)textPtr, tempBuffer, sizeof(tempBuffer)-1, &bytesRead)) {
        return false;
    }
    
    tempBuffer[bytesRead] = '\0';
    
    // Zkus různé offsety (0, 1, 2, 4)
    for (int offset = 0; offset < 4 && offset < bytesRead; offset++) {
        char* startPos = tempBuffer + offset;
        size_t remainingBytes = bytesRead - offset;
        
        // Zkontroluj, jestli je to platný text
        size_t textLen = 0;
        bool isValid = true;
        
        for (size_t i = 0; i < remainingBytes && i < bufferSize-1; i++) {
            char c = startPos[i];
            if (c == '\0') break;
            
            if (c >= 32 && c <= 126) {  // Printable ASCII
                textLen++;
            } else {
                isValid = false;
                break;
            }
        }
        
        if (isValid && textLen > 2) {  // Alespoň 3 znaky
            strncpy(buffer, startPos, textLen);
            buffer[textLen] = '\0';
            return true;
        }
    }
    
    return false;
}

// Zkontroluje, jestli hodnota může být platný text pointer
bool IsValidTextPointer(DWORD_PTR ptr) {
    // Základní kontrola rozsahu paměti
    return (ptr > 0x400000 && ptr < 0x80000000) && (ptr & 0x3) == 0; // Zarovnané na 4 byty
}

// Vytvoří hex dump do souboru
void DumpMemoryToHex(const char* filename, MemoryBlock* blocks, int blockCount) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        printf("❌ Nelze vytvořit soubor %s\n", filename);
        return;
    }
    
    fprintf(f, "=== MEMORY ANALYZER HEX DUMP ===\n");
    fprintf(f, "=== OWNER-DRAWN FIXED LISTBOX ANALYSIS ===\n\n");
    fprintf(f, "Celkem analyzovaných bloků: %d\n\n", blockCount);
    
    for (int i = 0; i < blockCount; i++) {
        char description[256];
        snprintf(description, sizeof(description), "Block %d (0x%p)", i+1, (void*)blocks[i].address);
        HexDumpBlock(f, &blocks[i], description);
        fprintf(f, "\n");
    }
    
    fclose(f);
    printf("✅ Hex dump vytvořen: %s\n", filename);
}

// Vytvoří hex dump jednoho memory bloku
void HexDumpBlock(FILE* f, MemoryBlock* block, const char* description) {
    fprintf(f, "=== %s ===\n", description);
    fprintf(f, "Adresa: 0x%p\n", (void*)block->address);
    fprintf(f, "Velikost: %zu bytů\n", block->actualRead);
    fprintf(f, "\n");
    
    for (size_t offset = 0; offset < block->actualRead; offset += HEX_BYTES_PER_LINE) {
        // Adresa
        fprintf(f, "%08X: ", (unsigned int)(block->address + offset));
        
        // Hex hodnoty
        for (int i = 0; i < HEX_BYTES_PER_LINE; i++) {
            if (offset + i < block->actualRead) {
                fprintf(f, "%02X ", block->data[offset + i]);
            } else {
                fprintf(f, "   ");
            }
        }
        
        fprintf(f, " | ");
        
        // ASCII reprezentace
        for (int i = 0; i < HEX_BYTES_PER_LINE && offset + i < block->actualRead; i++) {
            unsigned char c = block->data[offset + i];
            fprintf(f, "%c", (c >= 32 && c <= 126) ? c : '.');
        }
        
        fprintf(f, "\n");
    }
}

// Vytvoří přehled analýzy
void PrintAnalysisSummary(ItemDataAnalysis* analyses, int count) {
    FILE* f = fopen("analysis_summary.txt", "w");
    if (!f) {
        printf("❌ Nelze vytvořit analysis_summary.txt\n");
        return;
    }
    
    fprintf(f, "=== MEMORY ANALYSIS SUMMARY ===\n");
    fprintf(f, "=== OWNER-DRAWN FIXED LISTBOX ===\n\n");
    fprintf(f, "Analyzovaných položek: %d\n\n", count);
    
    for (int i = 0; i < count; i++) {
        fprintf(f, "--- Položka %d ---\n", i);
        fprintf(f, "ItemData pointer: 0x%p\n", (void*)analyses[i].itemDataPtr);
        fprintf(f, "Velikost struktury: %zu bytů\n", analyses[i].structureSize);
        fprintf(f, "Nalezené text pointery: %d\n", analyses[i].textPointerCount);
        
        for (int t = 0; t < analyses[i].textPointerCount; t++) {
            fprintf(f, "  Pointer[%d]: 0x%p\n", t, (void*)analyses[i].textPointers[t]);
        }
        
        fprintf(f, "Extrahované texty: %d\n", analyses[i].validTextCount);
        for (int t = 0; t < analyses[i].validTextCount; t++) {
            fprintf(f, "  Text[%d]: '%s'\n", t, analyses[i].extractedTexts[t]);
        }
        
        fprintf(f, "\n");
    }
    
    fclose(f);
    printf("✅ Přehled analýzy vytvořen: analysis_summary.txt\n");
}
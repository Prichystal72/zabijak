#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "lib/twincat_navigator.h"

// Struktura pro kompletní přehled
typedef struct {
    int itemIndex;
    DWORD_PTR itemDataAddr;
    DWORD_PTR textPointer;
    char extractedText[256];
    bool hasValidText;
    DWORD flags;
    DWORD position;
    DWORD rawStructure[32];  // První 32 DWORDs pro analýzu
} ItemOverview;

// Funkce pro extrakci textu s různými offsety
bool ExtractTextAdvanced(HANDLE hProcess, DWORD_PTR textPtr, char* buffer, size_t bufferSize) {
    if (!buffer || bufferSize == 0) return false;
    
    memset(buffer, 0, bufferSize);
    
    // Zkus přečíst 512 bytů z adresy
    char rawBuffer[512];
    SIZE_T bytesRead;
    
    if (!ReadProcessMemory(hProcess, (void*)textPtr, rawBuffer, sizeof(rawBuffer), &bytesRead)) {
        return false;
    }
    
    // Zkus různé offsety a způsoby čtení
    int offsets[] = {0, 1, 2, 4, 8, 16};
    int numOffsets = sizeof(offsets) / sizeof(offsets[0]);
    
    for (int o = 0; o < numOffsets; o++) {
        int offset = offsets[o];
        if (offset >= bytesRead) continue;
        
        // ASCII text
        char* start = rawBuffer + offset;
        size_t remaining = bytesRead - offset;
        size_t textLen = 0;
        bool validText = true;
        
        for (size_t i = 0; i < remaining && i < bufferSize-1; i++) {
            char c = start[i];
            if (c == 0) break;
            
            if (c >= 32 && c <= 126) {
                textLen++;
            } else {
                validText = false;
                break;
            }
        }
        
        if (validText && textLen >= 3) {  // Alespoň 3 znaky
            strncpy(buffer, start, textLen);
            buffer[textLen] = '\0';
            return true;
        }
        
        // Unicode text (každý druhý byte)
        textLen = 0;
        validText = true;
        for (size_t i = offset; i < remaining-1 && textLen < bufferSize-1; i += 2) {
            char c = start[i];
            if (c == 0) break;
            
            if (c >= 32 && c <= 126) {
                buffer[textLen++] = c;
            } else {
                validText = false;
                break;
            }
        }
        
        if (validText && textLen >= 3) {
            buffer[textLen] = '\0';
            return true;
        }
    }
    
    return false;
}

int main() {
    printf("=== KOMPLETNÍ STRUKTURÁLNÍ ANALÝZA LISTBOX ===\n");
    printf("=== PŘEHLED VŠECH POLOŽEK A JEJICH VZTAHŮ ===\n\n");
    
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
    
    // Otevři proces
    HANDLE hProcess = OpenTwinCatProcess(listbox);
    if (!hProcess) {
        printf("❌ Nelze otevřít proces!\n");
        return 1;
    }
    
    int itemCount = GetListBoxItemCount(listbox);
    printf("✅ Celkem položek: %d\n\n", itemCount);
    
    if (itemCount <= 0) {
        printf("❌ ListBox je prázdný!\n");
        CloseHandle(hProcess);
        return 1;
    }
    
    // Vytvoř přehledový soubor
    FILE* overview = fopen("listbox_structure_overview.txt", "w");
    if (!overview) {
        printf("❌ Nelze vytvořit overview soubor!\n");
        CloseHandle(hProcess);
        return 1;
    }
    
    fprintf(overview, "=== KOMPLETNÍ STRUKTURÁLNÍ PŘEHLED LISTBOX ===\n");
    fprintf(overview, "TwinCAT: %s\n", windowTitle);
    fprintf(overview, "ListBox: 0x%p\n", (void*)listbox);
    fprintf(overview, "Celkem položek: %d\n", itemCount);
    fprintf(overview, "===================================================\n\n");
    
    // Analyzuj všechny položky
    ItemOverview* items = malloc(itemCount * sizeof(ItemOverview));
    if (!items) {
        printf("❌ Chyba alokace paměti!\n");
        fclose(overview);
        CloseHandle(hProcess);
        return 1;
    }
    
    printf("🔍 Analyzuji strukturu všech %d položek...\n", itemCount);
    
    for (int i = 0; i < itemCount; i++) {
        ItemOverview* item = &items[i];
        memset(item, 0, sizeof(ItemOverview));
        item->itemIndex = i;
        
        // Získej ItemData
        LRESULT itemData = SendMessage(listbox, LB_GETITEMDATA, i, 0);
        if (itemData == LB_ERR || itemData == 0) {
            continue;
        }
        
        item->itemDataAddr = (DWORD_PTR)itemData;
        
        // Přečti strukturu
        SIZE_T bytesRead;
        if (ReadProcessMemory(hProcess, (void*)itemData, item->rawStructure, 
                            sizeof(item->rawStructure), &bytesRead)) {
            
            // Standardní offsety z existujícího kódu
            item->position = item->rawStructure[1];
            item->flags = item->rawStructure[2];
            item->textPointer = item->rawStructure[5];  // Offset 20
            
            // Pokus se extrahovat text
            if (item->textPointer > 0x400000 && item->textPointer < 0x80000000) {
                item->hasValidText = ExtractTextAdvanced(hProcess, item->textPointer, 
                                                       item->extractedText, 
                                                       sizeof(item->extractedText));
            }
        }
        
        if ((i + 1) % 10 == 0) {
            printf("📊 Zpracováno %d/%d...\n", i+1, itemCount);
        }
    }
    
    // Vytvoř strukturovaný přehled
    fprintf(overview, "=== HIERARCHICKÁ STRUKTURA ===\n\n");
    
    for (int i = 0; i < itemCount; i++) {
        ItemOverview* item = &items[i];
        
        // Odsazení podle pozice/hierarchie
        int level = 0;
        if (item->position == 0) level = 0;
        else if (item->position <= 3) level = 1;
        else level = 2;
        
        // Odsazení
        for (int l = 0; l < level; l++) {
            fprintf(overview, "  ");
        }
        
        // Základní info
        fprintf(overview, "[%02d] ", i);
        
        if (item->hasValidText) {
            fprintf(overview, "%s", item->extractedText);
        } else {
            fprintf(overview, "<no text>");
        }
        
        fprintf(overview, "\n");
        
        // Detailní info pro debugging
        fprintf(overview, "     ItemData: 0x%p\n", (void*)item->itemDataAddr);
        fprintf(overview, "     Position: %u, Flags: 0x%X\n", item->position, item->flags);
        fprintf(overview, "     TextPtr: 0x%p\n", (void*)item->textPointer);
        
        // Hex dump prvních 8 DWORDs
        fprintf(overview, "     Structure: ");
        for (int j = 0; j < 8; j++) {
            fprintf(overview, "%08X ", item->rawStructure[j]);
        }
        fprintf(overview, "\n\n");
    }
    
    // Statistiky
    int textCount = 0;
    int folderCount = 0;
    int fileCount = 0;
    
    for (int i = 0; i < itemCount; i++) {
        if (items[i].hasValidText) textCount++;
        if (items[i].flags == 0x3604FD) folderCount++;
        if (items[i].flags == 0x704ED) fileCount++;
    }
    
    fprintf(overview, "\n=== STATISTIKY ===\n");
    fprintf(overview, "Celkem položek: %d\n", itemCount);
    fprintf(overview, "S platným textem: %d\n", textCount);
    fprintf(overview, "Složky (0x3604FD): %d\n", folderCount);
    fprintf(overview, "Soubory (0x704ED): %d\n", fileCount);
    
    // Analýza vzorců
    fprintf(overview, "\n=== ANALÝZA VZORCŮ ===\n");
    fprintf(overview, "Offset 0: ItemData base addresses\n");
    fprintf(overview, "Offset 4: Position values (0,1,2,3...)\n");
    fprintf(overview, "Offset 8: Flags (typ položky)\n");
    fprintf(overview, "Offset 20: Text pointer (nejdůležitější)\n");
    
    fclose(overview);
    free(items);
    CloseHandle(hProcess);
    
    printf("\n✅ STRUKTURÁLNÍ ANALÝZA DOKONČENA!\n");
    printf("📄 Přehled: listbox_structure_overview.txt\n");
    printf("📊 Statistiky:\n");
    printf("   - Celkem položek: %d\n", itemCount);
    printf("   - S platným textem: %d\n", textCount);
    printf("   - Složky: %d\n", folderCount);
    printf("   - Soubory: %d\n", fileCount);
    
    printf("\n💡 Nyní máte kompletní přehled struktury!\n");
    printf("💡 Soubor obsahuje hierarchii, vztahy a hex data pro ruční analýzu.\n");
    
    return 0;
}
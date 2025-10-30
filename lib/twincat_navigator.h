/**
 * twincat_navigator.h - Základní rozhraní pro práci s TwinCAT 2 Project Explorer
 * 
 * Obsahuje:
 * - Hledání TwinCAT okna a ListBox kontroly s project stromem
 * - Čtení paměti TwinCAT procesu pro získání položek stromu
 * - Extrakce informací o položkách (text, level, flags, hasChildren)
 * - Struktura TreeItem pro reprezentaci položky stromu
 * 
 * Konstanty flags určují typ položky:
 * - FLAG_FOLDER  (0x3604FD) - Rozbalitelné složky (POUs, Data Types, atd.)
 * - FLAG_FILE    (0x704ED)  - Soubory/listy (jednotlivé POU, proměnné)
 * - FLAG_SPECIAL (0x4047D)  - Speciální položky (GVLs, konfigurace)
 * - FLAG_ACTION  (0x504DD)  - Akce/funkce
 */

#ifndef TWINCAT_NAVIGATOR_H
#define TWINCAT_NAVIGATOR_H

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Konstanty
#define TEXT_POINTER_OFFSET 20
#define MAX_TEXT_LENGTH 256
#define MAX_ITEMS 200
#define STRUCT_FIELD_COUNT 16

// Typy položek podle flags
#define FLAG_FOLDER   0x3604FD  // Rozbalitelné složky  
#define FLAG_FILE     0x704ED   // Soubory
#define FLAG_SPECIAL  0x4047D   // Speciální položky
#define FLAG_ACTION   0x504DD   // Akce/funkce

// Struktura pro položku stromu v TwinCAT Project Explorer
typedef struct {
    int index;              // Index v ListBoxu (0 = první položka)
    char text[MAX_TEXT_LENGTH]; // Text položky (název POU, složky, atd.)
    DWORD_PTR itemData;     // Pointer na ItemData strukturu v paměti TwinCAT
    DWORD_PTR textPtr;      // Pointer na text v paměti (pokud je dostupný)
    DWORD raw[STRUCT_FIELD_COUNT]; // Surová data struktury pro analýzu (16 DWORD hodnot)
    int rawCount;           // Počet platných DWORD hodnot
    DWORD position;         // Pozice v hierarchii (odhad, nyní nepoužíváno)
    DWORD flags;            // Typ položky (FLAG_FOLDER, FLAG_FILE, atd.)
    int level;              // Úroveň odsazení (0 = root, 1 = POUs, 2 = MAIN, atd.)
    int hasChildren;        // 1 = má podpoložky, 0 = list
    int parentIndex;        // Index rodiče v poli položek
    int firstChild;         // Index prvního dítěte
    int nextSibling;        // Index dalšího sourozence
    const char* type;       // Typ jako string ("Folder", "File", atd.)
    const char* icon;       // Ikona (placeholder)
} TreeItem;

// Hlavní funkce pro navigaci v TwinCAT Project Explorer
HWND FindTwinCatWindow(void);                  // Najde TwinCAT okno podle titulku
HWND FindProjectListBox(HWND parentWindow);    // Najde ListBox s project stromem (scoring algoritmus)
HANDLE OpenTwinCatProcess(HWND hListBox);      // Otevře proces TwinCAT pro čtení paměti
int GetListBoxItemCount(HWND hListBox);        // Vrátí počet položek v ListBoxu
bool ExtractTreeItem(HANDLE hProcess, HWND hListBox, int index, TreeItem* item); // Načte položku z paměti
void PrintTreeStructure(TreeItem* items, int count); // Vytiskne stromovou strukturu do konzole
bool ExpandAllFolders(HWND hListBox, HANDLE hProcess);   // Rozbalí všechny složky (zastaralé)
bool CollapseAllFolders(HWND hListBox, HANDLE hProcess); // Zabalí všechny složky (zastaralé)
bool CloseFolder(HWND hListBox, int index);    // Zavře konkrétní složku
bool FocusOnItem(HWND hListBox, int index);    // Nastaví focus na položku
bool IsItemExpanded(HWND hListBox, HANDLE hProcess, int index);  // Zjistí, zda je položka otevřená (level comparison)
int GetFolderState(HWND hListBox, HANDLE hProcess, int index);   // Vrací stav složky: 1=otevřená, 0=zavřená, -1=chyba (používá structure[3])
int ToggleListBoxItem(HWND hListBox, int index);  // Vrací rozdíl v počtu položek (+ při otevření, - při zavření)
bool ExtractTargetFromTitle(const char* windowTitle, char* targetText, int maxLength);
int SearchInLevel(TreeItem* items, int count, const char* searchText);
int FindAndOpenPath(HWND hListBox, HANDLE hProcess, const char* searchText);
int FindItemByText(TreeItem* items, int count, const char* searchText);

// Memory analysis functions
bool AnalyzeFullMemoryStructure(HANDLE hProcess, const char* outputFileName);
bool SearchCompleteProjectStructure(HANDLE hProcess, const char* outputFileName);
bool SearchInMemoryDump(HANDLE hProcess, const char* searchText, char* outputFileName);
int ExtractAllItemsFromMemory(HANDLE hProcess, TreeItem* items, int maxItems);

#endif // TWINCAT_NAVIGATOR_H
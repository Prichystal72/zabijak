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

// Struktura pro položku stromu
typedef struct {
    int index;              // Index v ListBoxu
    char text[MAX_TEXT_LENGTH]; // Text položky
    DWORD_PTR itemData;     // Pointer na ItemData strukturu
    DWORD_PTR textPtr;      // Pointer na text v paměti (pokud je dostupný)
    DWORD raw[STRUCT_FIELD_COUNT]; // Surová data struktury pro analýzu
    int rawCount;           // Počet platných DWORD hodnot
    DWORD position;         // Pozice v hierarchii (odhad)
    DWORD flags;            // Typ položky
    int level;              // Úroveň odsazení
    int hasChildren;        // Má podpoložky
    int parentIndex;        // Index rodiče v poli položek
    int firstChild;         // Index prvního dítěte
    int nextSibling;        // Index dalšího sourozence
    const char* type;       // Typ jako string
    const char* icon;       // Ikona
} TreeItem;

// Hlavní funkce
HWND FindTwinCatWindow(void);
HWND FindProjectListBox(HWND parentWindow);
HANDLE OpenTwinCatProcess(HWND hListBox);
int GetListBoxItemCount(HWND hListBox);
bool ExtractTreeItem(HANDLE hProcess, HWND hListBox, int index, TreeItem* item);
void PrintTreeStructure(TreeItem* items, int count);
bool ExpandAllFolders(HWND hListBox, HANDLE hProcess);
bool CollapseAllFolders(HWND hListBox, HANDLE hProcess);
bool CloseFolder(HWND hListBox, int index);
bool FocusOnItem(HWND hListBox, int index);
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
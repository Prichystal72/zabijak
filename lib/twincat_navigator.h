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

// Typy položek podle flags
#define FLAG_FOLDER   0x3604FD  // Rozbalitelné složky  
#define FLAG_FILE     0x704ED   // Soubory
#define FLAG_SPECIAL  0x4047D   // Speciální položky
#define FLAG_ACTION   0x504DD   // Akce/funkce

// Struktura pro položku stromu
typedef struct {
    int index;              // Index v ListBoxu
    char text[MAX_TEXT_LENGTH]; // Text položky
    DWORD itemData;         // Pointer na ItemData strukturu
    DWORD textPtr;          // Pointer na text v paměti
    DWORD position;         // Pozice v hierarchii
    DWORD flags;            // Typ položky
    DWORD hasChildren;      // Má podpoložky
    int level;              // Úroveň odsazení
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
bool ExpandAllFolders(HWND hListBox);
bool FocusOnItem(HWND hListBox, int index);

#endif // TWINCAT_NAVIGATOR_H
#ifndef TWINCAT_TREE_H
#define TWINCAT_TREE_H

#include <windows.h>
#include <stdbool.h>
#include "twincat_navigator.h"

// Ziska vsechny viditelne polozky z ListBoxu
// Vraci: pocet nactenych polozek
int GetAllVisibleItems(HWND listbox, HANDLE hProcess, TreeItem* items, int maxItems);

// Najde polozku podle textu (case-insensitive)
// Vraci: index polozky nebo -1 pokud nenalezeno
int FindItemByText(TreeItem* items, int count, const char* text);

// Najde vsechny polozky na danem levelu
// Vraci: pocet nalezenych polozek
int FindItemsByLevel(TreeItem* items, int count, int level, int* foundIndices, int maxFound);

// Vytiskne stromovou strukturu
void PrintTreeStructure(TreeItem* items, int count);

// Ziska statistiku podle levelu
// Vraci: pole s pocty pro kazdy level (0-9)
void GetLevelStatistics(TreeItem* items, int count, int* levelCounts);

// Najde polozku v ListBoxu (i ve zlozkach, ktere otevre)
// Automaticky expanduje cestu k nalezene polozce
// Vraci: index polozky nebo -1 pokud nenalezeno
int FindAndExpandPath(HWND listbox, HANDLE hProcess, const char* searchText);

#endif // TWINCAT_TREE_H

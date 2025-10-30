/**
 * twincat_tree.h - Funkce pro práci se stromovou strukturou TwinCAT projektu
 * 
 * Obsahuje:
 * - Čtení všech viditelných položek z ListBoxu
 * - Vyhledávání položek podle textu nebo levelu
 * - FindAndExpandPath() - Klíčová funkce pro navigaci:
 *   1. Načte cache z JSON souboru
 *   2. Najde položku podle searchText
 *   3. Parsuje cestu (POUs/MAIN/ST_Funkce)
 *   4. Postupně expanduje složky v cestě
 *   5. Vrátí finální index nalezené položky
 */

#ifndef TWINCAT_TREE_H
#define TWINCAT_TREE_H

#include <windows.h>
#include <stdbool.h>
#include "twincat_navigator.h"

// Ziska vsechny viditelne polozky z ListBoxu (extrahuje TreeItem pro každou)
// Vraci: pocet nactenych polozek
int GetAllVisibleItems(HWND listbox, HANDLE hProcess, TreeItem* items, int maxItems);

// Najde polozku podle textu (case-insensitive porovnání přes _stricmp)
// Vraci: index polozky v poli items[] nebo -1 pokud nenalezeno
int FindItemByText(TreeItem* items, int count, const char* text);

// Najde vsechny polozky na danem levelu (0=root, 1=POUs, 2=MAIN, atd.)
// Vraci: pocet nalezenych polozek, jejich indexy jsou v foundIndices[]
int FindItemsByLevel(TreeItem* items, int count, int level, int* foundIndices, int maxFound);

// Vytiskne stromovou strukturu s odsazením podle levelu do konzole
void PrintTreeStructure(TreeItem* items, int count);

// Ziska statistiku podle levelu (kolik položek je na level 0, 1, 2, atd.)
// Vraci: pole s pocty pro kazdy level (0-9)
void GetLevelStatistics(TreeItem* items, int count, int* levelCounts);

// KLÍČOVÁ FUNKCE: Najde položku v cache a expanduje cestu k ní
// 1. Načte cache z JSON souboru (twincat_tree_cache.json)
// 2. Vyhledá searchText v cache (case-insensitive)
// 3. Parsuje cestu (POUs/MAIN/ST_Funkce) na jednotlivé části
// 4. Postupně expanduje složky v cestě (SendMessage LB_SETCURSEL + double-click)
// 5. Po každém expandování čeká na změnu počtu položek v ListBoxu
// Vraci: index nalezené polozky v ListBoxu nebo -1 pokud nenalezeno
int FindAndExpandPath(HWND listbox, HANDLE hProcess, const char* searchText);

#endif // TWINCAT_TREE_H

/**
 * twincat_search.h - Rozhraní pro extrakci názvu POU z titulku TwinCAT okna
 * 
 * Hlavní funkce:
 * - ExtractTargetFromTitle() - Parsuje titulek okna a extrahuje název POU
 *   Podporované formáty:
 *   - "MAIN (PRG) - TwinCAT PLC Control" -> extrahuje "MAIN"
 *   - "ST_Funkce (FB) - TwinCAT..." -> extrahuje "ST_Funkce"
 *   - "GVL_Variables (GVL) - TwinCAT..." -> extrahuje "GVL_Variables"
 * 
 * Poznámka: TwinCatSearchAndActivate() je starší funkce, není aktuálně používána
 */

#ifndef TWINCAT_SEARCH_H
#define TWINCAT_SEARCH_H

#include <windows.h>
#include <stdbool.h>

// Návratové kódy
#define SEARCH_SUCCESS          0  // Cíl nalezen a aktivován
#define SEARCH_NOT_FOUND        1  // Cíl nebyl nalezen v projektu
#define SEARCH_INVALID_PARAMS   2  // Neplatné vstupní parametry
#define SEARCH_PROCESS_ERROR    3  // Chyba při přístupu k procesu
#define SEARCH_ROOT_ERROR       4  // Chyba přípravy kořenového uzlu

// Maximální délka výstupního řetězce
#define SEARCH_MAX_PATH 512

/**
 * Extrahuje název POU/funkce z titulku TwinCAT okna
 * 
 * Příklady:
 * - "MAIN (PRG) - TwinCAT PLC Control" -> "MAIN"
 * - "ST_Markiere_WT_NIO (FB) - TwinCAT..." -> "ST_Markiere_WT_NIO"
 * 
 * @param windowTitle  Titulek okna (získaný pomocí GetWindowText)
 * @param target       Výstupní buffer pro extrahovaný název
 * @param maxLen       Maximální délka výstupního bufferu
 * @return true pokud se podařilo extrahovat, false pokud formát neodpovídá
 */
bool ExtractTargetFromTitle(const char* windowTitle, char* target, int maxLen);

/**
 * ZASTARALÁ FUNKCE - není aktuálně používána
 * Vyhledá položku v TwinCAT project exploreru a aktivuje ji.
 * 
 * @param hListBox      Handle na ListBox TwinCAT project exploreru
 * @param targetName    Název hledané položky (např. "ST_Markiere_WT_NIO")
 * @param resultPath    Výstupní buffer pro cestu k nalezené položce (může být NULL)
 * @param maxPathLen    Maximální délka výstupního bufferu
 * 
 * @return Návratový kód (SEARCH_SUCCESS, SEARCH_NOT_FOUND, atd.)
 */
int TwinCatSearchAndActivate(
    HWND hListBox,
    const char* targetName,
    char* resultPath,
    int maxPathLen
);

#endif // TWINCAT_SEARCH_H

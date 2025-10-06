#include <windows.h>
#include <stdio.h>
#include "../lib/twincat_navigator.h"

/**
 * Jednoduchý test: Otevře a zavře položku na indexu 6
 * Používá knihovní funkci ToggleListBoxItem()
 */

int main() {
    printf("=== TEST: Otevření a zavření položky 6 ===\n\n");
    
    HWND twincatWindow = FindTwinCatWindow();
    if (!twincatWindow) {
        printf("❌ TwinCAT okno nenalezeno!\n");
        return 1;
    }
    
    HWND listbox = FindProjectListBox(twincatWindow);
    if (!listbox) {
        printf("❌ Project ListBox nenalezen!\n");
        return 1;
    }
    
    DWORD processId;
    GetWindowThreadProcessId(listbox, &processId);
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) {
        printf("❌ Nelze otevřít proces!\n");
        return 1;
    }
    
    int count = SendMessage(listbox, LB_GETCOUNT, 0, 0);
    printf("📊 ListBox obsahuje %d položek\n", count);
    
    if (count < 7) {
        printf("⚠️ ListBox má méně než 7 položek. Index 6 neexistuje.\n");
        CloseHandle(hProcess);
        return 1;
    }
    
    // Získej text položky 6
    char text[256] = {0};
    SendMessage(listbox, LB_GETTEXT, 6, (LPARAM)text);
    printf("📋 Položka 6: '%s'\n", text);
    
    // Zjisti aktuální stav
    bool isExpanded = IsItemExpanded(listbox, hProcess, 6);
    printf("📂 Stav před: %s\n\n", isExpanded ? "OTEVŘENÁ" : "ZAVŘENÁ");
    
    printf("▶️ Přepínám položku 6...\n");
    int delta = ToggleListBoxItem(listbox, 6);
    if (delta > 0) {
        printf("✅ Otevřeno! Přibylo %d položek pod větví.\n", delta);
    } else if (delta < 0) {
        printf("✅ Zavřeno! Ubylo %d položek.\n", -delta);
    } else {
        printf("⚠️ Žádná změna (není to složka nebo chyba).\n");
    }
    
    // Zjisti nový stav
    isExpanded = IsItemExpanded(listbox, hProcess, 6);
    printf("📂 Stav po: %s\n\n", isExpanded ? "OTEVŘENÁ" : "ZAVŘENÁ");
    
    printf("⏸️ Pauza 2 sekundy...\n\n");
    Sleep(2000);
    
    printf("🔽 Přepínám zpět položku 6...\n");
    delta = ToggleListBoxItem(listbox, 6);
    if (delta > 0) {
        printf("✅ Otevřeno! Přibylo %d položek pod větví.\n", delta);
    } else if (delta < 0) {
        printf("✅ Zavřeno! Ubylo %d položek.\n", -delta);
    } else {
        printf("⚠️ Žádná změna.\n");
    }
    
    // Finální stav
    isExpanded = IsItemExpanded(listbox, hProcess, 6);
    printf("📂 Finální stav: %s\n", isExpanded ? "OTEVŘENÁ" : "ZAVŘENÁ");
    
    CloseHandle(hProcess);
    
    printf("\n✅ Test dokončen!\n");
    return 0;
}

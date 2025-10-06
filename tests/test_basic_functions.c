#include <windows.h>
#include <stdio.h>
#include "../lib/twincat_navigator.h"

/**
 * Test základních funkcí, které fungují spolehlivě:
 * 1. FindTwinCatWindow() - nalezení TwinCAT okna
 * 2. FindProjectListBox() - nalezení project explorer ListBoxu
 * 3. OpenTwinCatProcess() - otevření procesu pro čtení paměti
 * 4. GetListBoxItemCount() - zjištění počtu položek
 * 5. ExtractTreeItem() - načtení dat o položce
 */

int main() {
    printf("===================================================\n");
    printf("  TEST SPOLEHLIVYCH ZAKLADNICH FUNKCI\n");
    printf("===================================================\n\n");
    
    // ========================================
    // 1. NAJDI TWINCAT OKNO
    // ========================================
    printf("1. FindTwinCatWindow()\n");
    printf("   Hledam TwinCAT okno...\n");
    
    HWND twincatWindow = FindTwinCatWindow();
    if (!twincatWindow) {
        printf("   [X] SELHALO: TwinCAT okno nenalezeno!\n");
        return 1;
    }
    
    char windowTitle[512];
    GetWindowText(twincatWindow, windowTitle, sizeof(windowTitle));
    printf("   [OK] USPECH: 0x%p - '%s'\n\n", twincatWindow, windowTitle);
    
    // ========================================
    // 2. NAJDI PROJECT LISTBOX
    // ========================================
    printf("2. FindProjectListBox()\n");
    printf("   Hledam Project Explorer ListBox...\n");
    
    HWND listbox = FindProjectListBox(twincatWindow);
    if (!listbox) {
        printf("   [X] SELHALO: ListBox nenalezen!\n");
        return 1;
    }
    
    RECT rect;
    GetWindowRect(listbox, &rect);
    printf("   [OK] USPECH: 0x%p (pozice: %d,%d, velikost: %dx%d)\n\n", 
           listbox, rect.left, rect.top, 
           rect.right - rect.left, rect.bottom - rect.top);
    
    // ========================================
    // 3. OTEVRI PROCES PRO CTENI PAMETI
    // ========================================
    printf("3. OpenTwinCatProcess()\n");
    printf("   Oteviram TwinCAT proces s PROCESS_VM_READ...\n");
    
    HANDLE hProcess = OpenTwinCatProcess(listbox);
    if (!hProcess) {
        printf("   [X] SELHALO: Nelze otevrit proces!\n");
        return 1;
    }
    
    DWORD processId;
    GetWindowThreadProcessId(listbox, &processId);
    printf("   [OK] USPECH: PID=%d, Handle=0x%p\n\n", processId, hProcess);
    
    // ========================================
    // 4. ZJISTI POCET POLOZEK
    // ========================================
    printf("4. GetListBoxItemCount()\n");
    printf("   Pocitam viditelne polozky v ListBoxu...\n");
    
    int itemCount = GetListBoxItemCount(listbox);
    if (itemCount <= 0) {
        printf("   [X] SELHALO: Zadne polozky (count=%d)\n", itemCount);
        CloseHandle(hProcess);
        return 1;
    }
    
    printf("   [OK] USPECH: %d viditelnych polozek\n\n", itemCount);
    
    // ========================================
    // 5. NACTI DATA O POLOZKACH
    // ========================================
    printf("5. ExtractTreeItem()\n");
    printf("   Nacitam data prvnich 5 polozek...\n");
    
    int successCount = 0;
    for (int i = 0; i < 5 && i < itemCount; i++) {
        TreeItem item;
        if (ExtractTreeItem(hProcess, listbox, i, &item)) {
            printf("   [%d] [OK] %s '%s' (level=%d, flags=0x%X)\n", 
                   i, item.icon, item.text, item.level, item.flags);
            successCount++;
        } else {
            printf("   [%d] [!] Nelze nacist\n", i);
        }
    }
    
    if (successCount == 0) {
        printf("   [X] SELHALO: Zadna polozka nenactena!\n");
        CloseHandle(hProcess);
        return 1;
    }
    
    printf("   [OK] USPECH: %d/%d polozek nacteno\n\n", successCount, 5);
    
    // ========================================
    // SOUHRN
    // ========================================
    printf("===================================================\n");
    printf("  VYSLEDEK TESTU\n");
    printf("===================================================\n");
    printf("  [OK] FindTwinCatWindow()    - FUNGUJE\n");
    printf("  [OK] FindProjectListBox()   - FUNGUJE\n");
    printf("  [OK] OpenTwinCatProcess()   - FUNGUJE\n");
    printf("  [OK] GetListBoxItemCount()  - FUNGUJE\n");
    printf("  [OK] ExtractTreeItem()      - FUNGUJE\n");
    printf("===================================================\n");
    printf("  Vsechny zakladni funkce funguji spolehlive!\n");
    printf("===================================================\n");
    
    CloseHandle(hProcess);
    return 0;
}

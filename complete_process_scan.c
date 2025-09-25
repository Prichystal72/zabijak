#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <commctrl.h>

typedef struct {
    DWORD processId;
    char processName[256];
    char fullPath[512];
} ProcessInfo;

ProcessInfo allProcesses[100];
int processCount = 0;

// Callback pro hledání TreeView v oknech
BOOL CALLBACK FindTreeViewInWindows(HWND hwnd, LPARAM lParam) {
    DWORD processId = (DWORD)lParam;
    DWORD windowProcessId;
    
    GetWindowThreadProcessId(hwnd, &windowProcessId);
    
    if (windowProcessId == processId) {
        char className[256];
        char windowText[1024];
        RECT rect;
        
        GetClassName(hwnd, className, sizeof(className));
        GetWindowText(hwnd, windowText, sizeof(windowText));
        GetWindowRect(hwnd, &rect);
        
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        
        // Zobrazit jen rozumně velká okna
        if (width > 50 && height > 50 && IsWindowVisible(hwnd)) {
            printf("    Okno: %08p [%dx%d] '%s'", hwnd, width, height, className);
            if (strlen(windowText) > 0) {
                printf(" - '%s'", windowText);
            }
            printf("\n");
            
            // Rekurzivní hledání TreeView v child oknech
            HWND hChild = GetWindow(hwnd, GW_CHILD);
            int childCount = 0;
            while (hChild && childCount < 25) {
                char childClass[256];
                char childText[256];
                GetClassName(hChild, childClass, sizeof(childClass));
                GetWindowText(hChild, childText, sizeof(childText));
                
                // Hledáme TreeView
                if (strstr(childClass, "Tree") != NULL || strstr(childClass, "SysTree") != NULL) {
                    printf("      *** TREE VIEW NALEZEN: %08p '%s' ***\n", hChild, childClass);
                    
                    int itemCount = SendMessage(hChild, TVM_GETCOUNT, 0, 0);
                    printf("      Počet položek: %d\n", itemCount);
                    
                    if (itemCount > 0 && itemCount < 200) {
                        printf("      >>> MOŽNÝ PROJEKTOVÝ STROM! <<<\n");
                        
                        // Zkusme zobrazit první pár položek
                        HTREEITEM hRoot = (HTREEITEM)SendMessage(hChild, TVM_GETNEXTITEM, TVGN_ROOT, 0);
                        if (hRoot) {
                            TVITEM tvi;
                            char buffer[256];
                            tvi.mask = TVIF_TEXT;
                            tvi.hItem = hRoot;
                            tvi.pszText = buffer;
                            tvi.cchTextMax = sizeof(buffer);
                            
                            if (SendMessage(hChild, TVM_GETITEM, 0, (LPARAM)&tvi)) {
                                printf("      Root: '%s'\n", buffer);
                            }
                        }
                    }
                }
                
                // Kontrola i jiných užitečných ovládacích prvků
                if (strstr(childClass, "List") != NULL && strstr(childClass, "View") != NULL) {
                    printf("      List View: %08p '%s'\n", hChild, childClass);
                    
                    int items = SendMessage(hChild, LVM_GETITEMCOUNT, 0, 0);
                    if (items > 0) {
                        printf("      Položek: %d\n", items);
                    }
                }
                
                hChild = GetWindow(hChild, GW_HWNDNEXT);
                childCount++;
            }
        }
    }
    
    return TRUE;
}

void ScanAllProcesses() {
    printf("=== HLEDÁNÍ VŠECH CODESYS/TWINCAT PROCESŮ ===\n");
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return;
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    processCount = 0;
    
    if (Process32First(hSnapshot, &pe32)) {
        do {
            // Hledáme procesy s relevantními názvy
            if (strstr(pe32.szExeFile, "TwinCAT") != NULL ||
                strstr(pe32.szExeFile, "CoDeSys") != NULL ||
                strstr(pe32.szExeFile, "Tcat") != NULL ||
                strstr(pe32.szExeFile, "Plc") != NULL ||
                strstr(pe32.szExeFile, "plc") != NULL ||
                strstr(pe32.szExeFile, "Project") != NULL ||
                strstr(pe32.szExeFile, "Manager") != NULL ||
                strstr(pe32.szExeFile, "Navigator") != NULL) {
                
                allProcesses[processCount].processId = pe32.th32ProcessID;
                strcpy(allProcesses[processCount].processName, pe32.szExeFile);
                
                // Zkusit získat plnou cestu procesu
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
                if (hProcess) {
                    char fullPath[512] = {0};
                    if (GetModuleFileNameEx(hProcess, NULL, fullPath, sizeof(fullPath))) {
                        strcpy(allProcesses[processCount].fullPath, fullPath);
                    }
                    CloseHandle(hProcess);
                }
                
                printf("[%d] %s (PID: %d)\n", processCount + 1, pe32.szExeFile, pe32.th32ProcessID);
                if (strlen(allProcesses[processCount].fullPath) > 0) {
                    printf("    Cesta: %s\n", allProcesses[processCount].fullPath);
                }
                
                processCount++;
                if (processCount >= 100) break;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
    printf("Nalezeno %d relevantních procesů.\n", processCount);
}

int main() {
    printf("=== KOMPLETNÍ SCANNER CODESYS/TWINCAT PROCESŮ ===\n");
    
    ScanAllProcesses();
    
    printf("\n=== ANALÝZA OKEN A HLEDÁNÍ TREE VIEW ===\n");
    for (int i = 0; i < processCount; i++) {
        printf("\n[%d] Analyzuji: %s (PID: %d)\n", i+1, 
               allProcesses[i].processName, allProcesses[i].processId);
        
        EnumWindows(FindTreeViewInWindows, allProcesses[i].processId);
    }
    
    printf("\nAnalýza dokončena. Stiskněte Enter...\n");
    getchar();
    return 0;
}
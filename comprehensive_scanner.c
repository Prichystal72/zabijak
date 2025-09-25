#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <commctrl.h>
#include <tlhelp32.h>

// Scanner všech procesů hledající TwinCAT/CoDeSys aplikace
void ScanProcesses() {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    
    printf("=== SCANNING RUNNING PROCESSES ===\n");
    
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        printf("CreateToolhelp32Snapshot failed!\n");
        return;
    }
    
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (!Process32First(hProcessSnap, &pe32)) {
        printf("Process32First failed!\n");
        CloseHandle(hProcessSnap);
        return;
    }
    
    int processCount = 0;
    do {
        // Hledáme procesy obsahující TwinCAT/CoDeSys klíčová slova
        if (strstr(pe32.szExeFile, "CoDeSys") != NULL ||
            strstr(pe32.szExeFile, "TwinCAT") != NULL ||
            strstr(pe32.szExeFile, "PLC") != NULL ||
            strstr(pe32.szExeFile, "Project") != NULL ||
            strstr(pe32.szExeFile, "Manager") != NULL) {
            
            processCount++;
            printf("[%d] Process: %s (PID: %d)\n", 
                   processCount, pe32.szExeFile, pe32.th32ProcessID);
        }
    } while (Process32Next(hProcessSnap, &pe32));
    
    CloseHandle(hProcessSnap);
    printf("Found %d potentially related processes.\n\n", processCount);
}

// Rozšířený scanner všech oken bez omezení na konkrétní proces
BOOL CALLBACK ScanAllWindowsExtended(HWND hwnd, LPARAM lParam) {
    char className[256];
    char windowText[1024];
    RECT rect;
    DWORD processId;
    
    GetClassName(hwnd, className, sizeof(className));
    GetWindowText(hwnd, windowText, sizeof(windowText));
    GetWindowRect(hwnd, &rect);
    GetWindowThreadProcessId(hwnd, &processId);
    
    // Hledáme okna s CoDeSys/TwinCAT/Project klíčovými slovy
    // ROZŠÍŘENÉ HLEDÁNÍ - více variant
    if (strstr(windowText, "CoDeSys") != NULL || 
        strstr(className, "CoDeSys") != NULL ||
        strstr(windowText, "TwinCAT") != NULL ||
        strstr(className, "TwinCAT") != NULL ||
        strstr(windowText, "PLC") != NULL ||
        strstr(windowText, "Project") != NULL ||
        strstr(windowText, "Manager") != NULL ||
        strstr(windowText, "Navigator") != NULL ||
        strstr(windowText, "Workspace") != NULL ||
        strstr(windowText, "Library") != NULL ||
        (strlen(windowText) > 0 && strstr(windowText, ".pro") != NULL)) {
        
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        
        printf("=== POTENTIAL MATCH ===\n");
        printf("Handle: %08p (PID: %d)\n", hwnd, processId);
        printf("Class: '%s'\n", className);
        printf("Title: '%s'\n", windowText);
        printf("Size: [%dx%d]\n", width, height);
        printf("Visible: %s\n", IsWindowVisible(hwnd) ? "YES" : "NO");
        
        // Pokud má rozumnou velikost, prohledat child okna
        if (width > 100 && height > 100) {
            printf("=== CHILD WINDOWS SCAN ===\n");
            
            HWND hChild = GetWindow(hwnd, GW_CHILD);
            int childCount = 0;
            
            while (hChild && childCount < 30) {
                char childClass[256];
                char childText[256];
                RECT childRect;
                
                GetClassName(hChild, childClass, sizeof(childClass));
                GetWindowText(hChild, childText, sizeof(childText));
                GetWindowRect(hChild, &childRect);
                
                int childWidth = childRect.right - childRect.left;
                int childHeight = childRect.bottom - childRect.top;
                
                printf("  [%d] %08p '%-20s' [%dx%d]", 
                       childCount, hChild, childClass, childWidth, childHeight);
                
                if (strlen(childText) > 0) {
                    printf(" '%s'", childText);
                }
                printf("\n");
                
                // EXTRA pozornost pro TreeView!
                if (strstr(childClass, "Tree") != NULL || 
                    strstr(childClass, "SysTree") != NULL) {
                    
                    printf("    *** TREE VIEW DISCOVERED! ***\n");
                    
                    int itemCount = SendMessage(hChild, TVM_GETCOUNT, 0, 0);
                    printf("    Tree item count: %d\n", itemCount);
                    
                    if (itemCount > 0) {
                        // Zkusme získat root a první few items
                        HTREEITEM hRoot = (HTREEITEM)SendMessage(hChild, TVM_GETNEXTITEM, TVGN_ROOT, 0);
                        if (hRoot) {
                            printf("    Root item handle: %p\n", hRoot);
                            
                            // Získat text root itemu
                            TVITEM tvi;
                            char buffer[256];
                            tvi.mask = TVIF_TEXT;
                            tvi.hItem = hRoot;
                            tvi.pszText = buffer;
                            tvi.cchTextMax = sizeof(buffer);
                            
                            if (SendMessage(hChild, TVM_GETITEM, 0, (LPARAM)&tvi)) {
                                printf("    Root text: '%s'\n", buffer);
                                
                                // Získat child items
                                HTREEITEM hChildItem = (HTREEITEM)SendMessage(hChild, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)hRoot);
                                int itemNum = 0;
                                while (hChildItem && itemNum < 10) {
                                    tvi.hItem = hChildItem;
                                    if (SendMessage(hChild, TVM_GETITEM, 0, (LPARAM)&tvi)) {
                                        printf("    Child %d: '%s'\n", itemNum, buffer);
                                    }
                                    hChildItem = (HTREEITEM)SendMessage(hChild, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hChildItem);
                                    itemNum++;
                                }
                            }
                        }
                    }
                }
                
                hChild = GetWindow(hChild, GW_HWNDNEXT);
                childCount++;
            }
            printf("=== END CHILD SCAN ===\n");
        }
        printf("\n");
    }
    
    return TRUE;
}

int main() {
    printf("=== COMPREHENSIVE TWINCAT/CODESYS SCANNER ===\n");
    printf("Scanning processes and all windows...\n\n");
    
    // 1. Scan processes
    ScanProcesses();
    
    // 2. Scan all windows from all processes
    printf("=== SCANNING ALL WINDOWS ===\n");
    EnumWindows(ScanAllWindowsExtended, 0);
    
    printf("\nScan completed. Press Enter to exit...\n");
    getchar();
    return 0;
}
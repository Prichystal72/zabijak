#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <commctrl.h>

DWORD twinCatProcessId = 0;
FILETIME twinCatStartTime = {0};
char targetUnit[256] = {0};

typedef struct {
    DWORD processId;
    char processName[256];
    FILETIME startTime;
    DWORD timeDiffSeconds;
} ProcessInfo;

ProcessInfo relatedProcesses[50];
int relatedProcessCount = 0;

DWORD CompareFileTimes(FILETIME* ft1, FILETIME* ft2) {
    ULARGE_INTEGER uli1, uli2;
    uli1.LowPart = ft1->dwLowDateTime;
    uli1.HighPart = ft1->dwHighDateTime;
    uli2.LowPart = ft2->dwLowDateTime;
    uli2.HighPart = ft2->dwHighDateTime;
    
    if (uli1.QuadPart > uli2.QuadPart) {
        return (DWORD)((uli1.QuadPart - uli2.QuadPart) / 10000000);
    } else {
        return (DWORD)((uli2.QuadPart - uli1.QuadPart) / 10000000);
    }
}

BOOL CALLBACK FindTwinCatWindow(HWND hwnd, LPARAM lParam) {
    char title[1024];
    GetWindowText(hwnd, title, sizeof(title));
    
    if (strstr(title, "TwinCAT PLC Control") != NULL && strstr(title, "[") != NULL) {
        printf("=== NALEZENO TWINCAT OKNO ===\n");
        printf("Titulek: %s\n", title);
        
        char *start = strchr(title, '[');
        char *end = strchr(title, ']');
        if (start && end && end > start) {
            strncpy(targetUnit, start + 1, end - start - 1);
            targetUnit[end - start - 1] = '\0';
            printf("Cílová jednotka: '%s'\n", targetUnit);
        }
        
        GetWindowThreadProcessId(hwnd, &twinCatProcessId);
        printf("Process ID: %d\n", twinCatProcessId);
        
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, twinCatProcessId);
        if (hProcess) {
            FILETIME createTime, exitTime, kernelTime, userTime;
            if (GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime)) {
                twinCatStartTime = createTime;
                
                SYSTEMTIME st;
                FileTimeToSystemTime(&createTime, &st);
                printf("Čas spuštění: %02d:%02d:%02d\n", st.wHour, st.wMinute, st.wSecond);
            }
            CloseHandle(hProcess);
        }
        
        return FALSE;
    }
    
    return TRUE;
}

void ScanRelatedProcesses() {
    printf("\n=== PROCESY SE STEJNÝM ČASEM SPUŠTĚNÍ ===\n");
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return;
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    relatedProcessCount = 0;
    
    if (Process32First(hSnapshot, &pe32)) {
        do {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
            if (hProcess) {
                FILETIME createTime, exitTime, kernelTime, userTime;
                if (GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime)) {
                    
                    DWORD timeDiff = CompareFileTimes(&createTime, &twinCatStartTime);
                    
                    if (timeDiff <= 30 && relatedProcessCount < 50) {
                        relatedProcesses[relatedProcessCount].processId = pe32.th32ProcessID;
                        strcpy(relatedProcesses[relatedProcessCount].processName, pe32.szExeFile);
                        relatedProcesses[relatedProcessCount].startTime = createTime;
                        relatedProcesses[relatedProcessCount].timeDiffSeconds = timeDiff;
                        relatedProcessCount++;
                        
                        SYSTEMTIME st;
                        FileTimeToSystemTime(&createTime, &st);
                        printf("[%d] PID: %5d, +%2ds, %02d:%02d:%02d, %s\n", 
                               relatedProcessCount, pe32.th32ProcessID, timeDiff, 
                               st.wHour, st.wMinute, st.wSecond, pe32.szExeFile);
                    }
                }
                CloseHandle(hProcess);
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
    printf("Nalezeno %d procesů.\n", relatedProcessCount);
}

BOOL CALLBACK AnalyzeProcessWindows(HWND hwnd, LPARAM lParam) {
    DWORD processId = (DWORD)lParam;
    DWORD windowProcessId;
    
    GetWindowThreadProcessId(hwnd, &windowProcessId);
    
    if (windowProcessId == processId && IsWindowVisible(hwnd)) {
        char className[256];
        char windowText[1024];
        RECT rect;
        
        GetClassName(hwnd, className, sizeof(className));
        GetWindowText(hwnd, windowText, sizeof(windowText));
        GetWindowRect(hwnd, &rect);
        
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        
        if (width > 100 && height > 100) {
            printf("    Okno: %08p [%dx%d] '%s'", hwnd, width, height, className);
            if (strlen(windowText) > 0) {
                printf(" - '%s'", windowText);
            }
            printf("\n");
            
            // Rychlé hledání TreeView
            HWND hChild = GetWindow(hwnd, GW_CHILD);
            int childCount = 0;
            while (hChild && childCount < 15) {
                char childClass[256];
                GetClassName(hChild, childClass, sizeof(childClass));
                
                if (strstr(childClass, "Tree") != NULL) {
                    printf("      *** TREE VIEW: %08p ***\n", hChild);
                    
                    int itemCount = SendMessage(hChild, TVM_GETCOUNT, 0, 0);
                    printf("      Položek: %d\n", itemCount);
                }
                
                hChild = GetWindow(hChild, GW_HWNDNEXT);
                childCount++;
            }
        }
    }
    
    return TRUE;
}

int main() {
    printf("=== SCANNER SOUVISEJÍCÍCH PROCESŮ ===\n");
    
    EnumWindows(FindTwinCatWindow, 0);
    
    if (twinCatProcessId == 0) {
        printf("TwinCAT okno nenalezeno!\n");
        getchar();
        return 1;
    }
    
    ScanRelatedProcesses();
    
    printf("\n=== ANALÝZA OKEN ===\n");
    for (int i = 0; i < relatedProcessCount; i++) {
        printf("\n[%d] %s (PID: %d)\n", i+1, 
               relatedProcesses[i].processName, relatedProcesses[i].processId);
        
        EnumWindows(AnalyzeProcessWindows, relatedProcesses[i].processId);
    }
    
    printf("\nDokončeno. Enter...\n");
    getchar();
    return 0;
}
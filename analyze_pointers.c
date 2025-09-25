#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

void dump_memory_at(HANDLE hProcess, DWORD addr, const char* label) {
    printf("\n=== %s (0x%08X) ===\n", label, addr);
    
    char buffer[256] = {0};
    wchar_t wbuffer[128] = {0};
    SIZE_T bytesRead;
    
    // Zkus ANSI string
    if (ReadProcessMemory(hProcess, (void*)addr, buffer, sizeof(buffer)-1, &bytesRead)) {
        printf("ANSI: '");
        for (size_t i = 0; i < bytesRead && i < 100; i++) {
            if (buffer[i] == 0) break;
            if (buffer[i] >= 32 && buffer[i] <= 126) {
                printf("%c", buffer[i]);
            } else {
                printf("\\x%02X", (unsigned char)buffer[i]);
            }
        }
        printf("'\n");
    }
    
    // Zkus Unicode string
    if (ReadProcessMemory(hProcess, (void*)addr, wbuffer, sizeof(wbuffer)-2, &bytesRead)) {
        printf("Unicode: '");
        for (size_t i = 0; i < bytesRead/2 && i < 50; i++) {
            if (wbuffer[i] == 0) break;
            if (wbuffer[i] >= 32 && wbuffer[i] <= 126) {
                printf("%c", (char)wbuffer[i]);
            } else {
                printf("\\u%04X", wbuffer[i]);
            }
        }
        printf("'\n");
    }
    
    // Hex dump
    unsigned char hexbuf[64];
    if (ReadProcessMemory(hProcess, (void*)addr, hexbuf, sizeof(hexbuf), &bytesRead)) {
        printf("Hex: ");
        for (size_t i = 0; i < bytesRead && i < 32; i++) {
            printf("%02X ", hexbuf[i]);
        }
        printf("\n");
    }
}

int main() {
    printf("=== RUČNÍ ANALÝZA POINTERŮ V PRVNÍ POLOŽCE ===\n");
    
    // Najdi TwinCAT
    HWND hTwinCAT = NULL;
    HWND hWnd = GetTopWindow(GetDesktopWindow());
    
    while (hWnd) {
        char windowTitle[512];
        if (GetWindowText(hWnd, windowTitle, sizeof(windowTitle))) {
            if (strstr(windowTitle, "TwinCAT") != NULL) {
                hTwinCAT = hWnd;
                break;
            }
        }
        hWnd = GetNextWindow(hWnd, GW_HWNDNEXT);
    }
    
    if (!hTwinCAT) {
        printf("❌ TwinCAT nenalezen!\n");
        return 1;
    }
    
    // ListBox
    HWND hListBox = (HWND)0x00070400;
    if (!IsWindow(hListBox)) {
        printf("❌ ListBox neplatný!\n");
        return 1;
    }
    
    // Proces
    DWORD processId;
    GetWindowThreadProcessId(hListBox, &processId);
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId);
    
    if (!hProcess) {
        printf("❌ Nelze otevřít proces\n");
        return 1;
    }
    
    // Analyzuj konkrétní pointery z hex dumpu
    dump_memory_at(hProcess, 0x095991C8, "Pointer na offset 0x14");
    dump_memory_at(hProcess, 0x095999A8, "Pointer na offset 0x34");
    dump_memory_at(hProcess, 0x095990A8, "Pointer na offset 0xAC");
    
    // Zkusme také analyzovat strukturu jako celek
    printf("\n=== ANALÝZA JAKO STRUCT ARRAY ===\n");
    
    LRESULT itemData = SendMessage(hListBox, LB_GETITEMDATA, 0, 0);
    unsigned char buffer[200];
    SIZE_T bytesRead;
    
    if (ReadProcessMemory(hProcess, (void*)itemData, buffer, sizeof(buffer), &bytesRead)) {
        // Interpretuj jako array of pointers
        DWORD* ptrs = (DWORD*)buffer;
        int ptrCount = bytesRead / 4;
        
        printf("Všechny 32-bit hodnoty jako možné pointery:\n");
        for (int i = 0; i < ptrCount && i < 50; i++) {
            DWORD addr = ptrs[i];
            if (addr > 0x400000 && addr < 0x80000000) {
                printf("[%02d] 0x%08X: ", i * 4, addr);
                
                char testStr[100] = {0};
                SIZE_T strRead;
                if (ReadProcessMemory(hProcess, (void*)addr, testStr, sizeof(testStr)-1, &strRead)) {
                    bool hasText = false;
                    for (size_t j = 0; j < strRead && j < 20; j++) {
                        if (testStr[j] == 0) break;
                        if (testStr[j] >= 32 && testStr[j] <= 126) {
                            if (!hasText) printf("'");
                            hasText = true;
                            printf("%c", testStr[j]);
                        } else if (hasText) {
                            break;
                        }
                    }
                    if (hasText) printf("'");
                }
                printf("\n");
            }
        }
    }
    
    CloseHandle(hProcess);
    
    printf("\nStiskněte Enter...\n");
    getchar();
    return 0;
}
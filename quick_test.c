#include <windows.h>
#include <stdio.h>

int main() {
    printf("=== JEDNODUCHÝ TEST ČTENÍ PAMĚTI ===\n");
    
    // Získej TwinCAT process
    HWND hwnd = FindWindow(NULL, "TwinCAT PLC Control - Palettierer.pro - [ST03_Main (PRG-SFC)]");
    if (!hwnd) {
        printf("TwinCAT okno nenalezeno!\n");
        getchar();
        return 1;
    }
    
    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId);
    
    if (!hProcess) {
        printf("Nelze otevřít proces! Chyba: %d\n", GetLastError());
        getchar();
        return 1;
    }
    
    printf("Process otevřen (PID: %d)\n", processId);
    
    // Test čtení z první adresy (0x094CE268)
    DWORD address = 0x094CE268;
    printf("\nTestuji čtení z adresy: 0x%08X\n", address);
    
    unsigned char buffer[256];
    SIZE_T bytesRead = 0;
    
    if (ReadProcessMemory(hProcess, (LPCVOID)address, buffer, sizeof(buffer), &bytesRead)) {
        printf("Úspěšně přečteno %zu bytů:\n", bytesRead);
        
        // Hex dump
        for (int i = 0; i < min(64, bytesRead); i += 16) {
            printf("%04X: ", i);
            for (int j = 0; j < 16 && (i + j) < min(64, bytesRead); j++) {
                printf("%02X ", buffer[i + j]);
            }
            printf(" | ");
            for (int j = 0; j < 16 && (i + j) < min(64, bytesRead); j++) {
                char c = buffer[i + j];
                printf("%c", (c >= 32 && c <= 126) ? c : '.');
            }
            printf("\n");
        }
        
        // Zkusit číst jako text
        buffer[min(sizeof(buffer) - 1, bytesRead)] = 0;
        printf("\nPokus o čtení jako text: '%.50s'\n", buffer);
        
    } else {
        printf("Chyba čtení: %d\n", GetLastError());
    }
    
    CloseHandle(hProcess);
    
    printf("\nStiskněte Enter...\n");
    getchar();
    return 0;
}
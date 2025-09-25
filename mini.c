#include <windows.h>
#include <stdio.h>

int main() {
    printf("MINIMAL READER\n");
    
    // Najdi TwinCAT proces  
    DWORD pid = 0;
    HWND hwnd = NULL;
    
    // Projdi všechna okna
    while ((hwnd = FindWindowEx(NULL, hwnd, NULL, NULL)) != NULL) {
        char title[256];
        GetWindowText(hwnd, title, sizeof(title));
        if (strstr(title, "TwinCAT") && strstr(title, "ST03")) {
            GetWindowThreadProcessId(hwnd, &pid);
            printf("PID: %d\n", pid);
            break;
        }
    }
    
    if (!pid) {
        printf("TwinCAT not found\n");
        return 1;
    }
    
    HANDLE hProc = OpenProcess(PROCESS_VM_READ, FALSE, pid);
    if (!hProc) {
        printf("Cannot open process\n");
        return 1;
    }
    
    // Test čtení z konkrétních adres
    DWORD addresses[] = {0x094CE268, 0x09495760, 0x09497080, 0x094CE008};
    
    for (int i = 0; i < 4; i++) {
        printf("\nADR 0x%08X:\n", addresses[i]);
        
        char buf[128];
        SIZE_T read = 0;
        
        if (ReadProcessMemory(hProc, (void*)addresses[i], buf, 127, &read)) {
            buf[127] = 0;
            
            // Hex dump
            for (int j = 0; j < 32; j++) {
                printf("%02X ", (unsigned char)buf[j]);
                if ((j + 1) % 16 == 0) printf("\n");
            }
            
            // Pokus o text
            printf("TEXT: ");
            for (int j = 0; j < 32; j++) {
                printf("%c", (buf[j] >= 32 && buf[j] <= 126) ? buf[j] : '.');
            }
            printf("\n");
            
        } else {
            printf("ERROR: %d\n", GetLastError());
        }
    }
    
    CloseHandle(hProc);
    printf("\nDONE\n");
    return 0;
}
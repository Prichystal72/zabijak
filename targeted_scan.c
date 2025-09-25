#include <windows.h>
#include <stdio.h>

void analyzeAddress(HANDLE hProcess, DWORD address, const char *description) {
    printf("\n=== %s: 0x%08X ===\n", description, address);
    
    unsigned char buffer[256];
    SIZE_T bytesRead = 0;
    
    if (ReadProcessMemory(hProcess, (LPCVOID)address, buffer, sizeof(buffer), &bytesRead)) {
        printf("Precteno %zu bytu:\n", bytesRead);
        
        // Hex dump prvnich 64 bytu
        for (int i = 0; i < (int)min(64, bytesRead); i += 16) {
            printf("%04X: ", i);
            for (int j = 0; j < 16 && (i + j) < (int)min(64, bytesRead); j++) {
                printf("%02X ", buffer[i + j]);
            }
            printf(" | ");
            for (int j = 0; j < 16 && (i + j) < (int)min(64, bytesRead); j++) {
                char c = buffer[i + j];
                printf("%c", (c >= 32 && c <= 126) ? c : '.');
            }
            printf("\n");
        }
        
        // Zkusit jako primy text
        buffer[(int)min(sizeof(buffer) - 1, bytesRead)] = 0;
        if (strlen((char*)buffer) > 4) {
            printf("Jako text: \"%.60s\"\n", buffer);
        }
        
        // Hledat pointery uvnitr
        printf("Pointery uvnitr struktury:\n");
        for (int offset = 0; offset < (int)min(64, bytesRead) - 4; offset += 4) {
            DWORD ptr = *(DWORD*)(buffer + offset);
            if (ptr > 0x09000000 && ptr < 0x0A000000) {
                printf("  +%02d: 0x%08X\n", offset, ptr);
                
                // Zkusit cist z tohoto pointeru
                char textBuf[128];
                SIZE_T textRead = 0;
                if (ReadProcessMemory(hProcess, (LPCVOID)ptr, textBuf, sizeof(textBuf) - 1, &textRead)) {
                    textBuf[(int)min(sizeof(textBuf) - 1, textRead)] = 0;
                    
                    int validChars = 0;
                    for (int k = 0; k < 40 && textBuf[k] != 0; k++) {
                        if (textBuf[k] >= 32 && textBuf[k] <= 126) {
                            validChars++;
                        } else break;
                    }
                    
                    if (validChars > 4) {
                        printf("       -> \"%.40s\"\n", textBuf);
                        if (strstr(textBuf, "ST03") || strstr(textBuf, "Main") || strstr(textBuf, "PRG")) {
                            printf("       *** MOZNY CILOVY TEXT! ***\n");
                        }
                    }
                }
            }
        }
    } else {
        printf("CHYBA cteni: %d\n", GetLastError());
    }
}

int main() {
    printf("=== CILENA ANALYZA ZNAMYCH ADRES ===\n");
    
    // Ziskej TwinCAT process
    HWND hwnd = NULL;
    DWORD processId = 0;
    
    while ((hwnd = FindWindowEx(NULL, hwnd, NULL, NULL)) != NULL) {
        char title[512];
        GetWindowText(hwnd, title, sizeof(title));
        if (strstr(title, "TwinCAT PLC Control") && strstr(title, "ST03_Main")) {
            GetWindowThreadProcessId(hwnd, &processId);
            printf("Nalezen TwinCAT process (PID: %d)\n", processId);
            break;
        }
    }
    
    if (!processId) {
        printf("TwinCAT process nenalezen!\n");
        getchar();
        return 1;
    }
    
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) {
        printf("Nelze otevrit proces!\n");
        getchar();
        return 1;
    }
    
    // Analyzuj zname adresy z memory_reader vystupu
    analyzeAddress(hProcess, 0x094CE268, "PRVNI OBJEKT");
    analyzeAddress(hProcess, 0x09495760, "POINTER Z OFFSET +16");
    analyzeAddress(hProcess, 0x09497080, "POINTER Z OFFSET +48");
    
    // Analyzuj take druhy objekt pro srovnani
    analyzeAddress(hProcess, 0x094CE008, "DRUHY OBJEKT");
    
    CloseHandle(hProcess);
    
    printf("\n\nAnalyza dokoncena. Stisknete Enter...\n");
    getchar();
    return 0;
}
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Jednoduchy memory disassembler pro TwinCAT owner-drawn ListBox
int main() {
    printf("=== TwinCAT LISTBOX MEMORY DISASSEMBLER ===\n\n");
    
    // Najdi TwinCAT okno - hledej okno s "TwinCAT" v názvu
    HWND hTwinCAT = NULL;
    HWND hWnd = GetTopWindow(GetDesktopWindow());
    
    while (hWnd) {
        char windowTitle[512];
        if (GetWindowText(hWnd, windowTitle, sizeof(windowTitle))) {
            if (strstr(windowTitle, "TwinCAT") != NULL) {
                hTwinCAT = hWnd;
                printf("✓ TwinCAT okno nalezeno: '%s'\n", windowTitle);
                break;
            }
        }
        hWnd = GetNextWindow(hWnd, GW_HWNDNEXT);
    }
    
    if (!hTwinCAT) {
        printf("❌ TwinCAT okno nenalezeno!\n");
        return 1;
    }
    
    printf("✓ TwinCAT okno nalezeno: 0x%p\n", hTwinCAT);
    
    // Najdi náš ListBox (handle 0x00070400)
    HWND hListBox = (HWND)0x00070400;
    
    if (!IsWindow(hListBox)) {
        printf("❌ ListBox 0x00070400 není platný!\n");
        return 1;
    }
    
    printf("✓ ListBox nalezen: 0x%p\n", hListBox);
    
    // Získej proces
    DWORD processId;
    GetWindowThreadProcessId(hListBox, &processId);
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId);
    
    if (!hProcess) {
        printf("❌ Nelze otevřít proces %lu (error: %lu)\n", processId, GetLastError());
        return 1;
    }
    
    printf("✓ Proces %lu otevřen pro čtení\n", processId);
    
    // Získej počet položek
    int count = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
    printf("✓ ListBox má %d položek\n\n", count);
    
    // Analyzuj první 10 položek
    for (int item = 0; item < min(10, count); item++) {
        printf("=== POLOŽKA %d ===\n", item);
        
        LRESULT itemData = SendMessage(hListBox, LB_GETITEMDATA, item, 0);
        if (itemData == LB_ERR || itemData == 0) {
            printf("Bez ItemData\n\n");
            continue;
        }
        
        printf("ItemData: 0x%08lX\n", itemData);
        
        // Přečti 200 bytů jako hex dump
        unsigned char buffer[200];
        SIZE_T bytesRead;
        
        if (ReadProcessMemory(hProcess, (void*)itemData, buffer, sizeof(buffer), &bytesRead)) {
            printf("Hex dump (%zu bytes):\n", bytesRead);
            
            // Hex dump
            for (size_t i = 0; i < bytesRead; i += 16) {
                printf("%04zX: ", i);
                
                // Hex část
                for (size_t j = 0; j < 16 && i + j < bytesRead; j++) {
                    printf("%02X ", buffer[i + j]);
                }
                
                // Padding
                for (size_t j = bytesRead - i; j < 16; j++) {
                    printf("   ");
                }
                
                printf(" | ");
                
                // ASCII část
                for (size_t j = 0; j < 16 && i + j < bytesRead; j++) {
                    unsigned char c = buffer[i + j];
                    printf("%c", (c >= 32 && c <= 126) ? c : '.');
                }
                printf("\n");
            }
            
            // Hledej string pointery
            printf("\nString pointery:\n");
            for (size_t offset = 0; offset < min(100, bytesRead); offset += 4) {
                if (offset + 4 <= bytesRead) {
                    DWORD* ptr = (DWORD*)(buffer + offset);
                    DWORD addr = *ptr;
                    
                    // Je to rozumný pointer?
                    if (addr > 0x400000 && addr < 0x80000000) {
                        char testStr[256] = {0};
                        SIZE_T strBytesRead;
                        
                        if (ReadProcessMemory(hProcess, (void*)addr, testStr, sizeof(testStr)-1, &strBytesRead)) {
                            bool isValid = strBytesRead > 0;
                            size_t validLen = 0;
                            
                            for (size_t i = 0; i < strBytesRead && i < 100; i++) {
                                if (testStr[i] == 0) break;
                                if (testStr[i] < 32 || testStr[i] > 126) {
                                    if (i < 3) isValid = false;
                                    break;
                                }
                                validLen++;
                            }
                            
                            if (isValid && validLen > 1 && validLen < 100) {
                                printf("  [+%03zu] 0x%08X -> '%.*s' (len:%zu)\n", 
                                       offset, addr, (int)validLen, testStr, validLen);
                                
                                // Zkus číst víc dat z tohoto pointeru
                                char fullStr[512] = {0};
                                if (ReadProcessMemory(hProcess, (void*)addr, fullStr, sizeof(fullStr)-1, &strBytesRead)) {
                                    printf("       FULL: '");
                                    for (size_t k = 0; k < strBytesRead && k < 200; k++) {
                                        if (fullStr[k] == 0) break;
                                        if (fullStr[k] >= 32 && fullStr[k] <= 126) {
                                            printf("%c", fullStr[k]);
                                        } else {
                                            printf("\\x%02X", (unsigned char)fullStr[k]);
                                        }
                                    }
                                    printf("'\n");
                                }
                            }
                        }
                    }
                }
            }
        } else {
            printf("❌ Nelze číst paměť na 0x%08lX\n", itemData);
        }
        
        printf("\n");
    }
    
    CloseHandle(hProcess);
    
    printf("Stiskněte Enter...\n");
    getchar();
    
    return 0;
}
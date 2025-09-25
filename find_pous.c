#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

int main() {
    printf("=== HLEDÁNÍ STRINGU 'Pous' V TWINCAT LISTBOX ===\n\n");
    
    // Najdi TwinCAT okno
    HWND hTwinCAT = NULL;
    HWND hWnd = GetTopWindow(GetDesktopWindow());
    
    while (hWnd) {
        char windowTitle[512];
        if (GetWindowText(hWnd, windowTitle, sizeof(windowTitle))) {
            if (strstr(windowTitle, "TwinCAT") != NULL) {
                hTwinCAT = hWnd;
                printf("✓ TwinCAT: '%s'\n", windowTitle);
                break;
            }
        }
        hWnd = GetNextWindow(hWnd, GW_HWNDNEXT);
    }
    
    if (!hTwinCAT) {
        printf("❌ TwinCAT nenalezen!\n");
        return 1;
    }
    
    // ListBox handle
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
    
    printf("✓ Proces %lu otevřen\n", processId);
    
    // Analyzuj první položku - hledej "Pous"
    printf("\n=== HLEDÁNÍ 'Pous' V PRVNÍ POLOŽCE ===\n");
    
    LRESULT itemData = SendMessage(hListBox, LB_GETITEMDATA, 0, 0);
    printf("ItemData[0]: 0x%08lX\n", itemData);
    
    if (itemData != LB_ERR && itemData != 0) {
        // Čti 1000 bytů pro důkladnou analýzu
        unsigned char buffer[1000];
        SIZE_T bytesRead;
        
        if (ReadProcessMemory(hProcess, (void*)itemData, buffer, sizeof(buffer), &bytesRead)) {
            printf("Přečteno %zu bytů\n", bytesRead);
            
            // Hledej všechny string pointery a zkontroluj, jestli obsahují "Pous"
            printf("\nHledání string pointerů s 'Pous':\n");
            
            for (size_t offset = 0; offset < bytesRead - 4; offset += 4) {
                DWORD* ptr = (DWORD*)(buffer + offset);
                DWORD addr = *ptr;
                
                // Je to rozumný pointer?
                if (addr > 0x400000 && addr < 0x80000000) {
                    char testStr[256] = {0};
                    SIZE_T strBytesRead;
                    
                    if (ReadProcessMemory(hProcess, (void*)addr, testStr, sizeof(testStr)-1, &strBytesRead)) {
                        // Hledej "Pous" v tomto stringu
                        if (strstr(testStr, "Pous") != NULL) {
                            printf("*** NALEZEN! ***\n");
                            printf("Offset: +%zu (0x%zX)\n", offset, offset);
                            printf("Pointer: 0x%08X\n", addr);
                            printf("String: '");
                            
                            // Vypiš celý string
                            for (size_t i = 0; i < strBytesRead && i < 200; i++) {
                                if (testStr[i] == 0) break;
                                if (testStr[i] >= 32 && testStr[i] <= 126) {
                                    printf("%c", testStr[i]);
                                } else {
                                    printf("\\x%02X", (unsigned char)testStr[i]);
                                }
                            }
                            printf("'\n\n");
                            
                            // Hex dump okolo tohoto offsetu
                            printf("Hex dump okolo offsetu %zu:\n", offset);
                            size_t start = (offset > 32) ? offset - 32 : 0;
                            size_t end = (offset + 48 < bytesRead) ? offset + 48 : bytesRead;
                            
                            for (size_t i = start; i < end; i += 16) {
                                printf("%04zX: ", i);
                                
                                for (size_t j = 0; j < 16 && i + j < end; j++) {
                                    if (i + j == offset) printf("[");
                                    printf("%02X", buffer[i + j]);
                                    if (i + j == offset + 3) printf("]");
                                    printf(" ");
                                }
                                printf("\n");
                            }
                            
                            // Teď testuj tento offset na dalších položkách
                            printf("\n=== TESTOVÁNÍ OFFSETU %zu NA DALŠÍCH POLOŽKÁCH ===\n", offset);
                            
                            int count = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
                            for (int item = 1; item < min(10, count); item++) {
                                LRESULT itemData2 = SendMessage(hListBox, LB_GETITEMDATA, item, 0);
                                if (itemData2 != LB_ERR && itemData2 != 0) {
                                    unsigned char buffer2[200];
                                    SIZE_T bytesRead2;
                                    
                                    if (ReadProcessMemory(hProcess, (void*)itemData2, buffer2, sizeof(buffer2), &bytesRead2)) {
                                        if (offset + 4 <= bytesRead2) {
                                            DWORD* ptr2 = (DWORD*)(buffer2 + offset);
                                            DWORD addr2 = *ptr2;
                                            
                                            if (addr2 > 0x400000 && addr2 < 0x80000000) {
                                                char testStr2[256] = {0};
                                                SIZE_T strBytesRead2;
                                                
                                                if (ReadProcessMemory(hProcess, (void*)addr2, testStr2, sizeof(testStr2)-1, &strBytesRead2)) {
                                                    printf("[%d] 0x%08X -> '", item, addr2);
                                                    
                                                    for (size_t k = 0; k < strBytesRead2 && k < 50; k++) {
                                                        if (testStr2[k] == 0) break;
                                                        if (testStr2[k] >= 32 && testStr2[k] <= 126) {
                                                            printf("%c", testStr2[k]);
                                                        } else {
                                                            printf("\\x%02X", (unsigned char)testStr2[k]);
                                                        }
                                                    }
                                                    printf("'\n");
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            
                            printf("\nStiskněte Enter...\n");
                            getchar();
                            return 0;
                        }
                    }
                }
            }
            
            printf("❌ String 'Pous' nenalezen v žádném pointeru!\n");
            
            // Backup: zobraz hex dump pro manuální analýzu
            printf("\nHex dump první položky pro manuální analýzu:\n");
            for (size_t i = 0; i < min(200, bytesRead); i += 16) {
                printf("%04zX: ", i);
                
                for (size_t j = 0; j < 16 && i + j < min(200, bytesRead); j++) {
                    printf("%02X ", buffer[i + j]);
                }
                
                printf(" | ");
                
                for (size_t j = 0; j < 16 && i + j < min(200, bytesRead); j++) {
                    unsigned char c = buffer[i + j];
                    printf("%c", (c >= 32 && c <= 126) ? c : '.');
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
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

int main() {
    printf("=== DEBUG EXTRAKCE - CO JE SKUTEČNĚ NA ADRESÁCH ===\n\n");
    
    // TwinCAT
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
    
    if (!hTwinCAT) return 1;
    
    // ListBox
    HWND hListBox = (HWND)0x00070400;
    DWORD processId;
    GetWindowThreadProcessId(hListBox, &processId);
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId);
    
    if (!hProcess) return 1;
    
    int count = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
    printf("ListBox má %d položek\n\n", count);
    
    // Analyzuj prvních 10 položek detailně
    for (int item = 0; item < min(10, count); item++) {
        printf("=== POLOŽKA %d ===\n", item);
        
        LRESULT itemData = SendMessage(hListBox, LB_GETITEMDATA, item, 0);
        if (itemData == LB_ERR || itemData == 0) {
            printf("Bez ItemData\n\n");
            continue;
        }
        
        printf("ItemData: 0x%08lX\n", itemData);
        
        // Přečti strukturu
        unsigned char buffer[200];
        SIZE_T bytesRead;
        
        if (ReadProcessMemory(hProcess, (void*)itemData, buffer, sizeof(buffer), &bytesRead)) {
            // Zobraz hex dump struktury
            printf("Hex dump ItemData:\n");
            for (size_t i = 0; i < min(64, bytesRead); i += 16) {
                printf("%04zx: ", i);
                for (size_t j = 0; j < 16 && i + j < min(64, bytesRead); j++) {
                    if (i + j == 20) printf("[");
                    printf("%02X", buffer[i + j]);
                    if (i + j == 23) printf("]");
                    printf(" ");
                }
                printf("\n");
            }
            
            // Zkus offset 20
            if (bytesRead >= 24) {
                DWORD* ptr20 = (DWORD*)(buffer + 20);
                DWORD addr20 = *ptr20;
                printf("Offset 20: 0x%08X\n", addr20);
                
                if (addr20 > 0x400000 && addr20 < 0x80000000) {
                    // Čti co je na této adrese
                    unsigned char textBuffer[256];
                    SIZE_T textRead;
                    
                    if (ReadProcessMemory(hProcess, (void*)addr20, textBuffer, sizeof(textBuffer), &textRead)) {
                        printf("Raw data na 0x%08X (%zu bytů):\n", addr20, textRead);
                        
                        // Hex dump
                        for (size_t i = 0; i < min(64, textRead); i += 16) {
                            printf("  %04zx: ", i);
                            for (size_t j = 0; j < 16 && i + j < min(64, textRead); j++) {
                                printf("%02X ", textBuffer[i + j]);
                            }
                            printf(" | ");
                            for (size_t j = 0; j < 16 && i + j < min(64, textRead); j++) {
                                unsigned char c = textBuffer[i + j];
                                printf("%c", (c >= 32 && c <= 126) ? c : '.');
                            }
                            printf("\n");
                        }
                        
                        // Zkus interpretovat jako string
                        printf("Jako ANSI string: '");
                        for (size_t i = 0; i < textRead && i < 100; i++) {
                            if (textBuffer[i] == 0) break;
                            if (textBuffer[i] >= 32 && textBuffer[i] <= 126) {
                                printf("%c", textBuffer[i]);
                            }
                        }
                        printf("'\n");
                        
                        // Zkus interpretovat jako wide string
                        wchar_t* wideBuffer = (wchar_t*)textBuffer;
                        printf("Jako Unicode string: '");
                        for (size_t i = 0; i < textRead/2 && i < 50; i++) {
                            if (wideBuffer[i] == 0) break;
                            if (wideBuffer[i] >= 32 && wideBuffer[i] <= 126) {
                                printf("%c", (char)wideBuffer[i]);
                            }
                        }
                        printf("'\n");
                    }
                }
            }
            
            // Zkus také další offsety
            printf("Testování dalších offsetů:\n");
            for (int offset = 0; offset < min(100, (int)bytesRead); offset += 4) {
                if (offset + 4 <= bytesRead) {
                    DWORD* ptr = (DWORD*)(buffer + offset);
                    DWORD addr = *ptr;
                    
                    if (addr > 0x400000 && addr < 0x80000000) {
                        char testStr[100] = {0};
                        SIZE_T testRead;
                        
                        if (ReadProcessMemory(hProcess, (void*)addr, testStr, sizeof(testStr)-1, &testRead)) {
                            // Zkontroluj jestli to vypadá jako string
                            bool hasValidChars = false;
                            size_t validLen = 0;
                            
                            for (size_t i = 0; i < testRead && i < 50; i++) {
                                if (testStr[i] == 0) break;
                                if (testStr[i] >= 32 && testStr[i] <= 126) {
                                    hasValidChars = true;
                                    validLen++;
                                } else if (hasValidChars) {
                                    break;
                                }
                            }
                            
                            if (hasValidChars && validLen > 2) {
                                printf("  [+%02d] 0x%08X -> '%.*s'\n", 
                                       offset, addr, (int)validLen, testStr);
                            }
                        }
                    }
                }
            }
        }
        
        printf("\n");
    }
    
    CloseHandle(hProcess);
    
    printf("Stiskněte Enter...\n");
    getchar();
    return 0;
}
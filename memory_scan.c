#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

int main() {
    printf("=== HLEDÁNÍ 'PLC_PRG' V CELÉ PAMĚTI TWINCAT PROCESU ===\n\n");
    
    // Najdi TwinCAT
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
    
    // ListBox
    HWND hListBox = (HWND)0x00070400;
    DWORD processId;
    GetWindowThreadProcessId(hListBox, &processId);
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processId);
    
    if (!hProcess) {
        printf("❌ Nelze otevřít proces %lu\n", processId);
        return 1;
    }
    
    printf("✓ Proces %lu otevřen pro skenování paměti\n", processId);
    
    // Získej informace o paměti procesu
    MEMORY_BASIC_INFORMATION mbi;
    SIZE_T address = 0;
    int foundCount = 0;
    
    printf("\nSkenování paměti pro 'PLC_PRG' a 'ST00__Init'...\n");
    
    while (VirtualQueryEx(hProcess, (LPCVOID)address, &mbi, sizeof(mbi)) == sizeof(mbi)) {
        // Skenuj pouze committed memory pages
        if (mbi.State == MEM_COMMIT && 
            (mbi.Type == MEM_PRIVATE || mbi.Type == MEM_MAPPED) &&
            (mbi.Protect == PAGE_READWRITE || mbi.Protect == PAGE_READONLY)) {
            
            // Alokuj buffer pro tento memory region
            SIZE_T regionSize = mbi.RegionSize;
            if (regionSize > 1024 * 1024) regionSize = 1024 * 1024; // Omez na 1MB
            
            char* buffer = malloc(regionSize);
            if (buffer) {
                SIZE_T bytesRead;
                if (ReadProcessMemory(hProcess, mbi.BaseAddress, buffer, regionSize, &bytesRead)) {
                    // Hledej "PLC_PRG" v tomto regionu
                    for (SIZE_T i = 0; i < bytesRead - 7; i++) {
                        if (memcmp(buffer + i, "PLC_PRG", 7) == 0) {
                            SIZE_T absoluteAddr = (SIZE_T)mbi.BaseAddress + i;
                            printf("\n*** NALEZEN 'PLC_PRG' ***\n");
                            printf("Adresa: 0x%08zX\n", absoluteAddr);
                            printf("Region: Base=0x%p, Size=%zu, Protect=0x%lX\n", 
                                   mbi.BaseAddress, mbi.RegionSize, mbi.Protect);
                            
                            // Zobraz kontext okolo
                            printf("Kontext: '");
                            SIZE_T start = (i > 50) ? i - 50 : 0;
                            SIZE_T end = (i + 50 < bytesRead) ? i + 50 : bytesRead;
                            
                            for (SIZE_T j = start; j < end; j++) {
                                char c = buffer[j];
                                if (j == i) printf("[");
                                if (c >= 32 && c <= 126) {
                                    printf("%c", c);
                                } else if (c == 0) {
                                    printf("\\0");
                                } else {
                                    printf("\\x%02X", (unsigned char)c);
                                }
                                if (j == i + 6) printf("]");
                            }
                            printf("'\n");
                            
                            // Hex dump okolo
                            printf("Hex dump:\n");
                            SIZE_T hexStart = (i > 32) ? i - 32 : 0;
                            SIZE_T hexEnd = (i + 32 < bytesRead) ? i + 32 : bytesRead;
                            
                            for (SIZE_T j = hexStart; j < hexEnd; j += 16) {
                                printf("%08zX: ", absoluteAddr - i + j);
                                
                                for (SIZE_T k = 0; k < 16 && j + k < hexEnd; k++) {
                                    if (j + k >= i && j + k < i + 7) printf("[");
                                    printf("%02X", (unsigned char)buffer[j + k]);
                                    if (j + k >= i && j + k < i + 7) printf("]");
                                    printf(" ");
                                }
                                printf("\n");
                            }
                            
                            foundCount++;
                            
                            // Pokud je to první nalezení, pokračuj v analýze
                            if (foundCount == 1) {
                                printf("\n=== ANALÝZA PRVNÍ NALEZENÉ INSTANCE ===\n");
                                
                                // Teď zkus najít jak se k této adrese dostat z ItemData
                                printf("Hledání reference na 0x%08zX v ItemData strukturách...\n", absoluteAddr);
                                
                                int count = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
                                for (int item = 0; item < min(count, 20); item++) {
                                    LRESULT itemData = SendMessage(hListBox, LB_GETITEMDATA, item, 0);
                                    if (itemData != LB_ERR && itemData != 0) {
                                        char itemBuffer[1000];
                                        SIZE_T itemBytesRead;
                                        
                                        if (ReadProcessMemory(hProcess, (void*)itemData, itemBuffer, sizeof(itemBuffer), &itemBytesRead)) {
                                            // Hledej naši adresu v ItemData
                                            for (SIZE_T offset = 0; offset < itemBytesRead - 4; offset += 4) {
                                                DWORD* ptr = (DWORD*)(itemBuffer + offset);
                                                if (*ptr == (DWORD)absoluteAddr) {
                                                    printf("*** REFERENCE NALEZENA! ***\n");
                                                    printf("Item[%d], offset %zu: 0x%08X -> 'PLC_PRG'\n", 
                                                           item, offset, *ptr);
                                                }
                                                
                                                // Zkus také indirect reference
                                                if (*ptr > 0x400000 && *ptr < 0x80000000) {
                                                    DWORD secondPtr;
                                                    SIZE_T secondRead;
                                                    if (ReadProcessMemory(hProcess, (void*)*ptr, &secondPtr, 4, &secondRead)) {
                                                        if (secondPtr == (DWORD)absoluteAddr) {
                                                            printf("*** NEPŘÍMÁ REFERENCE! ***\n");
                                                            printf("Item[%d], offset %zu -> 0x%08X -> 0x%08X -> 'PLC_PRG'\n", 
                                                                   item, offset, *ptr, secondPtr);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            
                            if (foundCount >= 3) break; // Omez na první 3 nálezy
                        }
                    }
                    
                    // Hledej také "ST00__Init"
                    for (SIZE_T i = 0; i < bytesRead - 10; i++) {
                        if (memcmp(buffer + i, "ST00__Init", 10) == 0) {
                            SIZE_T absoluteAddr = (SIZE_T)mbi.BaseAddress + i;
                            printf("\n*** NALEZEN 'ST00__Init' ***\n");
                            printf("Adresa: 0x%08zX\n", absoluteAddr);
                            printf("Region: Base=0x%p, Size=%zu, Protect=0x%lX\n", 
                                   mbi.BaseAddress, mbi.RegionSize, mbi.Protect);
                            
                            // Zobraz kontext okolo
                            printf("Kontext: '");
                            SIZE_T start = (i > 50) ? i - 50 : 0;
                            SIZE_T end = (i + 50 < bytesRead) ? i + 50 : bytesRead;
                            
                            for (SIZE_T j = start; j < end; j++) {
                                char c = buffer[j];
                                if (j == i) printf("[");
                                if (c >= 32 && c <= 126) {
                                    printf("%c", c);
                                } else if (c == 0) {
                                    printf("\\0");
                                } else {
                                    printf("\\x%02X", (unsigned char)c);
                                }
                                if (j == i + 9) printf("]");
                            }
                            printf("'\n");
                            
                            foundCount++;
                            
                            // Hledej reference na tuto adresu v ItemData
                            printf("Hledání reference na ST00__Init v ItemData...\n");
                            
                            int count = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
                            for (int item = 0; item < min(count, 20); item++) {
                                LRESULT itemData = SendMessage(hListBox, LB_GETITEMDATA, item, 0);
                                if (itemData != LB_ERR && itemData != 0) {
                                    char itemBuffer[1000];
                                    SIZE_T itemBytesRead;
                                    
                                    if (ReadProcessMemory(hProcess, (void*)itemData, itemBuffer, sizeof(itemBuffer), &itemBytesRead)) {
                                        // Hledej naši adresu v ItemData
                                        for (SIZE_T offset = 0; offset < itemBytesRead - 4; offset += 4) {
                                            DWORD* ptr = (DWORD*)(itemBuffer + offset);
                                            if (*ptr == (DWORD)absoluteAddr) {
                                                printf("*** ST00__Init REFERENCE! ***\n");
                                                printf("Item[%d], offset %zu: 0x%08X -> 'ST00__Init'\n", 
                                                       item, offset, *ptr);
                                            }
                                        }
                                    }
                                }
                            }
                            
                            if (foundCount >= 6) break;
                        }
                    }
                }
                free(buffer);
            }
        }
        
        address = (SIZE_T)mbi.BaseAddress + mbi.RegionSize;
        
        // Omez skenování - proces může být velký
        if (address > 0x80000000) break;
        if (foundCount >= 3) break;
    }
    
    printf("\nCelkem nalezeno %d instancí textů\n", foundCount);
    
    CloseHandle(hProcess);
    
    printf("\nStiskněte Enter...\n");
    getchar();
    return 0;
}
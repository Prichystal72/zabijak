#include <windows.h>
#include <stdio.h>

int main() {
    printf("=== ANALÝZA SPOLEČNÉ ADRESY 0x01621ED0 ===\n");
    
    // Získej TwinCAT process
    HWND hwnd = FindWindow(NULL, NULL);
    DWORD processId = 0;
    
    // Najdi TwinCAT okno
    while ((hwnd = FindWindowEx(NULL, hwnd, NULL, NULL)) != NULL) {
        char title[512];
        GetWindowText(hwnd, title, sizeof(title));
        if (strstr(title, "TwinCAT PLC Control") && strstr(title, "ST03_Main")) {
            GetWindowThreadProcessId(hwnd, &processId);
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
        printf("Nelze otevřít proces! Chyba: %d\n", GetLastError());
        getchar();
        return 1;
    }
    
    printf("Process otevřen (PID: %d)\n", processId);
    
    // Analyzuj společnou adresu
    DWORD commonAddress = 0x01621ED0;
    printf("\nAnalyzuji společnou adresu: 0x%08X\n", commonAddress);
    
    unsigned char buffer[512];
    SIZE_T bytesRead = 0;
    
    if (ReadProcessMemory(hProcess, (LPCVOID)commonAddress, buffer, sizeof(buffer), &bytesRead)) {
        printf("Přečteno %zu bytů ze společné adresy:\n", bytesRead);
        
        // Hex dump
        for (int i = 0; i < min(128, bytesRead); i += 16) {
            printf("%04X: ", i);
            for (int j = 0; j < 16 && (i + j) < min(128, bytesRead); j++) {
                printf("%02X ", buffer[i + j]);
            }
            printf(" | ");
            for (int j = 0; j < 16 && (i + j) < min(128, bytesRead); j++) {
                char c = buffer[i + j];
                printf("%c", (c >= 32 && c <= 126) ? c : '.');
            }
            printf("\n");
        }
        
        printf("\nHledám pointery v této struktuře:\n");
        for (int offset = 0; offset < min(64, bytesRead) - 4; offset += 4) {
            DWORD possiblePtr = *(DWORD*)(buffer + offset);
            if (possiblePtr > 0x400000 && possiblePtr < 0x7FFFFFFF) {
                printf("Offset +%02d: 0x%08X\n", offset, possiblePtr);
            }
        }
        
    } else {
        printf("Chyba čtení společné adresy: %d\n", GetLastError());
    }
    
    // Nyní analyzuj první objekt (0x094CE268)
    DWORD objectAddress = 0x094CE268;
    printf("\n\nAnalyzuji první objekt: 0x%08X\n", objectAddress);
    
    if (ReadProcessMemory(hProcess, (LPCVOID)objectAddress, buffer, sizeof(buffer), &bytesRead)) {
        printf("Přečteno %zu bytů z objektu:\n", bytesRead);
        
        // Hex dump objektu
        for (int i = 0; i < min(128, bytesRead); i += 16) {
            printf("%04X: ", i);
            for (int j = 0; j < 16 && (i + j) < min(128, bytesRead); j++) {
                printf("%02X ", buffer[i + j]);
            }
            printf(" | ");
            for (int j = 0; j < 16 && (i + j) < min(128, bytesRead); j++) {
                char c = buffer[i + j];
                printf("%c", (c >= 32 && c <= 126) ? c : '.');
            }
            printf("\n");
        }
        
        printf("\nHledám textové pointery v objektu:\n");
        for (int offset = 0; offset < min(128, bytesRead) - 4; offset += 4) {
            DWORD possiblePtr = *(DWORD*)(buffer + offset);
            if (possiblePtr > 0x09000000 && possiblePtr < 0x0A000000) { // Upravený rozsah pro naše data
                char textBuffer[256];
                SIZE_T textBytesRead = 0;
                
                printf("Testuji pointer na offsetu +%03d: 0x%08X\n", offset, possiblePtr);
                
                if (ReadProcessMemory(hProcess, (LPCVOID)possiblePtr, textBuffer, sizeof(textBuffer) - 1, &textBytesRead)) {
                    textBuffer[min(sizeof(textBuffer) - 1, textBytesRead)] = 0;
                    
                    // Zobraz prvních 64 bytů jako hex
                    printf("   Raw data z 0x%08X:\n", possiblePtr);
                    for (int i = 0; i < min(64, textBytesRead); i += 16) {
                        printf("   %04X: ", i);
                        for (int j = 0; j < 16 && (i + j) < min(64, textBytesRead); j++) {
                            printf("%02X ", (unsigned char)textBuffer[i + j]);
                        }
                        printf(" | ");
                        for (int j = 0; j < 16 && (i + j) < min(64, textBytesRead); j++) {
                            char c = textBuffer[i + j];
                            printf("%c", (c >= 32 && c <= 126) ? c : '.');
                        }
                        printf("\n");
                    }
                    
                    // Zkontrolovat validitu textu
                    int validChars = 0;
                    for (int k = 0; k < min(50, textBytesRead) && textBuffer[k] != 0; k++) {
                        if (textBuffer[k] >= 32 && textBuffer[k] <= 126) {
                            validChars++;
                        } else if (textBuffer[k] == 0) {
                            break;
                        } else {
                            break;
                        }
                    }
                    
                    if (validChars > 4) {
                        printf("   TEXT na offsetu +%03d -> 0x%08X: \"%s\"\n", offset, possiblePtr, textBuffer);
                        
                        if (strstr(textBuffer, "ST03_Main") || strstr(textBuffer, "PRG") || strstr(textBuffer, "SFC")) {
                            printf("   *** NALEZEN HLEDANÝ TEXT! ***\n");
                        }
                    }
                    
                    // Zkusit také číst jako pointery na text
                    for (int subOffset = 0; subOffset < min(32, textBytesRead) - 4; subOffset += 4) {
                        DWORD subPtr = *(DWORD*)(textBuffer + subOffset);
                        if (subPtr > 0x09000000 && subPtr < 0x0A000000) {
                            char subText[128];
                            SIZE_T subBytesRead = 0;
                            
                            if (ReadProcessMemory(hProcess, (LPCVOID)subPtr, subText, sizeof(subText) - 1, &subBytesRead)) {
                                subText[min(sizeof(subText) - 1, subBytesRead)] = 0;
                                
                                int subValidChars = 0;
                                for (int m = 0; m < min(30, subBytesRead) && subText[m] != 0; m++) {
                                    if (subText[m] >= 32 && subText[m] <= 126) {
                                        subValidChars++;
                                    } else if (subText[m] == 0) {
                                        break;
                                    } else {
                                        break;
                                    }
                                }
                                
                                if (subValidChars > 4) {
                                    printf("   SUB +%02d -> 0x%08X: \"%s\"\n", subOffset, subPtr, subText);
                                    
                                    if (strstr(subText, "ST03_Main") || strstr(subText, "PRG") || strstr(subText, "SFC")) {
                                        printf("   *** NALEZEN HLEDANÝ TEXT V SUB-POINTERU! ***\n");
                                    }
                                }
                            }
                        }
                    }
                    
                } else {
                    printf("   Chyba čtení z 0x%08X: %d\n", possiblePtr, GetLastError());
                }
                printf("\n");
            }
        }
        
    } else {
        printf("Chyba čtení objektu: %d\n", GetLastError());
    }
    
    CloseHandle(hProcess);
    
    printf("\nStiskněte Enter...\n");
    getchar();
    return 0;
}
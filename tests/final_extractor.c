#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

int main() {
    printf("=== FINÁLNÍ EXTRAKTOR TEXTŮ Z TWINCAT LISTBOX ===\n");
    printf("=== STRUKTURA STROMU VČETNĚ VŠECH POLOŽEK ===\n\n");
    
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
    if (!IsWindow(hListBox)) {
        printf("❌ ListBox neplatný!\n");
        return 1;
    }
    
    DWORD processId;
    GetWindowThreadProcessId(hListBox, &processId);
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId);
    
    if (!hProcess) {
        printf("❌ Nelze otevřít proces %lu\n", processId);
        return 1;
    }
    
    printf("✓ Proces %lu otevřen\n", processId);
    
    int count = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
    printf("✓ ListBox obsahuje %d položek\n\n", count);
    
    printf("=== EXTRAKCE VŠECH TEXTŮ (OFFSET 20) ===\n\n");
    
    // Extrahuj všechny texty pomocí offset 20
    for (int item = 0; item < count; item++) {
        LRESULT itemData = SendMessage(hListBox, LB_GETITEMDATA, item, 0);
        
        if (itemData == LB_ERR || itemData == 0) {
            printf("[%3d] <bez dat>\n", item);
            continue;
        }
        
        // Přečti strukturu
        unsigned char buffer[200];
        SIZE_T bytesRead;
        
        if (ReadProcessMemory(hProcess, (void*)itemData, buffer, sizeof(buffer), &bytesRead)) {
            // Zkus offset 20 (0x14)
            if (bytesRead >= 24) { // 20 + 4 bytes
                DWORD* ptr = (DWORD*)(buffer + 20);
                DWORD textAddr = *ptr;
                
                if (textAddr > 0x400000 && textAddr < 0x80000000) {
                    char textStr[512] = {0};
                    SIZE_T textRead;
                    
                    if (ReadProcessMemory(hProcess, (void*)textAddr, textStr, sizeof(textStr)-1, &textRead)) {
                        // Vyčisti string (jen printable chars)
                        char cleanStr[512] = {0};
                        size_t cleanLen = 0;
                        
                        for (size_t i = 0; i < textRead && i < 500 && cleanLen < 500; i++) {
                            if (textStr[i] == 0) break;
                            if (textStr[i] >= 32 && textStr[i] <= 126) {
                                cleanStr[cleanLen++] = textStr[i];
                            }
                        }
                        cleanStr[cleanLen] = '\0';
                        
                        if (cleanLen > 0) {
                            // Zkus určit hierarchii podle odsazení nebo struktury
                            char indent[50] = "";
                            
                            // Heuristika pro odsazení na základě názvu
                            if (strstr(cleanStr, "POUs") || strstr(cleanStr, "Stations") || strstr(cleanStr, "Basic")) {
                                strcpy(indent, "");  // Root level
                            }
                            else if (strstr(cleanStr, "PLC_PRG") || strstr(cleanStr, "ST_") || strstr(cleanStr, "FB_")) {
                                strcpy(indent, "  ");  // Level 1
                            }
                            else if (strstr(cleanStr, "__") || strstr(cleanStr, "_Init") || strstr(cleanStr, "_Body")) {
                                strcpy(indent, "    ");  // Level 2
                            }
                            else {
                                strcpy(indent, "      ");  // Level 3+
                            }
                            
                            printf("[%3d] %s%s\n", item, indent, cleanStr);
                        } else {
                            printf("[%3d] <prázdný text na 0x%08X>\n", item, textAddr);
                        }
                    } else {
                        printf("[%3d] <nelze přečíst text na 0x%08X>\n", item, textAddr);
                    }
                } else {
                    printf("[%3d] <neplatný pointer 0x%08X>\n", item, textAddr);
                }
            } else {
                printf("[%3d] <příliš malá struktura: %zu bytů>\n", item, bytesRead);
            }
        } else {
            printf("[%3d] <nelze přečíst ItemData 0x%08lX>\n", item, itemData);
        }
    }
    
    printf("\n=== SOUHRN ===\n");
    printf("Celkem zpracováno: %d položek\n", count);
    printf("Použitý offset: 20 (0x14)\n");
    
    // Bonus: pokus o fokus na 25. položku
    if (count > 24) {
        printf("\n=== POKUS O FOCUS NA 25. POLOŽKU ===\n");
        
        // Nejdřív zjisti jaký text má 25. položka
        LRESULT itemData25 = SendMessage(hListBox, LB_GETITEMDATA, 24, 0);
        if (itemData25 != LB_ERR && itemData25 != 0) {
            unsigned char buffer25[200];
            SIZE_T bytesRead25;
            
            if (ReadProcessMemory(hProcess, (void*)itemData25, buffer25, sizeof(buffer25), &bytesRead25)) {
                if (bytesRead25 >= 24) {
                    DWORD* ptr25 = (DWORD*)(buffer25 + 20);
                    DWORD textAddr25 = *ptr25;
                    
                    if (textAddr25 > 0x400000 && textAddr25 < 0x80000000) {
                        char textStr25[512] = {0};
                        SIZE_T textRead25;
                        
                        if (ReadProcessMemory(hProcess, (void*)textAddr25, textStr25, sizeof(textStr25)-1, &textRead25)) {
                            char cleanStr25[512] = {0};
                            size_t cleanLen25 = 0;
                            
                            for (size_t i = 0; i < textRead25 && i < 500 && cleanLen25 < 500; i++) {
                                if (textStr25[i] == 0) break;
                                if (textStr25[i] >= 32 && textStr25[i] <= 126) {
                                    cleanStr25[cleanLen25++] = textStr25[i];
                                }
                            }
                            cleanStr25[cleanLen25] = '\0';
                            
                            if (cleanLen25 > 0) {
                                printf("25. položka obsahuje: '%s'\n", cleanStr25);
                                
                                // Pokus o nastavení focus
                                SetFocus(hListBox);
                                SendMessage(hListBox, LB_SETCURSEL, 24, 0);
                                SendMessage(hListBox, LB_SETTOPINDEX, 20, 0);
                                
                                printf("✓ Focus nastaven na položku 24 (25. položka)\n");
                            }
                        }
                    }
                }
            }
        }
    }
    
    CloseHandle(hProcess);
    
    printf("\nStiskněte Enter pro ukončení...\n");
    getchar();
    return 0;
}
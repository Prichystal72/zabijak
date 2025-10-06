#include "../lib/twincat_navigator.h"

int main() {
    printf("=== DEBUG POLOZKY [9] - Serielle Kommunikation ===\n\n");
    
    HWND twincatWindow = FindTwinCatWindow();
    if (!twincatWindow) {
        printf("[CHYBA] TwinCAT okno nenalezeno!\n");
        return 1;
    }
    
    HWND listbox = FindProjectListBox(twincatWindow);
    if (!listbox) {
        printf("[CHYBA] ListBox nenalezen!\n");
        return 1;
    }
    
    HANDLE hProcess = OpenTwinCatProcess(listbox);
    if (!hProcess) {
        printf("[CHYBA] Nelze otevrit proces!\n");
        return 1;
    }
    
    // Analyzuj polozku [9]
    int index = 9;
    
    LRESULT itemData = SendMessage(listbox, LB_GETITEMDATA, index, 0);
    printf("ItemData polozky [%d]: 0x%08X\n\n", index, (DWORD)itemData);
    
    if (itemData == LB_ERR || itemData == 0) {
        printf("Chyba - neplatny ItemData!\n");
        CloseHandle(hProcess);
        return 1;
    }
    
    // Nacti strukturu
    DWORD structure[16];
    SIZE_T bytesRead;
    
    if (!ReadProcessMemory(hProcess, (void*)itemData, structure, sizeof(structure), &bytesRead)) {
        printf("Chyba cteni struktury!\n");
        CloseHandle(hProcess);
        return 1;
    }
    
    printf("Precteno %zu bytu ze struktury\n\n", bytesRead);
    
    printf("STRUKTURA ITEMDATA:\n");
    for (int i = 0; i < 10; i++) {
        printf("  structure[%d] = 0x%08X (%u)\n", i, structure[i], structure[i]);
    }
    printf("\n");
    
    // Text pointer
    DWORD textPtr = structure[5];
    printf("Text pointer (structure[5]): 0x%08X\n", textPtr);
    
    if (textPtr < 0x400000 || textPtr > 0x80000000) {
        printf("Text pointer je mimo platny rozsah!\n");
        CloseHandle(hProcess);
        return 1;
    }
    
    // Zkus nacist text s ruznym offsetem
    printf("\nPOKUSY NACIST TEXT (ruzne offsety):\n");
    
    char textBuffer[256];
    SIZE_T textRead;
    
    for (int offset = 0; offset < 8; offset++) {
        memset(textBuffer, 0, sizeof(textBuffer));
        
        if (ReadProcessMemory(hProcess, (void*)(textPtr + offset), textBuffer, 100, &textRead)) {
            printf("\nOffset %d (adresa 0x%08X):\n", offset, textPtr + offset);
            printf("  Precteno: %zu bytu\n", textRead);
            
            // Hex dump prvnich 32 bytu
            printf("  Hex dump: ");
            for (int i = 0; i < 32 && i < textRead; i++) {
                printf("%02X ", (unsigned char)textBuffer[i]);
                if ((i + 1) % 16 == 0) printf("\n            ");
            }
            printf("\n");
            
            // Pokus o extrakci printable textu
            char extracted[256] = {0};
            int len = 0;
            for (int i = 0; i < textRead && len < 200; i++) {
                if (textBuffer[i] >= 32 && textBuffer[i] <= 126) {
                    extracted[len++] = textBuffer[i];
                } else if (len > 0) {
                    break;  // Konec textu
                }
            }
            extracted[len] = '\0';
            
            if (len > 0) {
                printf("  Text: '%s' (delka %d)\n", extracted, len);
            } else {
                printf("  Text: (zadny printable text)\n");
            }
        } else {
            printf("Offset %d: Chyba cteni!\n", offset);
        }
    }
    
    // Zkus i negativni offset
    printf("\n\nPOKUS S NEGATIVNIM OFFSETEM:\n");
    for (int offset = -4; offset < 0; offset++) {
        memset(textBuffer, 0, sizeof(textBuffer));
        
        if (ReadProcessMemory(hProcess, (void*)(textPtr + offset), textBuffer, 100, &textRead)) {
            char extracted[256] = {0};
            int len = 0;
            for (int i = 0; i < textRead && len < 200; i++) {
                if (textBuffer[i] >= 32 && textBuffer[i] <= 126) {
                    extracted[len++] = textBuffer[i];
                } else if (len > 0) {
                    break;
                }
            }
            extracted[len] = '\0';
            
            if (len > 3) {
                printf("Offset %d: '%s'\n", offset, extracted);
            }
        }
    }
    
    // Porovnej s jinou polozkou, ktera funguje
    printf("\n\n=== POROVNANI S FUNGUJICI POLOZKOU [8] ===\n");
    
    LRESULT itemData8 = SendMessage(listbox, LB_GETITEMDATA, 8, 0);
    DWORD structure8[16];
    
    if (ReadProcessMemory(hProcess, (void*)itemData8, structure8, sizeof(structure8), &bytesRead)) {
        printf("\nPolozka [8] - Programmbausteine:\n");
        printf("  ItemData: 0x%08X\n", (DWORD)itemData8);
        printf("  structure[5] (textPtr): 0x%08X\n", structure8[5]);
        
        memset(textBuffer, 0, sizeof(textBuffer));
        if (ReadProcessMemory(hProcess, (void*)structure8[5], textBuffer, 100, &textRead)) {
            printf("  Text na offset 0: '%s'\n", textBuffer);
            printf("  Text na offset 1: '%s'\n", textBuffer + 1);
        }
    }
    
    CloseHandle(hProcess);
    
    printf("\n=== DEBUG DOKONCEN ===\n");
    return 0;
}

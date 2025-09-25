#include <windows.h>
#include <stdio.h>

void analyzeBits(DWORD flags, const char* name) {
    printf("\n=== %s: 0x%lX ===\n", name, flags);
    printf("Decimalne: %lu\n", flags);
    printf("Binarni: ");
    for (int i = 31; i >= 0; i--) {
        printf("%d", (flags >> i) & 1);
        if (i % 4 == 0) printf(" ");
    }
    printf("\n");
    
    // Rozlož na byty
    printf("Byty: ");
    for (int i = 3; i >= 0; i--) {
        printf("0x%02X ", (flags >> (i * 8)) & 0xFF);
    }
    printf("\n");
    
    // Zkus rozložit na části
    printf("Horni word: 0x%04X, Dolni word: 0x%04X\n", 
           (flags >> 16) & 0xFFFF, flags & 0xFFFF);
}

int main() {
    printf("=== ANALYZA FLAGS ===\n");
    
    // Známé hodnoty z výpisů
    analyzeBits(0x3604FD, "FLAG_FOLDER (otevrena slozka s detmi)");
    analyzeBits(0x4047D, "FLAG_SPECIAL (zavrena slozka)");
    analyzeBits(0x704ED, "FLAG_FILE (soubor)");
    analyzeBits(0x504DD, "FLAG_ACTION (akce)");
    
    printf("\n=== POROVNANI ===\n");
    
    // Porovnej rozdíly
    DWORD folder = 0x3604FD;
    DWORD special = 0x4047D;
    DWORD file = 0x704ED;
    DWORD action = 0x504DD;
    
    printf("FOLDER vs SPECIAL: XOR = 0x%lX\n", folder ^ special);
    printf("FOLDER vs FILE: XOR = 0x%lX\n", folder ^ file);
    printf("FOLDER vs ACTION: XOR = 0x%lX\n", folder ^ action);
    printf("SPECIAL vs FILE: XOR = 0x%lX\n", special ^ file);
    
    // Zkus najít společné bity
    printf("\nSpolecne bity:\n");
    printf("FOLDER & SPECIAL: 0x%lX\n", folder & special);
    printf("FOLDER & FILE: 0x%lX\n", folder & file);
    printf("SPECIAL & FILE: 0x%lX\n", special & file);
    
    // Možné masky
    printf("\nMozne masky:\n");
    printf("Typ mask (dolni 16 bitu): FOLDER=0x%04X, SPECIAL=0x%04X, FILE=0x%04X, ACTION=0x%04X\n",
           folder & 0xFFFF, special & 0xFFFF, file & 0xFFFF, action & 0xFFFF);
    
    printf("Stav mask (horni 16 bitu): FOLDER=0x%04X, SPECIAL=0x%04X, FILE=0x%04X, ACTION=0x%04X\n",
           (folder >> 16) & 0xFFFF, (special >> 16) & 0xFFFF, (file >> 16) & 0xFFFF, (action >> 16) & 0xFFFF);
    
    return 0;
}
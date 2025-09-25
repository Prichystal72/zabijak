# TwinCAT Project Navigator

Automatická navigace a extrakce textu z TwinCAT PLC Control owner-drawn ListBox komponent.

## 🎯 Účel

Tento projekt řeší problém automatizované navigace v TwinCAT PLC Control, kde standardní Windows API selhává u owner-drawn ListBox komponent. Program dokáže:

- ✅ Dynamicky najít TwinCAT okno a project ListBox
- ✅ Extrahovat text z owner-drawn ListBox pomocí cross-process memory reading
- ✅ Zobrazit hierarchickou strukturu celého projektu
- ✅ Zaměřit se na konkrétní položku (např. 25. položku)
- ✅ Identifikovat různé typy položek (složky, soubory, akce)

## 📁 Struktura projektu

```
zachyceni_titulku_okna/
├── lib/
│   ├── twincat_navigator.h  # Header s definicemi a prototypy
│   └── twincat_navigator.c  # Implementace knihovních funkcí
├── navigator.c              # Hlavní program
├── navigator.exe            # Zkompilovaný program
├── zabijak.c               # Původní program (reference)
├── Makefile                # Build system
├── build32.bat             # 32-bit build skript
├── build64.bat             # 64-bit build skript
├── cleanup.bat             # Skript pro vyčištění testovacích souborů
└── README.md               # Tato dokumentace
```

## 🔧 Kompilace

### Pomocí GCC (MinGW):
```bash
gcc -Wall -std=c99 -o navigator.exe navigator.c lib/twincat_navigator.c -luser32 -lkernel32
```

### Pomocí Makefile:
```bash
make                # Základní build
make debug          # Debug verze s více informacemi  
make clean          # Vyčištění
make run            # Build a spuštění
```

### Pomocí batch skriptů:
```bash
build64.bat         # 64-bit verze
build32.bat         # 32-bit verze
```

## 🚀 Použití

1. **Spusť TwinCAT PLC Control** s otevřeným projektem
2. **Spusť navigator.exe**
3. Program automaticky:
   - Najde TwinCAT okno
   - Identifikuje správný project ListBox
   - Extrahuje a zobrazí strukturu stromu
   - Zaměří se na 25. položku

```bash
./navigator.exe
```

## 📊 Výstup

Program zobrazí:
- ✅ Informace o nalezeném TwinCAT okně
- 🔍 Proces hledání a výběru ListBox
- 📊 Počet nalezených položek
- 🌳 Hierarchickou strukturu projektu s ikonami
- 🎯 Potvrzení zaměření na požadovanou položku

### Příklad výstupu:
```
=== TWINCAT PROJECT NAVIGATOR ===

✅ TwinCAT nalezen: TwinCAT PLC Control - Projekt.pro* - [PLC_PRG (PRG-ST)]

🔍 Hledám project explorer ListBox...
  ListBox 0x00070404: pozice (270,950), velikost 1914x208, položek: 0, skóre: 120
  ListBox 0x00070400: pozice (10,87), velikost 234x1047, položek: 52, skóre: 256
✅ Nejlepší ListBox: 0x00070400 (skóre: 256)

📊 ListBox obsahuje 52 položek

🔍 Extrahuji položky stromu...
✅ Extrahováno 52 položek

=== STRUKTURA STROMU TWINCAT ===

[00] 📁 POUs
  [01] 📁 pBasic
    [02] 📄 PLC_PRG
  [03] 📁 Stations
  [04] 📁 ST_00
  ...
  [24] 📄 ST00_CallSFCs

🎯 Zaměřuji se na 25. položku...
✅ Focus nastaven na položku [24]: ST00_CallSFCs
```

## 🔬 Technické detaily

### Algoritmus extrakce textu:

1. **Dynamické hledání okna:** Najde TwinCAT okno podle titulku
2. **Identifikace ListBox:** Vyber nejlepší ListBox podle skóre (počet položek + pozice)
3. **Cross-process memory reading:** Použije `ReadProcessMemory` pro čtení struktury ItemData
4. **Text extraction:** Text pointer je na offsetu 20, text začíná na pozici +1 (přeskočí null byte)

### Typy položek (podle flags):
- `0x3604FD` = 📁 **FOLDER** (rozbalitelné složky)
- `0x704ED` = 📄 **FILE** (soubory/programy) 
- `0x4047D` = ⚙️ **SPECIAL** (speciální položky)
- `0x504DD` = 🔧 **ACTION** (akce/funkce)

### ItemData struktura:
```c
typedef struct {
    DWORD type;         // +0:  Typ struktury
    DWORD position;     // +4:  Pozice v hierarchii  
    DWORD flags;        // +8:  Typ položky
    DWORD hasChildren;  // +12: Má podpoložky
    DWORD padding;      // +16: Padding
    DWORD textPtr;      // +20: Pointer na text ⭐
    // ...
} ItemData;
```

## 📚 API Reference

### Hlavní funkce:

```c
HWND FindTwinCatWindow(void);
// Najde TwinCAT okno podle titulku

HWND FindProjectListBox(HWND parentWindow);  
// Najde nejlepší project ListBox podle skóre

bool ExtractTreeItem(HANDLE hProcess, HWND hListBox, int index, TreeItem* item);
// Extrahuje jednu položku stromu z paměti

void PrintTreeStructure(TreeItem* items, int count);
// Zobrazí hierarchickou strukturu

bool FocusOnItem(HWND hListBox, int index);
// Zaměří se na konkrétní položku
```

### TreeItem struktura:
```c
typedef struct {
    int index;              // Index v ListBoxu
    char text[256];         // Text položky
    DWORD flags;            // Typ položky  
    int level;              // Úroveň odsazení
    const char* type;       // Typ jako string
    const char* icon;       // Ikona pro zobrazení
    // ...
} TreeItem;
```

## ⚠️ Požadavky

- **Windows OS** (Windows API)
- **MinGW/GCC** nebo jiný C kompilátor
- **TwinCAT PLC Control** spuštěný s projektem
- **User32.dll, Kernel32.dll** (standardní Windows knihovny)

## 🐛 Řešení problémů

### Program nenajde TwinCAT okno:
- Ujisti se, že TwinCAT PLC Control je spuštěn
- Zkontroluj, že titulek obsahuje "TwinCAT"

### Nelze číst z paměti:
- Spusť program jako administrátor
- Zkontroluj, že TwinCAT proces má dostupná práva

### Nesprávný počet položek:
- Některé složky mohou být sbalené
- Program zobrazuje jen aktuálně viditelné položky

## 📋 Historie změn

- **v1.0** - Základní extrakce textu z owner-drawn ListBox
- **v1.1** - Dynamické hledání ListBox, modulární architektura
- **v1.2** - Identifikace typů položek, hierarchické zobrazení
- **v1.3** - Cleanup, dokumentace, finální verze

## 👨‍💻 Autor

Projekt vytvořen pro automatizaci TwinCAT navigace, řešení problémů s owner-drawn komponenty.

---

*Pro více informací nebo reportování chyb kontaktuj autora.*
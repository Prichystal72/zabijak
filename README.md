# TwinCAT Navigator Library

**Verze:** 3.0.0-stable  
**Datum:** 6. října 2025  
**Status:** ✅ Funkční knihovna pro čtení TwinCAT TreeView

Knihovna pro automatickou navigaci a čtení struktury TwinCAT PLC projektů přímo z paměti.

## 🎯 Účel

Navigator Library poskytuje robustní nástroje pro práci s TwinCAT projekty:

- ✅ **Window Detection** - Automatické nalezení TwinCAT okna a project explorer ListBoxu
- ✅ **Memory Reading** - Spolehlivé čtení ItemData struktury z paměti procesu
- ✅ **Text Extraction** - Inteligentní načítání textu s podporou různých offsetů (1 a 5)
- ✅ **Folder State Detection** - Dvě metody: `IsItemExpanded()` a `GetFolderState()` (structure[3])
- ✅ **Tree Manipulation** - `ToggleListBoxItem()` pro otevírání/zavírání složek
- ✅ **Complete Test Suite** - 7 funkčních testů pro validaci všech funkcí

## 📁 Struktura projektu (v3.0)

```
📦 twincat-navigator/
├── 📚 KNIHOVNA (CORE)
│   ├── lib/twincat_navigator.c/.h   # Hlavní navigační knihovna
│   │   ├── FindTwinCatWindow()      # Najde TwinCAT okno
│   │   ├── FindProjectListBox()     # Najde project explorer
│   │   ├── OpenTwinCatProcess()     # Otevře proces pro čtení
│   │   ├── GetListBoxItemCount()    # Počet položek v ListBoxu
│   │   ├── ExtractTreeItem()        # Čte položku z paměti (offset 1 a 5)
│   │   ├── GetFolderState()         # Stav složky (structure[3]: 0/1)
│   │   ├── IsItemExpanded()         # Stav složky (level comparison)
│   │   ├── ToggleListBoxItem()      # Otevře/zavře složku
│   │   └── PrintTreeStructure()     # Zobrazí strom
│   └── lib/twincat_search.c/.h      # Vyhledávací funkce (placeholder)
├── 🧪 TESTY (VŠECHNY FUNKČNÍ)
│   ├── test_show_all.exe            # ✅ Zobrazí všechny položky v ListBoxu
│   ├── test_basic_functions.exe     # ✅ Testy 5 základních funkcí
│   ├── test_folder_state.exe        # ✅ Porovnání IsItemExpanded vs GetFolderState
│   ├── test_simple_toggle.exe       # ✅ Test otevírání/zavírání složky
│   ├── test_decode_flags.exe        # ✅ Analýza flags (0x0205F5 vs 0x0205F7)
│   ├── test_structure_analysis.exe  # ✅ Analýza ItemData struktury
│   └── test_item_9_debug.exe        # ✅ Debug Serielle Kommunikation
├── 🔧 BUILD SKRIPTY
│   ├── build64.bat                  # Build pro 64-bit
│   ├── build32.bat                  # Build pro 32-bit
│   ├── cleanup.bat                  # Vyčistí workspace
│   └── cleanup_tests.bat            # Vyčistí testy
└── 📖 DOKUMENTACE
    ├── README.md                    # Tento soubor
    ├── README_API.md                # API dokumentace
    └── PROJECT_MAP.md               # Mapa architektury
```

## 🔧 Kompilace

### 🚀 Kompilace testů:
```bash
cd tests
gcc -o test_show_all.exe test_show_all.c ../lib/twincat_navigator.c -luser32 -lpsapi -I..
gcc -o test_folder_state.exe test_folder_state.c ../lib/twincat_navigator.c -luser32 -lpsapi -I..
```

### 📝 Rychlé spuštění testů:

**1. Zobrazit všechny položky:**
```powershell
cd tests ; .\test_show_all.exe
```

**2. Test detekce stavu složek:**
```powershell
cd tests ; .\test_folder_state.exe
```

**3. Test základních funkcí:**
```powershell
cd tests ; .\test_basic_functions.exe
```

## ✅ Funkční testy

Všech **7 testů** je plně funkčních:

| Test | Status | Popis |
|------|--------|-------|
| `test_show_all` | ✅ | Zobrazí všechny viditelné položky s indexy, flags a levely |
| `test_basic_functions` | ✅ | Validuje 5 základních funkcí knihovny |
| `test_folder_state` | ✅ | Porovnává `IsItemExpanded()` vs `GetFolderState()` |
| `test_simple_toggle` | ✅ | Testuje otevírání/zavírání složek |
| `test_decode_flags` | ✅ | Analyzuje význam flags (0x0205F5 = POUs, 0x0205F7 = folders) |
| `test_structure_analysis` | ✅ | Odhaluje strukturu ItemData (structure[3] = folder state) |
| `test_item_9_debug` | ✅ | Debug položky s offsetem 5 (Serielle Kommunikation) |

### Testovací workflow:

1. **Otevři TwinCAT projekt** v TwinCAT PLC Control
2. **Spusť test** (např. `.\test_show_all.exe`)
3. **Ručně upravuj složky** v TwinCAT (otevírej/zavírej)
4. **Spusť test znovu** pro kontrolu změn

## 🔍 Klíčové objevy

### ItemData struktura:
```c
structure[0] = 0x01621ED0  // Pointer (parent/meta)
structure[1] = level       // 0, 1, 2... (hierarchie)
structure[2] = flags       // 0x0205F5 (POUs), 0x0205F7 (folders)
structure[3] = folder_state // 0 = zavřená, 1 = otevřená ⭐
structure[5] = text_ptr    // Pointer na text
```

### Text offset:
- **Většina položek**: offset **1** (za null bytem)
- **Některé položky**: offset **5** (za DWORD metadata + null byte)
- **Knihovna**: automaticky zkouší oba offsety

### Folder state detection:
- **`GetFolderState()`**: Čte `structure[3]` - **spolehlivější** ⭐
- **`IsItemExpanded()`**: Porovnává levely - kompatibilní fallback
```

### Ruční kompilace:
```bash
# Hlavní smart navigator
gcc -o twincat_navigator_main.exe twincat_navigator_main.c twincat_project_parser.c twincat_path_finder.c lib/twincat_navigator.c -luser32 -lpsapi -ladvapi32 -lcomctl32

# Legacy navigator (reference)  
gcc -o navigator.exe navigator.c lib/twincat_navigator.c twincat_project_parser.c -luser32 -lgdi32 -lcomctl32 -lpsapi
```

### Alternativní build skripty:
```bash
build64.bat         # Legacy 64-bit verze
build32.bat         # Legacy 32-bit verze
```

## 🚀 Použití

### Hlavní Smart Navigator:
1. **Spusť TwinCAT PLC Control** s otevřeným projektem
2. **Spusť twincat_navigator_main.exe**
3. Program automaticky:
   - ✅ Najde TwinCAT okno a extrahuje název projektu
   - 🔍 Použije external path finder (3 strategie)
   - 📋 Zparsuje projekt dual-mode parserem (84.4% přesnost)
   - 📊 Najde správný ListBox a analyzuje memory
   - 📝 Exportuje struktury do .txt souborů
   - 🎮 Zobrazí interaktivní menu pro navigaci

```bash
./twincat_navigator_main.exe
```

## 📊 Současný výstup (v2.0)

### Smart Navigator workflow:
```
=== TwinCAT Smart Navigator - Clean Version ===

🔍 Hledám TwinCAT okno...
   🪟 TwinCAT okno: 'TwinCAT PLC Control - BA17xx.pro* - [ST_Hublift_rechts (PRG-ST)]'
   📄 Název souboru: 'BA17xx.pro'

🔍 Hledám cestu k projektu...
✅ Projekt nalezen: C:\Users\ept\Desktop\PLC\BA17xx.pro

📋 Parsuju strukturu projektu...
✅ Projekt naparsován: 141 elementů

📋 Hledám ListBox okno...
✅ ListBox nalezen: 0x000603f2

📝 Exportuji struktury do souborů...
✅ Kompletní struktura exportována do: BA17xx.pro_complete_structure.txt
⚠️  Stav ListBox exportován do: BA17xx.pro_listbox_state.txt (memory reading debugging)

🎯 === SMART NAVIGATION MENU ===
 1. [ (PRG)] BOOLARRAY_TO_BYTE
 2. [ (PRG)] BOOLARRAY_TO_DWORD
 ... dalších 121 elementů

Zadejte číslo elementu pro navigaci (0 = konec):
```

## 🔍 Současné problémy (debugging v průběhu)

### ⚠️ **PRIORITY:** Memory Reading Issue
- **Problém:** ExtractTreeItem() v lib/twincat_navigator.c nečte správně text z ListBox paměti
- **Status:** Debugging v průběhu - algoritmus detectuje struktury ale text je prázdný
- **Workaround:** Funkční reference v tests/final_extractor.c (offset-20 algoritmus)

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
- **Standard knihovny:** user32, psapi, advapi32, comctl32

## 🐛 Řešení problémů

### Program nenajde TwinCAT okno:
- Ujisti se, že TwinCAT PLC Control je spuštěn
- Zkontroluj, že titulek obsahuje "TwinCAT"

### Nelze číst z paměti:
- Spusť program jako administrátor
- Zkontroluj, že TwinCAT proces má dostupná práva

### Memory reading problémy (současný stav):
- ExtractTreeItem() debugging v průběhu
- Reference implementace v tests/final_extractor.c

---

## 📋 Changelog & Roadmap

### v2.0.0-alpha (Říjen 2025) - 🔧 Současná verze
**✅ DOKONČENO:**
- Reorganizace projektu a modularizace
- Dual-mode parser (84.4% přesnost na starších formátech)
- External path finder modul (3 strategie)
- Smart ListBox detection
- Export & compare functionality

**🔧 DEBUGGING:**
- ExtractTreeItem() memory reading
- ListBox text extraction algorithm

**🎯 PLÁNOVÁNO:**
- Fix memory reading algorithm
- Complete navigation functionality
- Support for newer TwinCAT formats (100%)
- Unit tests & validation suite

### v1.x (legacy)
- Původní single-file implementace
- Basic owner-drawn ListBox support
- Manual memory analysis tools

---

## 👨‍💻 Development Status

**Aktivní vývoj:** ✅ Ano  
**Posledních update:** 1. října 2025  
**Hlavní vývojář:** [Uživatel]  
**Licence:** Open Source

**Pro detailní architekturu viz:** `PROJECT_MAP.md`
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
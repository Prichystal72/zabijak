# TC2 Navigator - Globální navigátor pro TwinCAT 2

**Verze:** 4.0  
**Datum:** 30. října 2025  
**Status:** ✅ Funkční aplikace s globální klávesovou zkratkou

Aplikace pro automatickou navigaci v TwinCAT 2 Project Explorer pomocí globální klávesové zkratky.

---

## 🚀 Rychlý start

### 1. Kompilace
```bash
gcc -o TC2_navigator.exe TC2_navigator.c ^
    lib/twincat_navigator.c lib/twincat_tree.c ^
    lib/twincat_cache.c lib/twincat_search.c ^
    -luser32 -lpsapi -lcomctl32
```

### 2. Spuštění
```bash
TC2_navigator.exe
```

Aplikace běží na pozadí a zachytává globální klávesové zkratky.

### 3. Použití

1. **Otevři TwinCAT 2 projekt** (System Manager nebo PLC Control)
2. **Otevři POU/funkci** (například MAIN nebo ST_Funkce) - titulek okna se změní na "MAIN (PRG) - TwinCAT..."
3. **Stiskni `Ctrl+Shift+A`** - aplikace automaticky:
   - Extrahuje název POU z titulku okna
   - Najde ho v Project Explorer
   - Expanduje cestu
   - Klikne na položku a otevře ji

### 4. Klávesové zkratky

- **`Ctrl+Shift+A`** - Navigovat na aktuální POU (podle titulku okna)
- **`Ctrl+Shift+Q`** - Ukončit aplikaci

---

## ⚠️ Důležitá upozornění

### Při prvním spuštění (generování cache)
- **NEMINIMALIZUJ TwinCAT okno** - cache se musí vytvořit při viditelném Project Explorer
- **NEKLIKEJ nikam během generování** - aplikace automaticky prochází celý strom projektu
- **POČKEJ dokud se nedokončí JSON soubor** (`twincat_tree_cache.json`)
- **Trvá ~5-10 sekund** podle velikosti projektu

Aplikace během generování:
1. Otevře všechny složky v projektu (POUs, GVLs, Function Blocks...)
2. Načte kompletní strukturu do paměti
3. Uloží do JSON souboru
4. Zavře všechny složky zpět
5. Provede POUs reset (dvojité kliknutí pro synchronizaci)

### Problémy s minimalizovaným oknem
- **Chyba při minimalizaci:** Pokud je TwinCAT okno minimalizované během navigace, může dojít k desynchronizaci cache indexů
- **Řešení:** Před použitím `Ctrl+Shift+A` vždy obnovte (restore) TwinCAT okno
- Aplikace provádí automatický POUs reset před navigací pro synchronizaci stavu

---

## 📖 Jak to funguje

### První spuštění (vytvoření cache)
1. Aplikace najde TwinCAT okno a ListBox s project stromem
2. Expanduje všechny složky v projektu
3. Načte celou strukturu do paměti (všechny POU, GVL, složky)
4. Uloží do `twincat_tree_cache.json`
5. Zavře všechny složky zpět

**Trvá:** ~5-10 sekund (závisí na velikosti projektu)

### Další spuštění (rychlá navigace)
1. Načte cache z JSON souboru (rychlé, bez expandování)
2. Po stisku `Ctrl+Shift+A`:
   - Extrahuje název z titulku (např. "MAIN (PRG)" → "MAIN")
   - Najde položku v cache
   - Parsuje cestu (např. "POUs/MAIN")
   - Postupně expanduje složky v cestě
   - Klikne na cílovou položku

**Trvá:** <1 sekunda

---

## 🔧 Jak funguje cache systém

### Struktura cache (JSON)
```json
{
  "project": "POUs",
  "timestamp": "2025-10-30T07:44:46",
  "itemCount": 300,
  "items": [
    {
      "index": 0,
      "text": "POUs",
      "level": 0,
      "path": "POUs",
      "hasChildren": 1,
      "flags": 3605757
    },
    {
      "index": 15,
      "text": "MAIN",
      "level": 2,
      "path": "POUs/MAIN",
      "hasChildren": 0,
      "flags": 459005
    }
  ]
}
```

### Kdy smazat cache?
Cache je potřeba přegenerovat pokud:
- Přidáš/odstraníš POU v projektu
- Přejmenujete složky nebo POU
- Změníš strukturu projektu

**Řešení:** Smaž `twincat_tree_cache.json` a restartuj `TC2_navigator.exe`

---

## 📚 Architektura

### Hlavní soubory

**TC2_navigator.c** - Hlavní aplikace
- Globální keyboard hook (WH_KEYBOARD_LL)
- Zachytává Ctrl+Shift+A
- Volá navigační funkce

**lib/twincat_navigator.c** - Základní funkce
- `FindTwinCatWindow()` - Najde TwinCAT okno
- `FindProjectListBox()` - Najde ListBox s projektem (scoring algoritmus)
- `ExtractTreeItem()` - Čte paměť TwinCAT a extrahuje TreeItem

**lib/twincat_cache.c** - Cache systém
- `ExpandAllFoldersWrapper()` - Expanduje všechny složky
- `LoadFullTree()` - Načte celý strom
- `SaveCacheToFile()` - Uloží do JSON
- `LoadCacheFromFile()` - Načte z JSON

**lib/twincat_tree.c** - Navigace
- `FindAndExpandPath()` - Najde a expanduje cestu k položce

**lib/twincat_search.c** - Extrakce názvu
- `ExtractTargetFromTitle()` - Parsuje titulek okna

---

## 🛠️ Testovací utility

### test_tree_cache.exe
Vytvoří/obnoví cache soubor ručně
```bash
cd tests
test_tree_cache.exe
```

### test_find_item.exe
Testuje vyhledání konkrétní položky
```bash
test_find_item.exe "MAIN"
```

### test_hook_simple.exe
Testuje keyboard hook (bez TwinCAT navigace)
```bash
test_hook_simple.exe
```

---

## ⚙️ Technické detaily

### Jak funguje čtení paměti TwinCAT?
1. `FindProjectListBox()` najde ListBox kontrolu
2. `SendMessage(LB_GETITEMDATA)` získá pointer na ItemData strukturu
3. `ReadProcessMemory()` čte strukturu z paměti TwinCAT procesu
4. Struktura obsahuje: level [1], flags [2], hasChildren [3], textPtr [5]

### Scoring algoritmus pro hledání ListBoxu
```c
int score = itemCount + height / 10;
if (rect.left < windowWidth / 3) score += 100; // Levá pozice
```

Preferuje:
- Levou pozici v okně (+100 bodů)
- Vysoký ListBox (+height/10)
- Hodně položek (+itemCount)

---

## 🐛 Známé problémy

### LoadCacheFromFile počítá o 1 více
Parser počítá každý `}` jako konec položky, včetně závěrečného `}` pole items.
→ Vrací 301 místo 300

**Dopad:** Minimální, navigace funguje správně

**Fix:** Změnit parsování na počítání pouze `},` místo všech `}`

---
   - Dramaticky rychlejší než původní metoda
   - **Automatické odstranění modrého zvýraznění** - dvojité kliknutí na POUs na konci

4. **JSON export** (`SaveCacheToFile`)
   - Timestamp vytvoření
   - Kompletní metadata (index, text, level, path, hasChildren, flags)
   - Čitelný formát pro další zpracování

**Výstupní soubor:** `twincat_tree_cache.json` (300+ položek)

**Příklad JSON struktury:**
```json
{
  "project": "POUs",
  "timestamp": "2025-10-17T10:46:37",
  "itemCount": 300,
  "items": [
    {
      "index": 0,
      "text": "POUs",
      "level": 0,
      "path": "POUs",
      "hasChildren": true,
      "flags": "0x1404C5"
    },
    {
      "index": 1,
      "text": "pBasic",
      "level": 1,
      "path": "POUs/pBasic",
      "hasChildren": true,
      "flags": "0x1404C5"
    }
    ...
  ]
}
```

---

## 🎯 Účel

Navigator Library poskytuje robustní nástroje pro práci s TwinCAT projekty:

- ✅ **Smart projekt detection** - Automatické nalezení TwinCAT oken a projektů
- ✅ **Dual-mode parsing** - Podpora starších i novějších TwinCAT formátů (84.4% přesnost)
- ✅ **External path finder** - Modularní hledání projektů (3 strategie)
- ✅ **Memory-based navigation** - Čtení z owner-drawn ListBox komponent
- ✅ **Export & Compare** - Srovnání file struktury vs aktuální stav
- ⚠️ **Memory reading** - ExtractTreeItem() debugging v průběhu

## 📁 Struktura projektu (v3.1)


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
│   │   ├── CollapseAllFolders()     # Zavře všechny složky (starší metoda)
│   │   ├── ExpandAllFolders()       # Otevře všechny složky (starší metoda)
│   │   └── PrintTreeStructure()     # Zobrazí strom
│   ├── lib/twincat_cache.c/.h       # 🆕 Cache systém pro práci se strukturou
│   │   ├── GetProjectName()         # Získá název projektu
│   │   ├── ExpandAllFoldersWrapper()# Inteligentní expandování (až 100 iterací)
│   │   ├── LoadFullTree()           # Načte celý strom do cache
│   │   ├── SaveCacheToFile()        # Export do JSON souboru
│   │   ├── LoadCacheFromFile()      # Import z JSON souboru
│   │   ├── FindInCache()            # Vyhledání v cache podle textu
│   │   └── CollapseAllFoldersFromCache() # Optimalizované zavírání pomocí cache
│   └── lib/twincat_search.c/.h      # Vyhledávací funkce (placeholder)
├── 🧪 TESTY (VŠECHNY FUNKČNÍ)
│   ├── test_show_all.exe            # ✅ Zobrazí všechny položky v ListBoxu
│   ├── test_tree_cache.exe          # ✅ 🆕 Export celé struktury do JSON
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

### 🚀 Kompilace nových testů:
```bash
cd tests
# Zobrazení aktuálního stavu
gcc -o test_show_all.exe test_show_all.c ../lib/twincat_navigator.c -luser32 -lpsapi -I..

# Export celé struktury do JSON
gcc -o test_tree_cache.exe test_tree_cache.c ../lib/twincat_navigator.c ../lib/twincat_cache.c -luser32 -lpsapi -I..
```

### 📝 Rychlé spuštění testů:

**1. Zobrazit všechny položky:**
```powershell
cd tests ; .\test_show_all.exe
```

**2. Export celé struktury do JSON:**
```powershell
cd tests ; .\test_tree_cache.exe
# Vytvoří soubor: twincat_tree_cache.json
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
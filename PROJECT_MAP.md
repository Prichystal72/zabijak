# 🗺️ MAPA PROJEKTU: TwinCAT Smart Navigator

## 📁 **HLAVNÍ KOMPONENTY**

### 🎯 **HLAVNÍ PROGRAM**
```
twincat_navigator_main.c     - Hlavní smart navigator s menu systémem
twincat_navigator_main.exe   - Zkompilovaný hlavní program
```

### 🔧 **CORE MODULY**
```
twincat_project_parser.c/.h  - Dual-mode parser TwinCAT projektů (84.4% přesnost)
twincat_path_finder.c/.h     - Externí modul pro hledání cest k projektům  
lib/twincat_navigator.c/.h   - ListBox detekce a memory reading funkce
```

### 🗂️ **LEGACY/REFERENCE**
```
navigator.c                  - Původní verze navigatoru (referenční)
navigator.exe               - Původní executable
```

## 🔍 **ARCHITEKTURA HLAVNÍHO PROGRAMU**

### **twincat_navigator_main.c** - Workflow:
```
1. 🔍 Find TwinCAT Window
   ├── Scan all windows for "TwinCAT" title
   └── Extract filename from window title
   
2. 📂 Find Project Path  
   ├── Call: twincat_path_finder.c
   ├── Methods: Common paths → Registry → Memory search
   └── Returns: Full path to .pro file
   
3. 📋 Parse Project Structure
   ├── Call: twincat_project_parser.c
   ├── Detect format: New (POUs) vs Old (fallback)
   └── Extract: 141 elements with types (PRG/FB/FUN)
   
4. 📊 Find ListBox Window
   ├── Call: lib/twincat_navigator.c → FindProjectListBox()
   ├── Score-based selection of best ListBox
   └── Returns: HWND of project explorer ListBox
   
5. 📝 Export Structures
   ├── Complete structure → BA17xx.pro_complete_structure.txt
   └── ListBox state → BA17xx.pro_listbox_state.txt
   
6. 🎮 Interactive Menu
   ├── Display 20 first elements
   ├── Accept user input (element number)
   └── Navigate to selected element
```

## 🧩 **MODULE DEPENDENCIES**

### **twincat_path_finder.c** - 3 strategie:
```
FindTwinCATProjectPath()
├── 1. FindProjectInCommonPaths()   - Rychlé hledání v obvyklých místech
├── 2. FindProjectInRegistry()      - Hledání v Windows Registry  
└── 3. FindProjectByMemorySearch()  - Scan procesu v paměti
```

### **twincat_project_parser.c** - Dual-mode:
```
tc_parser_parse()
├── Detect: POUs section exists?
├── YES → Modern parser (structured XML-like)
└── NO  → Fallback parser (pattern matching)
```

### **lib/twincat_navigator.c** - Memory functions:
```
FindProjectListBox()          - Najde správný ListBox (score-based)
ExtractTreeItem()            - Čte text z ListBox item (memory reading)
OpenTwinCatProcess()         - Otevře proces pro memory access
```

## 📊 **CURRENT STATUS**

### ✅ **FUNKČNÍ KOMPONENTY:**
- ✅ Window detection (TwinCAT window nalezen)
- ✅ Path finding (external module funguje) 
- ✅ Project parsing (141 elementů, 84.4% přesnost)
- ✅ ListBox detection (správný ListBox nalezen)

### ⚠️ **PROBLÉMY K ŘEŠENÍ:**
- ❌ ExtractTreeItem() - memory reading nefunguje správně
- ❌ Export ListBox state - vrací prázdné texty
- ❌ Navigation - potřebuje opravené memory reading

## 🎯 **NEXT STEPS:**

1. **Debug ExtractTreeItem()** v lib/twincat_navigator.c
2. **Analyze memory structure** ListBox items
3. **Fix text extraction** pro získání správných názvů
4. **Test navigation** functionality

## 📂 **TESTOVACÍ SOUBORY** (v tests/):
```
tests/final_extractor.c      - Funkční offset-20 algoritmus (reference)
tests/test_path_finder.c     - Testy path finder modulu
tests/reference_structure_parser.c - Parser pro CELA.EXP
```

## 🗃️ **DATA SOUBORY:**
```
BA17xx.pro                   - Testovací TwinCAT projekt (starší formát)
Palettierer.pro             - Testovací projekt (novější formát)
cela/CELA.EXP               - Referenční export struktura
palettierer/PALETIZER.EXP   - Export struktura pro novější formát
```
# Memory Analyzer pro Owner-Drawn Fixed ListBox

## 🎯 Účel
Memory Analyzer je specializovaný nástroj pro analýzu paměti owner-drawn fixed ListBox komponent v TwinCAT aplikacích. Program extrahuje surová data z paměti a exportuje je do hex formátu pro detailní analýzu.

## 🔍 Funkce

### Základní analýza:
- ✅ **Automatické nalezení** TwinCAT okna a owner-drawn fixed ListBox
- ✅ **Memory dump** ItemData struktur do hex formátu
- ✅ **Text extraction** z memory pointerů s multiple offset podporou
- ✅ **Strukturální analýza** paměťových bloků
- ✅ **Detailní reporting** s přehlednými výstupy

### Hex výstupy:
- 📄 **memory_dump.hex** - Kompletní hex dump všech analyzovaných bloků
- 📄 **analysis_summary.txt** - Přehled nalezených struktur a textů

## 🚀 Použití

### Kompilace:
```bash
.\build_memory_analyzer.bat
```

### Spuštění:
1. **Spusť TwinCAT PLC Control** s otevřeným projektem
2. **Spusť memory analyzer:**
```bash
.\memory_analyzer.exe
```

## 📊 Výstup programu

```
=== MEMORY ANALYZER PRO OWNER-DRAWN FIXED LISTBOX ===
=== ANALÝZA PAMĚTI A HEX DUMP ===

🔍 Hledám TwinCAT okno...
✅ TwinCAT okno: 'TwinCAT PLC Control - BA17xx.pro*'
✅ Potvrzeno: Owner-drawn fixed ListBox

🔓 Otevírám proces pro čtení paměti...
✅ Proces otevřen pro čtení paměti

📊 Celkem položek v ListBox: 52
📋 Analyzuji prvních 20 položek...

🔍 Analyzuji položku 0...
   📍 ItemData pointer: 0x12345678
   ✅ Struktura analyzována (256 bytů)
   🔗 Nalezeno 3 možných text pointerů
   📝 Extrahováno 1 platných textů

💾 Vytvářím hex dump soubory...
✅ Hex dump vytvořen: memory_dump.hex
✅ Přehled analýzy vytvořen: analysis_summary.txt

✅ Analýza dokončena!
```

## 📁 Výsledné soubory

### memory_dump.hex
Obsahuje hex dump všech analyzovaných memory bloků:
```
=== MEMORY ANALYZER HEX DUMP ===
=== Block 1 (0x12345678) ===
Adresa: 0x12345678
Velikost: 256 bytů

12345678: 01 23 45 67 89 AB CD EF 00 11 22 33 44 55 66 77  | .#Eg......."3DUfw
12345688: 88 99 AA BB CC DD EE FF 00 01 02 03 04 05 06 07  | ................
...
```

### analysis_summary.txt
Přehled nalezených struktur:
```
=== MEMORY ANALYSIS SUMMARY ===
--- Položka 0 ---
ItemData pointer: 0x12345678
Velikost struktury: 256 bytů
Nalezené text pointery: 3
  Pointer[0]: 0x87654321
  Pointer[1]: 0x13579BDF
Extrahované texty: 1
  Text[0]: 'POUs'
```

## 🔧 Technické detaily

### Algoritmus analýzy:
1. **Nalezení owner-drawn fixed ListBox** - Ověření window style LBS_OWNERDRAWFIXED
2. **ItemData extraction** - Získání pointerů na ItemData struktury
3. **Memory reading** - Čtení pamětových bloků (až 64 DWORDs na strukturu)
4. **Pointer validation** - Kontrola platnosti memory pointerů (0x400000-0x80000000)
5. **Text extraction** - Pokus o extrakci textů s offsety 0,1,2,4
6. **Hex formatting** - Export do standardního hex formátu

### Podporované formáty:
- ✅ **Owner-drawn fixed ListBox** (LBS_OWNERDRAWFIXED)
- ✅ **32-bit memory addresses** 
- ✅ **ASCII text extraction**
- ✅ **Multiple offset detection** (0, 1, 2, 4 byte offsets)

## ⚠️ Požadavky

- **Windows OS** (Windows API)
- **TwinCAT PLC Control** spuštěný s projektem
- **Administrátorská práva** (pro memory reading)
- **MinGW/GCC** kompilátor

## 🔗 Integrace

Memory Analyzer používá existující komponenty z TwinCAT Smart Navigator:
- `lib/twincat_navigator.c/.h` - Window detection a ListBox finding
- Kompatibilní s existujícím build systemem

## 📋 Roadmap

### Plánované vylepšení:
- 🎯 **Pattern recognition** - Automatické rozpoznání typů struktur
- 🎯 **Advanced text extraction** - Podpora Unicode a dalších kódování
- 🎯 **Memory mapping** - Vizualizace memory layout
- 🎯 **Diff analysis** - Porovnání změn v čase
- 🎯 **Export formáty** - JSON, XML, CSV výstupy
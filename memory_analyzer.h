#ifndef MEMORY_ANALYZER_H
#define MEMORY_ANALYZER_H

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Konstanty pro memory analyzér
#define MAX_MEMORY_DUMP 4096
#define HEX_BYTES_PER_LINE 16
#define MAX_STRUCTURE_SIZE 256

// Struktura pro surové memory data
typedef struct {
    DWORD_PTR address;
    size_t size;
    unsigned char data[MAX_MEMORY_DUMP];
    size_t actualRead;
} MemoryBlock;

// Struktura pro analýzu ItemData
typedef struct {
    DWORD_PTR itemDataPtr;
    DWORD structure[64];  // Rozšířená struktura pro detailní analýzu
    size_t structureSize;
    DWORD_PTR textPointers[10];  // Možné text pointery
    int textPointerCount;
    char extractedTexts[10][256];  // Extrahované texty
    int validTextCount;
} ItemDataAnalysis;

// Funkce pro hex analysis
void DumpMemoryToHex(const char* filename, MemoryBlock* blocks, int blockCount);
void HexDumpBlock(FILE* f, MemoryBlock* block, const char* description);
bool AnalyzeItemData(HANDLE hProcess, DWORD_PTR itemDataPtr, ItemDataAnalysis* analysis);
bool ExtractTextFromPointer(HANDLE hProcess, DWORD_PTR textPtr, char* buffer, size_t bufferSize);
void PrintAnalysisSummary(ItemDataAnalysis* analyses, int count);
bool IsValidTextPointer(DWORD_PTR ptr);
void AnalyzeStructurePattern(DWORD* structure, size_t size, FILE* logFile);

// Funkce pro pokročilou analýzu
void AnalyzeMemoryPatterns(MemoryBlock* blocks, int blockCount, const char* reportFile);
void FindCommonStructures(ItemDataAnalysis* analyses, int count);
void ExportDetailedReport(ItemDataAnalysis* analyses, int count, const char* filename);

#endif // MEMORY_ANALYZER_H
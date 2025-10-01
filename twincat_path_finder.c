// twincat_path_finder.c
// Implementation of TwinCAT project path finding functions

#include "twincat_path_finder.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Main function to find TwinCAT project path using multiple methods
BOOL FindTwinCATProjectPath(const char* filename, char* result_path, DWORD max_path_len) {
    printf("🔍 Hledám cestu k projektu: %s\n", filename);
    
    // Method 1: Try common paths (fastest and most reliable)
    if (FindProjectByCommonPaths(filename, result_path)) {
        printf("✅ Nalezeno v obvyklých místech: %s\n", result_path);
        return TRUE;
    }
    
    // Method 2: Try registry
    if (FindProjectByRegistry(filename, result_path)) {
        printf("✅ Nalezeno v registry: %s\n", result_path);
        return TRUE;
    }
    
    // Method 3: Memory search (last resort)
    HWND hwnd = FindWindow(NULL, NULL);
    char title[512];
    
    do {
        hwnd = FindWindowEx(NULL, hwnd, NULL, NULL);
        if (hwnd && GetWindowText(hwnd, title, sizeof(title))) {
            if (strstr(title, "TwinCAT") && strstr(title, filename)) {
                DWORD pid;
                GetWindowThreadProcessId(hwnd, &pid);
                HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, pid);
                
                if (hProcess) {
                    if (FindProjectByMemorySearch(hProcess, filename, result_path)) {
                        CloseHandle(hProcess);
                        printf("✅ Nalezeno v paměti: %s\n", result_path);
                        return TRUE;
                    }
                    CloseHandle(hProcess);
                }
                break;
            }
        }
    } while (hwnd);
    
    printf("❌ Projekt nenalezen žádnou metodou\n");
    return FALSE;
}

// Method 1: Search in common locations
BOOL FindProjectByCommonPaths(const char* filename, char* result_path) {
    printf("   📁 Zkouším obvyklá místa...\n");
    
    // Get current user name
    char username[256];
    DWORD username_len = sizeof(username);
    GetUserName(username, &username_len);
    
    // Build common paths
    char* path_templates[] = {
        "C:\\Users\\%s\\Desktop\\PLC\\%s",
        "C:\\Users\\%s\\Desktop\\%s",
        "C:\\Users\\%s\\Documents\\%s",
        "C:\\Users\\%s\\Documents\\PLC\\%s",
        "C:\\TwinCAT\\Projects\\%s",
        "C:\\Projects\\%s",
        "D:\\Projects\\%s",
        NULL
    };
    
    for (int i = 0; path_templates[i]; i++) {
        char test_path[MAX_PATH];
        snprintf(test_path, sizeof(test_path), path_templates[i], username, filename);
        
        printf("      🔍 %s\n", test_path);
        
        FILE* f = fopen(test_path, "r");
        if (f) {
            fclose(f);
            strcpy(result_path, test_path);
            return TRUE;
        }
    }
    
    return FALSE;
}

// Method 2: Search in Windows registry
BOOL FindProjectByRegistry(const char* filename, char* result_path) {
    printf("   📋 Zkouším registry...\n");
    
    HKEY hKey;
    char buffer[1024];
    DWORD bufferSize;
    
    // Try TwinCAT3 recent projects
    char* registry_keys[] = {
        "SOFTWARE\\Beckhoff\\TwinCAT3\\3.1\\IDE\\General",
        "SOFTWARE\\Beckhoff\\TwinCAT3\\3.1\\Components\\Plc\\PlcAppCtrl",
        "SOFTWARE\\Beckhoff\\TwinCAT3\\3.1",
        NULL
    };
    
    char* value_names[] = {
        "LastProjectPath",
        "LastProject",
        "RecentProject1",
        "RecentProject2", 
        "RecentProject3",
        NULL
    };
    
    for (int i = 0; registry_keys[i]; i++) {
        if (RegOpenKeyEx(HKEY_CURRENT_USER, registry_keys[i], 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            printf("      📖 Prohledávám: %s\n", registry_keys[i]);
            
            for (int j = 0; value_names[j]; j++) {
                bufferSize = sizeof(buffer);
                if (RegQueryValueEx(hKey, value_names[j], NULL, NULL, 
                                   (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
                    
                    if (strstr(buffer, filename)) {
                        printf("      🎯 Našel v '%s': %s\n", value_names[j], buffer);
                        
                        FILE* f = fopen(buffer, "r");
                        if (f) {
                            fclose(f);
                            strcpy(result_path, buffer);
                            RegCloseKey(hKey);
                            return TRUE;
                        }
                    }
                }
            }
            
            RegCloseKey(hKey);
        }
    }
    
    return FALSE;
}

// Method 3: Search in process memory (fallback)
BOOL FindProjectByMemorySearch(HANDLE hProcess, const char* filename, char* result_path) {
    printf("   💾 Zkouším paměť procesu...\n");
    
    MEMORY_BASIC_INFORMATION mbi;
    char* addr = 0;
    char buffer[4096];
    int regions_checked = 0;
    const int MAX_REGIONS = 500; // Limit for performance
    
    while (VirtualQueryEx(hProcess, addr, &mbi, sizeof(mbi)) && regions_checked < MAX_REGIONS) {
        regions_checked++;
        
        if (mbi.State == MEM_COMMIT && 
            (mbi.Protect & PAGE_READWRITE || mbi.Protect & PAGE_READONLY)) {
            
            SIZE_T bytesRead;
            if (ReadProcessMemory(hProcess, mbi.BaseAddress, buffer, 
                                 min(sizeof(buffer), mbi.RegionSize), &bytesRead)) {
                
                // Search for filename in memory
                for (SIZE_T i = 0; i < bytesRead - strlen(filename); i++) {
                    if (strncmp(&buffer[i], filename, strlen(filename)) == 0) {
                        // Found filename, try to extract full path
                        int start = i;
                        while (start > 0 && buffer[start-1] != '\0' && buffer[start-1] >= 32) {
                            start--;
                            if (start >= 0 && buffer[start] >= 'A' && buffer[start] <= 'Z' && 
                                start + 1 < bytesRead && buffer[start+1] == ':') {
                                break; // Found drive letter
                            }
                        }
                        
                        int end = i + strlen(filename);
                        while (end < bytesRead && buffer[end] != '\0' && buffer[end] >= 32 && buffer[end] <= 126) {
                            end++;
                        }
                        
                        int path_len = end - start;
                        if (path_len > 0 && path_len < MAX_PATH) {
                            char candidate[MAX_PATH];
                            strncpy(candidate, &buffer[start], path_len);
                            candidate[path_len] = '\0';
                            
                            if (strstr(candidate, ":\\") && strstr(candidate, filename)) {
                                FILE* f = fopen(candidate, "r");
                                if (f) {
                                    fclose(f);
                                    strcpy(result_path, candidate);
                                    return TRUE;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        addr = (char*)mbi.BaseAddress + mbi.RegionSize;
    }
    
    printf("      ❌ Nenalezeno v %d oblastech paměti\n", regions_checked);
    return FALSE;
}
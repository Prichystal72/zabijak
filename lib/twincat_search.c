#include "twincat_search.h"
#include "twincat_navigator.h"
#include <stdio.h>
#include <string.h>
#include <psapi.h>

// Interní struktura pro uchování informací o položce
typedef struct {
    DWORD_PTR itemData;
    char text[256];
    int level;
    bool hasChildren;
    DWORD flags;
} ItemInfo;

// Interní kontext pro vyhledávání
typedef struct {
    HWND hListBox;
    HANDLE hProcess;
    const char* targetName;
    char path[SEARCH_MAX_PATH];
    int pathLen;
    bool found;
} SearchContext;

// Načte informace o položce na dané pozici
static bool GetItemInfo(HWND listbox, HANDLE hProcess, int position, ItemInfo* info) {
    if (!info) return false;

    LRESULT itemData = SendMessage(listbox, LB_GETITEMDATA, position, 0);
    if (itemData == LB_ERR || itemData == 0) {
        return false;
    }

    DWORD structure[STRUCT_FIELD_COUNT] = {0};
    SIZE_T bytesRead = 0;

    if (!ReadProcessMemory(hProcess, (void*)itemData, structure, sizeof(structure), &bytesRead) || bytesRead < 24) {
        return false;
    }

    info->itemData = (DWORD_PTR)itemData;
    info->level = (int)structure[1];
    info->hasChildren = (structure[3] != 0);
    info->flags = structure[2];
    info->text[0] = '\0';

    DWORD textPtr = structure[5];
    if (textPtr > 0x400000 && textPtr < 0x80000000) {
        char textBuffer[256];
        SIZE_T textRead = 0;

        if (ReadProcessMemory(hProcess, (void*)textPtr, textBuffer, sizeof(textBuffer)-1, &textRead)) {
            for (int offset = 0; offset < 4 && offset < (int)textRead; offset++) {
                char* text = textBuffer + offset;
                size_t len = 0;

                while (len < 200 && offset + len < textRead) {
                    char c = text[len];
                    if (c >= 32 && c <= 126) {
                        len++;
                    } else {
                        break;
                    }
                }

                if (len > 0) {
                    text[len] = '\0';
                    strncpy(info->text, text, sizeof(info->text) - 1);
                    info->text[sizeof(info->text) - 1] = '\0';
                    break;
                }
            }
        }
    }

    return info->text[0] != '\0';
}

// Najde index položky podle itemData
static int FindItemIndexByData(HWND listbox, DWORD_PTR itemData) {
    int count = SendMessage(listbox, LB_GETCOUNT, 0, 0);
    for (int i = 0; i < count; i++) {
        LRESULT data = SendMessage(listbox, LB_GETITEMDATA, i, 0);
        if (data != LB_ERR && (DWORD_PTR)data == itemData) {
            return i;
        }
    }
    return -1;
}

// Přepne složku (otevře/zavře)
static void ToggleFolder(HWND listbox, int index) {
    SetFocus(listbox);
    Sleep(5);
    SendMessage(listbox, LB_SETCURSEL, index, 0);
    Sleep(10);
    PostMessage(listbox, WM_KEYDOWN, VK_RETURN, 0);
    PostMessage(listbox, WM_KEYUP, VK_RETURN, 0);
}

// Zjistí, zda je položka expandovaná
static bool IsItemExpanded(HWND listbox, HANDLE hProcess, int index, int level) {
    int count = SendMessage(listbox, LB_GETCOUNT, 0, 0);
    if (index + 1 >= count) return false;

    ItemInfo nextInfo;
    if (!GetItemInfo(listbox, hProcess, index + 1, &nextInfo)) {
        return false;
    }

    return nextInfo.level > level;
}

// Přidá položku do cesty
static void AppendToPath(SearchContext* ctx, const char* name) {
    if (ctx->pathLen > 0 && ctx->pathLen < SEARCH_MAX_PATH - 3) {
        ctx->path[ctx->pathLen++] = '/';
    }
    int remaining = SEARCH_MAX_PATH - ctx->pathLen - 1;
    if (remaining > 0) {
        int len = strlen(name);
        if (len > remaining) len = remaining;
        memcpy(ctx->path + ctx->pathLen, name, len);
        ctx->pathLen += len;
        ctx->path[ctx->pathLen] = '\0';
    }
}

// Odebere položku z cesty
static void RemoveFromPath(SearchContext* ctx, int nameLen) {
    if (ctx->pathLen > nameLen) {
        ctx->pathLen -= nameLen;
        if (ctx->pathLen > 0 && ctx->path[ctx->pathLen - 1] == '/') {
            ctx->pathLen--;
        }
        ctx->path[ctx->pathLen] = '\0';
    }
}

// Rekurzivní DFS průchod stromem
static bool TraverseChildren(SearchContext* ctx, int startIndex, int parentLevel) {
    int index = startIndex;

    while (index < SendMessage(ctx->hListBox, LB_GETCOUNT, 0, 0)) {
        ItemInfo info;
        if (!GetItemInfo(ctx->hListBox, ctx->hProcess, index, &info)) {
            index++;
            continue;
        }

        // Jsme zpět na úrovni rodiče nebo výše - konec tohoto podstromu
        if (info.level <= parentLevel) {
            return false;
        }

        // Přidej do cesty
        int nameLen = strlen(info.text);
        AppendToPath(ctx, info.text);

        // Kontrola, zda jsme našli cíl
        if (strcmp(info.text, ctx->targetName) == 0) {
            ctx->found = true;
            // Aktivuj položku
            SetFocus(ctx->hListBox);
            SendMessage(ctx->hListBox, LB_SETCURSEL, index, 0);
            RemoveFromPath(ctx, nameLen); // Odeber z cesty před návratem
            return true; // Našli jsme, vracíme true
        }

        bool isFolder = info.hasChildren || info.flags == FLAG_FOLDER;

        if (isFolder) {
            bool expandedBefore = IsItemExpanded(ctx->hListBox, ctx->hProcess, index, info.level);

            // Otevři složku, pokud ještě není otevřená
            if (!expandedBefore) {
                ToggleFolder(ctx->hListBox, index);
                Sleep(80);
            }

            // Rekurzivně prohledej podstrom
            bool found = TraverseChildren(ctx, index + 1, info.level);

            if (found) {
                RemoveFromPath(ctx, nameLen);
                return true; // Nalezeno v podstromu, nechej otevřené
            } else {
                // Nenalezeno, zavři složku
                int refreshedIndex = FindItemIndexByData(ctx->hListBox, info.itemData);
                if (refreshedIndex != -1) {
                    ToggleFolder(ctx->hListBox, refreshedIndex);
                    Sleep(80);
                }
            }
        }

        RemoveFromPath(ctx, nameLen);
        index++;
    }

    return false;
}

// Zkontroluje a připraví kořenový uzel
static int EnsureRootPrepared(HWND listbox, HANDLE hProcess) {
    int count = SendMessage(listbox, LB_GETCOUNT, 0, 0);
    if (count <= 1) {
        return SEARCH_ROOT_ERROR;
    }

    ItemInfo rootInfo;
    if (!GetItemInfo(listbox, hProcess, 0, &rootInfo)) {
        return SEARCH_ROOT_ERROR;
    }

    if (!IsItemExpanded(listbox, hProcess, 0, rootInfo.level)) {
        return SEARCH_ROOT_ERROR;
    }

    return SEARCH_SUCCESS;
}

// Hlavní veřejná funkce
int TwinCatSearchAndActivate(
    HWND hListBox,
    const char* targetName,
    char* resultPath,
    int maxPathLen
) {
    // Validace vstupních parametrů
    if (!hListBox || !targetName || !targetName[0]) {
        return SEARCH_INVALID_PARAMS;
    }

    // Získání procesu
    DWORD processId;
    GetWindowThreadProcessId(hListBox, &processId);
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) {
        return SEARCH_PROCESS_ERROR;
    }

    // Příprava kontextu
    SearchContext ctx = {0};
    ctx.hListBox = hListBox;
    ctx.hProcess = hProcess;
    ctx.targetName = targetName;
    ctx.path[0] = '\0';
    ctx.pathLen = 0;
    ctx.found = false;

    // Kontrola kořene
    int rootCheck = EnsureRootPrepared(hListBox, hProcess);
    if (rootCheck != SEARCH_SUCCESS) {
        CloseHandle(hProcess);
        return rootCheck;
    }

    // Zahaj vyhledávání od prvního dítěte kořene (index 1)
    TraverseChildren(&ctx, 1, 0);

    // Zavři proces
    CloseHandle(hProcess);

    // Zkopíruj výslednou cestu, pokud je požadována
    if (resultPath && maxPathLen > 0) {
        if (ctx.found) {
            strncpy(resultPath, ctx.path, maxPathLen - 1);
            resultPath[maxPathLen - 1] = '\0';
        } else {
            resultPath[0] = '\0';
        }
    }

    return ctx.found ? SEARCH_SUCCESS : SEARCH_NOT_FOUND;
}

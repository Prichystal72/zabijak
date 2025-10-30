/**
 * Jednoduchý test keyboard hooku
 */

#include <windows.h>
#include <stdio.h>
#include <stdbool.h>

HHOOK g_Hook = NULL;

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT *kb = (KBDLLHOOKSTRUCT *)lParam;
        
        //printf("Key pressed: VK=0x%02X, flags=0x%08X\n", kb->vkCode, kb->flags);
        
        // Test: Ctrl+Shift+Z (bezpečnější než Alt)
        if (kb->vkCode == 'A') {
            // Použij GetKeyState místo GetAsyncKeyState pro spolehlivější detekci
            SHORT ctrlState = GetKeyState(VK_CONTROL);
            SHORT shiftState = GetKeyState(VK_SHIFT);
            
            bool ctrl = (ctrlState & 0x8000) != 0;
            bool shift = (shiftState & 0x8000) != 0;
            
            //printf("  Z pressed! Ctrl=%d (0x%04X), Shift=%d (0x%04X)\n", 
               //    ctrl, ctrlState, shift, shiftState);
            
            if (ctrl && shift) {
                printf("  >>> HOTKEY DETECTED! <<<\n");
                MessageBeep(MB_OK);
                // Otevri Win+R
                keybd_event(VK_LWIN, 0, 0, 0);
                keybd_event('R', 0, 0, 0);
                Sleep(50);
                keybd_event('R', 0, KEYEVENTF_KEYUP, 0);
                keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
                return 1;
            }
        }
    }
    
    return CallNextHookEx(g_Hook, nCode, wParam, lParam);
}

int main() {
    printf("=== TEST KEYBOARD HOOK ===\n");
    printf("Press Ctrl + Shift + Z to test hotkey\n");
    printf("Press ESC to exit\n\n");
    
    // Instalace hooku
    g_Hook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    
    if (!g_Hook) {
        printf("ERROR: SetWindowsHookEx failed! Error=%lu\n", GetLastError());
        return 1;
    }
    
    printf("Hook installed successfully! Handle=%p\n\n", g_Hook);
    
    // Message loop
    MSG msg;
    while (true) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            printf("\nESC pressed, exiting...\n");
            break;
        }
        
        Sleep(10);
    }
    
    UnhookWindowsHookEx(g_Hook);
    printf("Hook uninstalled\n");
    
    return 0;
}

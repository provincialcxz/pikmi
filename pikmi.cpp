#include <windows.h>
#include <string>
#include <vector>
#include <random>
#include <deque>
#include <algorithm>

extern "C" int WINAPI WinMain(HINSTANCE hInstance, 
                             HINSTANCE hPrevInstance, 
                             LPSTR lpCmdLine, 
                             int nCmdShow)
{
    return wWinMain(hInstance, hPrevInstance, (PWSTR)lpCmdLine, nCmdShow);
}

HHOOK hook = nullptr;
bool sending = false;

std::mt19937 rng(std::random_device{}());

const std::vector<std::wstring> emojis = {
    L" 🦄", L" 🌸", L" 🌟", L" 😊", L" 💞", L" 💢", L" 💫", L" 🌈", L" 😸"
};

std::deque<int> recentEmojis;

std::wstring randomSuffix() {
    std::vector<int> candidates;
    for (int i = 0; i < (int)emojis.size(); ++i) {
        if (std::find(recentEmojis.begin(), recentEmojis.end(), i) == recentEmojis.end()) {
            candidates.push_back(i);
        }
    }
    if (candidates.empty()) {
        for (int i = 0; i < (int)emojis.size(); ++i)
            candidates.push_back(i);
    }
    std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
    int chosen = candidates[dist(rng)];
    recentEmojis.push_back(chosen);
    if (recentEmojis.size() > 3)
        recentEmojis.pop_front();
    return emojis[chosen];
}

void sendText(const std::wstring& text) {
    if (text.empty()) return;
    std::vector<INPUT> inputs;
    inputs.reserve(text.size() * 2);
    for (wchar_t ch : text) {
        INPUT down = { INPUT_KEYBOARD };
        down.ki.wScan = ch;
        down.ki.dwFlags = KEYEVENTF_UNICODE;
        inputs.push_back(down);
        INPUT up = down;
        up.ki.dwFlags |= KEYEVENTF_KEYUP;
        inputs.push_back(up);
    }
    sending = true;
    SendInput((UINT)inputs.size(), inputs.data(), sizeof(INPUT));
    sending = false;
}

LRESULT CALLBACK keyboardProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION && !sending && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* k = (KBDLLHOOKSTRUCT*)lParam;
        BYTE state[256];
        if (!GetKeyboardState(state)) return CallNextHookEx(hook, code, wParam, lParam);
        wchar_t buf[5] = { 0 };
        int len = ToUnicodeEx(k->vkCode, k->scanCode, state, buf, 4, 0, GetKeyboardLayout(0));
        if (len < 1) return CallNextHookEx(hook, code, wParam, lParam);
        std::wstring current(buf, len);
        if (current == L" ") {
            sendText(randomSuffix());
        }
    }
    return CallNextHookEx(hook, code, wParam, lParam);
}

void AddToStartup() {
    HKEY hKey;
    const wchar_t* appName = L"pikmi";
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);

    if (RegOpenKeyExW(HKEY_CURRENT_USER, 
                     L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                     0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, appName, 0, REG_SZ, 
                      (const BYTE*)path, 
                      (wcslen(path) + 1) * sizeof(wchar_t));
        RegCloseKey(hKey);
    }
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    AddToStartup(); 
    hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardProc, nullptr, 0);
    if (!hook) return 1;
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnhookWindowsHookEx(hook);
    return 0;
}
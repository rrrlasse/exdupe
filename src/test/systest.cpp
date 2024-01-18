﻿
#pragma execution_character_set("utf-8")

#include <chrono>
#include <thread>
#include <array>
#include <iostream>
#include <format>
#include <map>
#include <regex>
#include <vector>
#include <filesystem>

#ifdef _WIN32
#include <iostream>
#include <Windows.h>
#include <shellapi.h>
#include <shlobj_core.h>
#else
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <wordexp.h>
#endif

#include "catch.hpp"

#include <iostream>
#include <locale>
#include <codecvt>
#include <string>

using namespace std;

namespace {

string p(string path);

#include <iostream>
#include <filesystem>

#ifdef _WIN32
string nul = "2>NUL";
bool win = true;
#define pclose _pclose
#define popen _popen
#else
bool win = false;
string nul = "2>/dev/null";
#endif

// Please customize
string root = win ? "e:\\exdupe" : "/mnt/hgfs/E/eXdupe"; // the dir that contains README.md
string work = win ? "e:\\exdupe\\tmp" : "~/out/tmp"; // tests will read and write here, it must support symlinks
string bin = win ? "e:\\exdupe\\exdupe.exe" : "~/out/exdupe";

// No need to edit
string tmp = p(work + "/tmp");
string in = p(tmp + "/in");
string out = p(tmp + "/out");
string full = p(tmp + "/full");
string diff = p(tmp + "/diff");
string testfiles = p(root + "/test/testfiles");
string diff_tool = win ? root + "test\\diffexe\\diff.exe" : "diff";

template<typename... Args> std::string conc(const Args&... args) {
    std::ostringstream oss;
    ((oss << args << ' '), ...);
    string cmd2 = oss.str();
    cmd2.pop_back();
    return cmd2;
}

template<typename... Args> std::string sys(const Args&... args) {
#ifdef _WIN32
    auto utf8w = [](std::string utf8str) {
        std::locale utf8_locale(std::locale(), new std::codecvt_utf8<wchar_t>);
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(utf8str);
    };
    std::array<char, 128> buffer;
    std::string result;
    string cmd2 = conc(args...);
    wstring cmd = utf8w(cmd2);
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_wpopen(wstring(cmd.begin(), cmd.end()).c_str(), L"r"), _pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
#else
    std::string result;
    char buffer[128];
    string cmd = conc(args...);
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        throw "Error executing command (at popen())";
    }
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    if (pclose(pipe) == -1) {
        throw "Error executing command (at pclose())";
    }
    return result;
#endif
}

string p(string path) {
    if(win) {
        std::ranges::replace(path, '/', '\\');
    }
    else {
        std::ranges::replace(path, '\\', '/');    
    }

#ifndef _WIN32
    // resolve ~
    wordexp_t expResult;
    wordexp(path.c_str(), &expResult, 0);
    std::filesystem::path resolvedPath(expResult.we_wordv[0]);
    wordfree(&expResult);
    path = resolvedPath.string();
#endif
    return path;
}

void rm(string path) {
    REQUIRE(path.find("tmp") != string::npos);
    path = p(path);
    if(win) {
        sys("rmdir /q/s", path, nul); // fs::is_link() doesn't work for broken link to directory
        sys("del", path, nul);
    }
    else {
        sys("rm -rf", path);
    }
}

void cp(string src, string dst) {
    sys(win ? "copy" : "cp", p(src), p(dst));
}

void clean() {
    rm(tmp);
    sys("mkdir", p(work), nul);
    sys("mkdir", p(tmp));
    sys("mkdir", p(in));
    sys("mkdir", p(out));
}

void md(string dir) {
    std::filesystem::create_directories(dir);
}

bool can_create_links() {
#ifdef _WIN32
    bool b = IsUserAnAdmin();
    if(!b) {
        cerr << "Cannot create symlink. Please run Visual Studio or Command Prompt as Administrator\n";
        CHECK(false);
    }
    return b;
#else
    return true;
#endif
}

void lf(string from, string to) {
    can_create_links();
    from = p(from);
    to = p(to);
    if(win) {
        sys("mklink", from, to);        
    }
    else {
        sys("ln -s", to, from);        
    }   
}

void ld(string from, string to) {
    can_create_links();
    from = p(from);
    to = p(to);
    if(win) {
        sys("mklink /D", from, to);        
    }
    else {
        sys("ln -s", to, from);        
    }
}

void pick(string file, string dir = "") {
    file = testfiles + "/" + file;
    std::filesystem::create_directories(p(in + "/" + dir));
    cp(file, in + "/" + dir);
}

template<typename... Args> void ex(const Args&... args) {
    sys(bin, conc(args...));
}

bool cmp_diff() {
    auto ret = sys(diff_tool, "--no-dereference -r", in, out, nul);
    return ret.empty();
}

// On Windows: Tells <SYMLINK> vs <SYMLINKD> and timestamp, which "diff" does not look at
bool cmp_ls() {
    string ls_in = sys(win ? "dir /s" : "ls -l", in);
    string ls_out = sys(win ? "dir /s" : "ls -l", out);

    if(win) {
        std::regex freeRegex(R"(.* Directory of.*)");
        std::regex filesRegex(R"(.* bytes free.*)");

        for(auto s : vector<string*>{&ls_in, &ls_out}) {
            *s = std::regex_replace(ls_in, freeRegex, "");
            *s = std::regex_replace(ls_in, filesRegex, "");
        }
    }

    CHECK(ls_in == ls_out);
    return ls_in == ls_out;
}

bool cmp() {
    if(win) {
        return cmp_diff() && cmp_ls();
    }
    else {
        return cmp_diff();    
    }
}

size_t siz(string file) {   
    return filesystem::file_size(file);
}

void all_types() {
    md(in + "/d"); // dir
    pick("a"); // file
    lf(in + "/link_to_a", in + "/a"); // link to file
    ld(in + "/link_to_d", in + "/d"); // link to dir
    lf(in + "/link_to_missing_file", in + "/missing_file"); // broken link to file  
    ld(in + "/link_to_missing_dir", in + "/missing_dir"); // broken link to dir
}
}

TEST_CASE("simple backup, diff backup and restore") {
    clean();
    pick("a");
    ex("-m1",in, full);
    ex("-R", full, out);
    cmp();

    ex("-D", in, full, diff);
    rm(out);
    ex("-RD", full, diff, out);
    cmp();
}

TEST_CASE("destination dirctory doesn't exist") {
    clean();
    pick("a");
    ex("-m1",in, full);
    rm(out);
    ex("-R", full, out);
    cmp();
    ex("-D", in, full, diff);
    rm(out);
    ex("-RD", full, diff, out);
    cmp();
}

TEST_CASE("unicode") {
    clean();
    
    // todo, test many more unicode sections
    SECTION("latin") {
        pick("æøåäöüßéèáéíóúüñ");
    }

    SECTION("hanja") {
        pick("운일암반계곡");
    }

    ex("-m1",in, full);
    ex("-R", full, out);
    cmp();
}

TEST_CASE("lua all or none") {
    clean();
    all_types();
    pick("æøåäöüßéèáéíóúüñ");

    SECTION("all") {
        ex("-m1 -u\"return true\"", in, full);
    }
    SECTION("none") {
        ex("-m1 -u\"return false\"", in, full);
    }

    ex("-R", full, out);
    cmp();
}

TEST_CASE("lua types") {
    clean();
    all_types();
    
    SECTION("add only dir") {
        rm(in);
        md(in);
        md(in + "/d");
        ex("-m1 -u\"return is_dir\"", in, full);
    }
    SECTION("add only file") {
        rm(in);
        md(in);
        pick("a");
        ex("-m1 -u\"return is_file\"", in, full);
    }

    // todo, links

    ex("-R", full, out);
    cmp();
}


TEST_CASE("duplicated data detcted within full backup") {
    clean();
    pick("high_entropy_a", "dir1"); // 65811 bytes
    pick("high_entropy_a", "dir2");
    ex("-m1x0",in, full);
    CHECK(((siz(full) > 65811) && siz(full) < 76000));
}

TEST_CASE("duplicated data detected between full backup and diff backup") {
    clean();
    pick("high_entropy_a", "dir1"); // 65811 bytes
    ex("-m1x0",in, full);
    pick("high_entropy_a", "dir2"); // 65811 bytes
    ex("-D", in, full, diff);
    CHECK(((siz(diff) > 100) && siz(diff) < 8000));
}

TEST_CASE("duplicated data detected within diff backup") {
    clean();
    pick("a");
    ex("-m1x0",in, full);
    pick("high_entropy_a", "dir1"); // 65811 bytes
    pick("high_entropy_a", "dir2");
    ex("-D", in, full, diff);
    CHECK(((siz(diff) > 65811) && siz(diff) < 76000));
}


TEST_CASE("symlink to dir") {
    clean();
    md(in + "/d");
    ld(in + "/link_to_d", in + "/d");
    ex("-m1",in, full);
    ex("-R", full, out);
    cmp();
}

TEST_CASE("broken symlink to dir") {
    clean();
    ld(in + "/link_to_missing_dir", in + "/missing_dir");
    ex("-m1",in, full);
    ex("-R", full, out);
    cmp();
}

TEST_CASE("symlink to file") {
    clean();
    pick("a");
    lf(in + "/link_to_a", in + "/a");
    ex("-m1",in, full);
    ex("-R", full, out);
    cmp();
}

TEST_CASE("broken symlink to file") {
    clean();
    lf(in + "/link_to_missing_file", in + "/missing_file");
    ex("-m1",in, full);
    ex("-R", full, out);
    cmp();
}

TEST_CASE("timestamps") {
    clean();
    all_types();
    ex("-m1", in, full);
    ex("-R", full, out);
    cmp();
}




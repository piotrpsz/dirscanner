#include <iostream>
#include <tbb/tbb.h>
#include <filesystem>
#include <string>
#include "share/share.h"
#include <fmt/core.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <atomic>
#include <vector>
#include <regex>
#include <cstddef>
#include <system_error>
#include <optional>
#include <variant>
#include "clap/clap.h"

namespace fs = std::filesystem;

tbb::task_group tg;
std::atomic_uint64_t total_dir_counter{};
std::atomic_uint64_t total_file_counter{};
std::atomic_uint64_t matched_dir_counter{};
std::atomic_uint64_t matched_file_counter{};

auto clap = Clap("dirscanner v. 0.1",
                 Arg()
                    .marker("-i")
                    .promarker("--icase")
                    .ordef(false),
                 Arg()
                    .marker("-r")
                    .promarker("--recursive"),
                 Arg()
                    .marker("-d")
                    .promarker("--dir"),
                 Arg()
                    .marker("-t")
                    .promarker("--text"),
                 Arg()
                    .marker("-n")
                    .promarker("--name")
);

bool is_readable(fs::file_status status) noexcept {
    try {
        auto perms = status.permissions();
        return ((perms & fs::perms::owner_read) != fs::perms::none) && ((perms & fs::perms::owner_exec) != fs::perms::none);
//            (perms & fs::perms::group_read) != fs::perms::none &&
//            (perms & fs::perms::others_read) != fs::perms::none;
    }
    catch (...) {
        return false;
    }
}

bool is_ok(std::string const& fpath) noexcept {
    if (access(fpath.c_str(), R_OK) == 0)
        return access(fpath.c_str(), X_OK) == 0;
    return false;
}

void parse_file(fs::path fp) {
    if (auto path = fp.lexically_normal().string(); !share::trim(path).empty()) {
        std::cout << fmt::format("{}\n", path) << std::flush;
//        std::cout << fmt::format("|{} | {}|\n", fp.filename().string(), fp.extension().string()) << std::flush;
    }
}

bool ends_with(std::string const& text, char c) {
    return *std::prev(std::end(text)) == c;
}

void iterate_dir(std::string const& dir, std::regex const& rgx) noexcept {
    total_dir_counter++;
    if (auto dirp = opendir(dir.c_str()); dirp) {
        auto* entry_prev = reinterpret_cast<struct dirent*>(malloc(offsetof(struct dirent, d_name) + NAME_MAX + 1));
        struct dirent* entry;

        for (;;) {
            if (readdir_r(dirp, entry_prev, &entry) != 0) {
                auto const err = std::make_error_code(std::errc{errno});
                fmt::print(stderr, "({}) {}\n", err.value(), err.message());
                break;
            }
            if (!entry) break;

            std::string name{entry->d_name};
            // no hidden, no . (current) and no .. (parent)
            if (name[0] == '.') continue;

            auto fp{dir};
            if (!ends_with(fp, '/')) fp.append("/");
            fp = fp.append(name);

            struct stat fstat{};
            if (0 == lstat(fp.c_str(), &fstat)) {
                if ((fstat.st_mode & S_IFREG) == S_IFREG) {
                    // Perform in a dedicated tbb-task
                    tg.run([fp, rgx] {
                        total_file_counter++;
                        std::smatch smatch;
                        if (std::regex_search(fp, smatch, rgx) && smatch[0].matched) {
                            matched_file_counter++;
                            fmt::print("{}\n", fp);
                        }
                    });
                }
                else if ((fstat.st_mode & S_IFDIR) == S_IFDIR) {
                    // Perform in a dedicated tbb-task
                    tg.run([fp, rgx] {
                        std::smatch smatch;
                        if (std::regex_search(fp, smatch, rgx) && smatch[0].matched) {
                            matched_dir_counter++;
                            fmt::print("{}\n", fp);
                        }
                        iterate_dir(fp, rgx);
                    });
                }
            }
        }
        closedir(dirp);
    }
}

int main(int argn, char* argv[]) {
    clap.parse(argn, argv);
//    std::cout << clap << '\n';
//    return 0;
/*
    std::string text = R"(
#include <vector>
namespace fs = std::filesystem;

tbb::task_group tg;
std::atomic_uint64_t dir_counter{};
std::atomic_uint64_t file_counter{};

bool is_readable(fs::file_status status) noexcept {}

int Main(char** argc, int argn) {}

avbrewer

)";
*/

//    auto expr = R"(\b(main)\S*(\(.*\))\S*)";
//    auto expr = R"(\b(.*)\b(main)\b(\(.*\)))";

//    auto expr = R"((brew))";
//    std::regex rgx(expr, std::regex_constants::ECMAScript | std::regex_constants::icase);
//    std::smatch smatch;
//    if (std::regex_search(text, smatch, rgx)) {
//        std::cout << "OK\n";
//        auto s0 = smatch[0];
//        auto pos = smatch.position();
//
//        std::cout << smatch[0] << "  => (" << pos << ")" << '\n';
//        std::cout << smatch[1] << '\n';
//        std::cout << smatch[2] << "  => (" << (std::distance(smatch[1].first, smatch[2].first)) << ")" <<'\n';
//    }
//    else {
//        std::cout << "nie znaleziono\n";
//    }
//
//    return 0;


//    fs::path fpath{"/Users/piotr"};
    fs::path fpath{"/"};
    auto expr = R"(\b(Brew)\b)";
    std::string name{};

    auto rgx_flags = std::regex_constants::ECMAScript;

    if (auto arg = clap["--icase"]; arg)
        if (auto flag = std::get_if<bool>(&arg->value()); flag)
            if (*flag)
                rgx_flags |= std::regex_constants::icase;

    if (auto arg = clap["--name"]; arg)
        if (auto value = std::get_if<std::string>(&arg->value()); value)
            name = *value;

    fmt::print("name: {}\n", name);

    std::regex rgx(expr, rgx_flags);

    tg.run([fpath, rgx] {
        iterate_dir(fpath, rgx);
    });

    auto dt = share::execution_timer([&] {
        tg.wait();
    }, 1);
    std::cout << dt << '\n';
    fmt::print("  dir total: {}, matched: {}\n", share::number2str(total_dir_counter.load()), share::number2str(matched_dir_counter.load()));
    fmt::print("files total: {}, matched: {}\n", share::number2str(total_file_counter.load()), share::number2str(matched_file_counter.load()));

    return 0;
}

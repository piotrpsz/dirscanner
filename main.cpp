// MIT License
//
// Copyright (c) 2023 Piotr Pszczółkowski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#include <tbb/tbb.h>
#include <string>
#include "share/share.h"
#include <fmt/core.h>
#include <dirent.h>
#include <sys/stat.h>
#include <atomic>
#include <regex>
#include <system_error>
#include <optional>
#include <variant>
#include <fstream>
#include <sstream>
#include "clap/clap.h"

namespace fs = std::filesystem;

tbb::task_group tg;
std::atomic_uint64_t total_dir_counter{};
std::atomic_uint64_t total_file_counter{};
std::atomic_uint64_t matched_dir_counter{};
std::atomic_uint64_t matched_file_counter{};
std::regex rgx_dir, rgx_file;
std::optional<std::regex> rgx_text{};

auto quiet{false};

auto clap = Clap("dirscanner v. 0.1",
                 Arg()
                    .marker("-i")
                    .promarker("--icase")
                    .help("regex with ignore case"),
                 Arg()
                    .marker("-r")
                    .promarker("--recursive")
                    .help("recursive scan"),
                 Arg()
                    .marker("-d")
                    .promarker("--dir"),
                 Arg()
                    .marker("-t")
                    .promarker("--text"),
                 Arg()
                    .marker("-n")
                    .promarker("--name")
                    .help("file/directory name"),
                 Arg()
                    .marker("-w")
                    .help("regex with word boundaries"),
                 Arg()
                    .marker("-e")
                    .promarker("--ext"),
                    Arg()
                    .marker("-q")
                    .promarker("--quiet")
);

bool parse_file(std::string const& fp) {
    std::ifstream f;
    f.open(fp);
    std::stringstream ss;
    ss << f.rdbuf();
    std::string str = ss.str();
    f.close();

    if (rgx_text) {
        auto const rgx = *rgx_text;
        std::smatch smatch;

        if (std::regex_search(str, smatch, rgx) && smatch[0].matched) {
            fmt::print("{}\n", fp);
            return true;
        }
    }
    return false;
}

bool ends_with(std::string const& text, char c) {
    return *std::prev(std::end(text)) == c;
}

void iterate_dir(std::string const& dir) noexcept {
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
                    tg.run([fp] {
                        total_file_counter++;
                        std::smatch smatch;
                        if (std::regex_search(fp, smatch, rgx_file) && smatch[0].matched) {
                            matched_file_counter++;
                            if (parse_file(fp))
                                return; // return from task
                            if (!quiet)
                                fmt::print("{}\n", fp);
                        }
                    });
                }
                else if ((fstat.st_mode & S_IFDIR) == S_IFDIR) {
                    // Perform in a dedicated tbb-task
                    tg.run([fp] {
                        std::smatch smatch;
                        if (std::regex_search(fp, smatch, rgx_dir) && smatch[0].matched) {
                            matched_dir_counter++;
                            if (!quiet)
                                fmt::print("{}\n", fp);
                        }
                        iterate_dir(fp);
                    });
                }
            }
        }
        closedir(dirp);
    }
}

int main(int argn, char* argv[]) {
    clap.parse(argn, argv);

    auto word_boundary{false};
    auto ignore_case{false};
    std::string name{};
    std::string extension{};
    std::string text{};
    std::string fpath{};

    // fetch directory
    if (auto arg = clap["--dir"]; arg)
        if (auto value = std::get_if<std::string>(&arg->value()); value) {
            auto const tmp = *value;
            fpath = tmp.substr(1, tmp.size() - 2);
        }

    // during operation, regex runs in ignore case mode
    if (auto arg = clap["--icase"]; arg)
        if (auto flag = std::get_if<bool>(&arg->value()); flag)
            ignore_case = *flag;

    // search for a file/directory that contains 'name' in its name
    if (auto arg = clap["--name"]; arg)
        if (auto value = std::get_if<std::string>(&arg->value()); value) {
            auto const tmp = *value;
            name = tmp.substr(1, tmp.size() - 2);
        }

    // regex searches for text at a word boundary
    if (auto arg = clap["-w"]; arg)
        if (auto value = std::get_if<bool>(&arg->value()); value)
            word_boundary = *value;

    // don't display directories and files
    if (auto arg = clap["--quiet"]; arg)
        if (auto value = std::get_if<bool>(&arg->value()); value)
            quiet = *value;

    // file extension
    if (auto arg = clap["--ext"]; arg)
        if (auto value = std::get_if<std::string>(&arg->value()); value) {
            auto const tmp = *value;
            extension = tmp.substr(1, tmp.size() - 2);
        }

    // text to search in file
    if (auto arg = clap["--text"]; arg)
        if (auto value = std::get_if<std::string>(&arg->value()); value) {
            auto tmp = *value;
            text = tmp.substr(1, tmp.size() - 2);
        }

    std::string express_dir{};
    if (!name.empty())
        express_dir = word_boundary ? fmt::format("\\b{}\\b", name) : name;
    auto express_file = express_dir;
    if (!extension.empty())
        express_file += fmt::format("\\w*\\.({})$", extension);

    fmt::print("------- settings --------------------------------\n");
    fmt::print("file/directory name provided: {}\n", name);
    fmt::print("              file extension: {}\n", extension);
    fmt::print("                        text: {}\n", text);
    fmt::print("                 ignore case: {}\n", ignore_case);
    fmt::print("             word boundaries: {}\n", word_boundary);
    fmt::print("                       quiet: {}\n", quiet);
    fmt::print("-------------------------------------------------\n");

    auto rgx_flags = std::regex_constants::ECMAScript;
    if (ignore_case)
        rgx_flags |= std::regex_constants::icase;

    rgx_dir = std::regex(express_dir, rgx_flags);
    rgx_file = std::regex(express_file, rgx_flags);
    if (!text.empty()) {
        auto pattern = fmt::format("\\b{}\\b", text);
//        fmt::print("{}\n", pattern);
        rgx_text = std::regex(pattern, rgx_flags);
    }

    tg.run([fpath] {
        iterate_dir(fpath);
    });

    auto dt = share::execution_timer([&] {
        tg.wait();
    }, 1);

    fmt::print("\nexecution time: {}\n", dt);
    fmt::print("     dir total: {}, matched: {}\n", share::number2str(total_dir_counter.load()), share::number2str(matched_dir_counter.load()));
    fmt::print("   files total: {}, matched: {}\n", share::number2str(total_file_counter.load()), share::number2str(matched_file_counter.load()));

    return 0;
}

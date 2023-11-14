#include <iostream>
#include <tbb/tbb.h>
#include <filesystem>
#include <string>
#include "share/share.h"
#include <fmt/core.h>
#include <unistd.h>
namespace fs = std::filesystem;

tbb::task_group tg;

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

void read_dir_content(fs::path fp) noexcept {
//    if (!is_readable(status(fp))) return;
    if (!is_ok(fp)) return;
    std::error_code ec;

  for (fs::directory_iterator pos{fp, ec}; pos != fs::directory_iterator{}; ++pos) {
//      try {
          auto e = pos->path();
          if (is_regular_file(e)) {
              tg.run([e] {
                  parse_file(e);
              });
          }
          else if (is_directory(e)) {
              if (auto const dir = fp / e; is_ok(dir) && dir.filename().string()[0] != '.')
                tg.run([dir] {
                    read_dir_content(dir);
                });
          }

//          ++pos;
//      }
//      catch (fs::filesystem_error const& err) {
////          std::cerr << err.what() << " - " << err.path1().string() << '\n';
//      }
//      catch (...) {}
  }

}

int main() {
    fs::path fpath{"/"};

    tg.run([fpath] {
        read_dir_content(fpath);
    });

    auto dt = share::execution_timer([&] {
        tg.wait();
    }, 1);
    std::cout << dt << '\n';

    return 0;
}

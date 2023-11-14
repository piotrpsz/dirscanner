#include <iostream>
#include <tbb/tbb.h>
#include <filesystem>
#include <string>
namespace fs = std::filesystem;

tbb::task_group tg;

bool is_readable(fs::file_status status) noexcept {
    std::error_code err{};
    auto perms = status.permissions();
    return  (perms & fs::perms::owner_read) != fs::perms::none &&
            (perms & fs::perms::group_read) != fs::perms::none &&
            (perms & fs::perms::others_read) != fs::perms::none;
}

void read_dir_content(int level, fs::path fpath) {
    std::string prefix(2 * level, ' ');

  for (fs::directory_iterator pos{fpath}; pos != fs::directory_iterator{}; ++pos) {
      try {
          auto e = pos->path();
          std::cout << prefix << e.lexically_normal().string();
          if (is_regular_file(e)) {
              std::cout << " ...file\n";
          } else if (is_directory(e)) {
              read_dir_content(level + 1, e);
          }
      }
      catch (fs::filesystem_error const& err) {
          std::cerr << err.what() << " - " << err.path1().string() << '\n';
      }
  }

}

int main() {
    fs::path fpath{"/"};

    tg.run([fpath] {
        read_dir_content(0, fpath);
    });
    tg.wait();

    return 0;
}

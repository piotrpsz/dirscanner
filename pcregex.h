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
//
// Software based on:
// https://www.pcre.org/current/doc/html/index.html

#pragma once
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <vector>

class Regex final {
    PCRE2_SPTR8 pattern_;
    PCRE2_SPTR8 subject_{};
    PCRE2_SIZE size_{};
    pcre2_code* re_;
    pcre2_match_data *match_data_;
public:
    struct item_t {
        uint32_t offset, size;
    };
public:
    Regex(char const* pattern);
    [[nodiscard]] std::vector<item_t> run(char const* subject, uint64_t n, bool find_all = false) noexcept;

private:
    std::vector<item_t> rest(PCRE2_SIZE *ovector) const noexcept;
    PCRE2_SIZE update_utf8_size(PCRE2_SIZE ovector) const noexcept;
};

#ifndef __UNICODE_H__
#define __UNICODE_H__

#include <cassert>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <stdexcept>
#include <regex>

// 辅助函数：检查 UTF-8 字节是否为续字节（10xxxxxx）
static inline bool is_utf8_continuation(uint8_t byte) {
    return (byte & 0b11000000) == 0b10000000;
}

// 辅助函数：将单个 Unicode 码点转为 \uhhhh 格式（支持代理对）
static inline std::string unicode_to_uhhhh(uint32_t codepoint) {
    std::stringstream ss;
    if (codepoint <= 0xFFFF) {
        // 普通码点：直接转为 4 位十六进制
        ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << std::uppercase << codepoint;
    } else if (codepoint <= 0x10FFFF) {
        // throw std::invalid_argument("Invalid Unicode codepoint (exceeds U+FFFF)");
        // 超出 BMP 码点：转为 UTF-16 代理对
        uint32_t surrogate = codepoint - 0x10000;
        uint16_t high = static_cast<uint16_t>(0xD800 + (surrogate >> 10));  // 高 10 位
        uint16_t low = static_cast<uint16_t>(0xDC00 + (surrogate & 0x3FF)); // 低 10 位
        ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << std::uppercase << high
           << "\\u" << std::hex << std::setw(4) << std::setfill('0') << std::uppercase << low;
    } else {
        throw std::invalid_argument("Invalid Unicode codepoint (exceeds U+10FFFF)");
    }

    return ss.str();
}


static inline size_t utf8_next(const char* data, size_t len=4) {
    assert(data);
    uint8_t first_byte = data[0];
    size_t i = 0;

    // 根据首字节类型跳过对应字节数（避免重复计数）
    if ((first_byte & 0b10000000) == 0) {
        // 1 字节字符：跳过 1 字节
        i += 1;
    } else if ((first_byte & 0b11100000) == 0b11000000) {
        // 2 字节字符：跳过 2 字节（需确保续字节有效，否则按错误处理）
        i += (i + 1 < len && is_utf8_continuation(data[i+1])) ? 2 : 1;
    } else if ((first_byte & 0b11110000) == 0b11100000) {
        // 3 字节字符：跳过 3 字节
        i += (i + 2 < len && is_utf8_continuation(data[i+1]) && is_utf8_continuation(data[i+2])) ? 3 : 1;
    } else if ((first_byte & 0b11111000) == 0b11110000) {
        // 4 字节字符：跳过 4 字节
        i += (i + 3 < len && is_utf8_continuation(data[i+1]) && is_utf8_continuation(data[i+2]) && is_utf8_continuation(data[i+3])) ? 4 : 1;
    } else {
        // 无效 UTF-8 字节：跳过该字节（容错处理）
        i += 1;
    }

    return i;
}

static inline std::pair<uint32_t, size_t> utf8_to_unicode(const char* data, size_t len=4) {
    uint32_t codepoint;
    uint8_t first_byte = *data;
    size_t i = 0;

    // 1 字节 UTF-8（U+0000~U+007F）
    if ((first_byte & 0b10000000) == 0) {
        codepoint = first_byte;
        i += 1;
        return {codepoint, i};
    }

    // 2 字节 UTF-8（U+0080~U+07FF）
    else if ((first_byte & 0b11100000) == 0b11000000) {
        if (i + 1 >= len || !is_utf8_continuation(data[i+1])) {
            throw std::runtime_error("Invalid 2-byte UTF-8 sequence");
        }
        codepoint = ((first_byte & 0b00011111) << 6) | (data[i+1] & 0b00111111);
        i += 2;
    }
    // 3 字节 UTF-8（U+0800~U+FFFF）
    else if ((first_byte & 0b11110000) == 0b11100000) {
        if (i + 2 >= len || !is_utf8_continuation(data[i+1]) || !is_utf8_continuation(data[i+2])) {
            throw std::runtime_error("Invalid 3-byte UTF-8 sequence");
        }
        codepoint = ((first_byte & 0b00001111) << 12) |
                    ((data[i+1] & 0b00111111) << 6) |
                    (data[i+2] & 0b00111111);
        i += 3;
    }
    // 4 字节 UTF-8（U+10000~U+10FFFF）
    else if ((first_byte & 0b11111000) == 0b11110000) {
        if (i + 3 >= len || !is_utf8_continuation(data[i+1]) || !is_utf8_continuation(data[i+2])
            || !is_utf8_continuation(data[i+3])) {
            throw std::runtime_error("Invalid 4-byte UTF-8 sequence");
        }
        codepoint = ((first_byte & 0b00000111) << 18) |
                    ((data[i+1] & 0b00111111) << 12) |
                    ((data[i+2] & 0b00111111) << 6) |
                    (data[i+3] & 0b00111111);
        i += 4;
    }
    // 无效 UTF-8 字节
    else {
        throw std::runtime_error("Invalid UTF-8 leading byte: " + std::to_string(first_byte));
    }

    return {codepoint, i};
}

// 核心函数：UTF-8 字符串转为 \uhhhh 格式
static inline std::string utf8_to_uhhhh(const std::string& utf8_str) {
    std::string result;
    const char* data = utf8_str.data();
    size_t len = utf8_str.size();
    size_t i = 0;

    while (i < len) {
        uint8_t first_byte = data[i];
        uint32_t codepoint = 0;

        // 1 字节 UTF-8（U+0000~U+007F）
        if ((first_byte & 0b10000000) == 0) {
            codepoint = first_byte;
            i += 1;
            result += static_cast<char>(first_byte);
            continue; // 1字节保持原样
        }

        auto [c, k] = utf8_to_unicode(data + i);
        codepoint = c;
        i += k;

        // 验证码点有效性（Unicode 允许的范围是 U+0000~U+10FFFF，排除代理项区域 U+D800~U+DFFF）
        if (codepoint >= 0xD800 && codepoint <= 0xDFFF) {
            throw std::runtime_error("Invalid Unicode codepoint (surrogate range)");
        }

        // 转为 \uhhhh 格式并追加到结果
        result += unicode_to_uhhhh(codepoint);
    }

    return result;
}

// 辅助函数：将单个十六进制字符转为数值（0~15）
inline int hex_char_to_val(char c) {
    if (isdigit(c)) return c - '0';
    if (isupper(c)) return 10 + (c - 'A');
    if (islower(c)) return 10 + (c - 'a');
    throw std::invalid_argument(std::string("Invalid hex character: ") + c);
}

// 辅助函数：解析 4 位十六进制字符串为 uint16_t（如 "4E16" → 0x4E16）
static inline uint16_t parse_4hex(const std::string& hex_str) {
    if (hex_str.size() != 4) {
        throw std::invalid_argument("Hex std::string must be 4 characters long");
    }
    uint16_t val = 0;
    for (char c : hex_str) {
        val = (val << 4) | hex_char_to_val(c);
    }
    return val;
}

// 辅助函数：Unicode 码点转为 UTF-8 字节序列
static inline std::string codepoint_to_utf8(uint32_t codepoint) {
    std::string utf8;
    if (codepoint <= 0x7F) {
        // 1 字节
        utf8.push_back(static_cast<uint8_t>(codepoint));
    } else if (codepoint <= 0x7FF) {
        // 2 字节
        utf8.push_back(static_cast<uint8_t>(0xC0 | (codepoint >> 6)));
        utf8.push_back(static_cast<uint8_t>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        // 3 字节
        utf8.push_back(static_cast<uint8_t>(0xE0 | (codepoint >> 12)));
        utf8.push_back(static_cast<uint8_t>(0x80 | ((codepoint >> 6) & 0x3F)));
        utf8.push_back(static_cast<uint8_t>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0x10FFFF) {
        // 4 字节
        utf8.push_back(static_cast<uint8_t>(0xF0 | (codepoint >> 18)));
        utf8.push_back(static_cast<uint8_t>(0x80 | ((codepoint >> 12) & 0x3F)));
        utf8.push_back(static_cast<uint8_t>(0x80 | ((codepoint >> 6) & 0x3F)));
        utf8.push_back(static_cast<uint8_t>(0x80 | (codepoint & 0x3F)));
    } else {
        throw std::invalid_argument("Invalid Unicode codepoint (exceeds U+10FFFF)");
    }
    return utf8;
}

// 核心函数：\uhhhh 格式字符串转为 UTF-8
static inline std::string uhhhh_to_utf8(const std::string& uhhhh_str) {
    std::string result;
    size_t len = uhhhh_str.size();
    size_t i = 0;

    while (i < len) {
        // 查找 \u 标记
        if (i + 1 < len && uhhhh_str[i] == '\\' && uhhhh_str[i+1] == 'u') {
            // 提取 \u 后的 4 位十六进制数
            if (i + 5 > len) {
                throw std::runtime_error("Incomplete \\uhhhh sequence (missing hex digits)");
            }
            std::string hex_part = uhhhh_str.substr(i + 2, 4);  // 从 i+2 开始取 4 个字符
            uint16_t code_unit = parse_4hex(hex_part);
            i += 6;  // 跳过 \u 和 4 位十六进制（共 6 个字符：\uXXXX）

            // 检查是否为高代理项（0xD800~0xDBFF），需读取后续低代理项
            if (code_unit >= 0xD800 && code_unit <= 0xDBFF) {
                // 必须紧跟另一个 \udyyy（低代理项 0xDC00~0xDFFF）
                if (i + 5 > len || uhhhh_str[i] != '\\' || uhhhh_str[i+1] != 'u') {
                    throw std::runtime_error("Missing low surrogate after high surrogate");
                }
                std::string low_hex = uhhhh_str.substr(i + 2, 4);
                uint16_t low_surrogate = parse_4hex(low_hex);
                i += 6;  // 跳过第二个 \uXXXX

                // 验证低代理项范围
                if (low_surrogate < 0xDC00 || low_surrogate > 0xDFFF) {
                    throw std::runtime_error("Invalid low surrogate (must be 0xDC00~0xDFFF)");
                }

                // 合并代理对为 Unicode 码点
                uint32_t codepoint = 0x10000 + ((code_unit - 0xD800) << 10) + (low_surrogate - 0xDC00);
                result += codepoint_to_utf8(codepoint);
            } else {
                // 普通码点（非代理项）：直接转为 UTF-8
                if (code_unit >= 0xDC00 && code_unit <= 0xDFFF) {
                    throw std::runtime_error("Invalid low surrogate without high surrogate");
                }
                result += codepoint_to_utf8(static_cast<uint32_t>(code_unit));
            }
        } else {
            // 非 \u 序列：直接保留原字符（如 ASCII 字符）
            result.push_back(uhhhh_str[i]);
            i += 1;
        }
    }

    return result;
}

// 核心函数：获取 UTF-8 字符串的字符数（Unicode 码点数量）
static inline size_t utf8_len(const std::string& utf8_str) {
    size_t char_count = 0;
    const char* data = utf8_str.data();
    size_t len = utf8_str.size();
    size_t i = 0;

    while (i < len) {
        uint8_t first_byte = *data;
        // 判断是否为字符首字节（非续字节）
        if (!is_utf8_continuation(first_byte)) {
            char_count++;
        }

        size_t k = utf8_next(data, len - i);
        data += k;
        i += k;
    }

    return char_count;
}

/**
 * Compare two uHHHH string: lhs, rhs
 * Return:
 *   0 if lhs == rhs
 *  -1 if lhs < rhs
 *   1 if lhs > rhs
 */
static inline int uhhhh_cmp(const std::string& lhs, const std::string& rhs) {
    assert(lhs.size() == rhs.size());
    for (size_t i = 0; i < lhs.size(); i++) {
        int a = std::tolower(lhs[i]);
        int b = std::tolower(rhs[i]);
        if (a < b) return -1;
        else if (a > b) return 1;
    }
    return 0;
}

static inline bool uhhhh_is_chinese(const std::string& u) {
    assert(u.size() == 6);
    return uhhhh_cmp("\\u4E00", u) <= 0 && uhhhh_cmp("\\u9FA5", u) >= 0;
}

// 判断 Unicode 编码是否为双宽度字符
static inline bool is_double_width_char(uint32_t unicode) {
    if (unicode < 0x1100) return false; // 基本拉丁字母及常用符号均为单宽度
    if (unicode >= 0x25C0 && unicode <= 0x25CF) return false; // 几何形状（部分双宽度）
    // 1. 全宽 ASCII 兼容区（U+FF01 ~ U+FF5E）
    if (unicode >= 0xFF01 && unicode <= 0xFF5E) return true;
    // 2. CJK 核心汉字区（U+4E00 ~ U+9FFF）
    if (unicode >= 0x4E00 && unicode <= 0x9FFF) return true;
    // 3. CJK 扩展 A 区（U+3400 ~ U+4DBF）
    if (unicode >= 0x3400 && unicode <= 0x4DBF) return true;
    // 4. 平假名（U+3040 ~ U+309F）+ 片假名（U+30A0 ~ U+30FF）+ 中文标点（U+3000 ~ U+303F）
    if (unicode >= 0x3000 && unicode <= 0x30FF) return true;
    // 5. 韩文（U+AC00 ~ U+D7AF + U+3130 ~ U+318F）
    if ((unicode >= 0xAC00 && unicode <= 0xD7AF) || (unicode >= 0x3130 && unicode <= 0x318F)) return true;
    // 6. CJK 扩展 B-F 区（极少使用，可选）
    if ((unicode >= 0x20000 && unicode <= 0x2EBEF)) return true;

    if (unicode >= 0x2f900 && unicode <= 0x2FA1F) return true; // CJK 扩展 G 区
    if (unicode >= 0xfd90 && unicode <= 0xfdef) return true;   // 私用区兼容汉字

    // 表意文字描述符	U+2FF0 ~ U+2FFF	CJK 字符扩展描述符
    // 中日韩兼容字符	U+F900 ~ U+FAFF	兼容旧字体的重复汉字（如 両 (U+F976)）
    // 注音符号	U+3100 ~ U+312F	中文注音（如 ㄅ (U+3105)）
    // 几何形状（全宽）	U+2580 ~ U+25FF	部分方块 / 图形字符（终端中双宽度）
    if ((unicode >= 0x2FF0 && unicode <= 0x2FFF)) return true;
    if ((unicode >= 0xF900 && unicode <= 0xFAFF)) return true;
    if ((unicode >= 0x3100 && unicode <= 0x312F)) return true;
    if ((unicode >= 0x2580 && unicode <= 0x25FF)) return true;
    return false;
}


static inline bool is_empty_width_char(uint32_t c) {
    // 匹配 ASCII 控制符（0-31）或 Unicode 零宽度格式符
    if (c <= 0x1F) return true;
    switch (c) {
        case 0x200B: // ZWSP
        case 0x200C: // ZWNJ
        case 0x200D: // ZWJ
        case 0x200E: // LRM
        case 0x200F: // RLM
        case 0xFEFF: // BOM/ZWNBSP
        case 0x2028: // LS
        case 0x2029: // PS
            return true;
        default: return false;
    }
}

static inline size_t ansi_size(const std::string& str) {
    if (str.empty()) return 0;
    // 正则表达式：匹配所有 ANSI 转义序列
    // \x1B：ESC 字符（\033）；\[：匹配 [；[0-9;]*：匹配数字/分号；[a-zA-Z]：匹配结束符（m/H/J/K 等）
    std::regex ansi_regex("\x1B\\[[0-9;]*[a-zA-Z]");
    
    std::sregex_iterator it(str.begin(), str.end(), ansi_regex);
    std::sregex_iterator end;
    
    size_t len = 0;
    for (; it != end; ++it) {
        // const smatch& match = *it;
        len += it->length();
    }
    return len;
}

static inline size_t visual_width(const std::string& utf8_str) {
    size_t char_count = 0;
    const char* data = utf8_str.data();
    size_t len = utf8_str.size();
    while (len > 0) {
        auto [ucode, k] = utf8_to_unicode(data, len);
        len -= k;
        data += k;
        if (is_double_width_char(ucode)) {
            char_count += 2;
        } else if (ucode == '\x1B' || !is_empty_width_char(ucode)) {
            char_count++;
        }
    }
    char_count -= ansi_size(utf8_str);
    return char_count;
}

static inline std::vector<std::pair<size_t,size_t>> utf8_split(const std::string& s) {
    std::vector<std::pair<size_t,size_t>> res;
    const char* data = s.data();
    size_t len = s.size();
    size_t i = 0;
    while (i < len) {
        size_t n = utf8_next(data, len-i);
        res.emplace_back(i, n);
        data += n;
        i += n;
    }
    return res;
}

#endif
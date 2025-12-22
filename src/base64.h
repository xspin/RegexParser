#ifndef BASE64_H
#define BASE64_H

#include <stdexcept>
#include <string>
#include "base64.h"

// Base64 编码表（标准）
static inline const char BASE64_TABLE[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// 获取 Base64 编码表对应字符（校验索引合法性）
static inline char base64_char(int index) {
    if (index < 0 || index >= 64) {
        throw std::invalid_argument("invalid Base64 index: " + std::to_string(index));
    }
    return BASE64_TABLE[index];
}

static inline int base64_index(char c) {
    // 遍历编码表找对应索引
    const char* pos = std::strchr(BASE64_TABLE, c);
    if (pos != nullptr) {
        return pos - BASE64_TABLE;
    }
    // 补位符 = 返回 -1（特殊处理）
    if (c == '=') {
        return -1;
    }
    // 无效字符抛出异常
    throw std::invalid_argument("无效的 Base64 字符：" + std::string(1, c));
}

// Base64 核心编码逻辑
static inline std::string base64_encode(const std::string& input) {
    std::string output;
    int input_len = input.size();
    int i = 0;

    // 按 3 字节分组处理
    while (i < input_len) {
        // 读取 3 个字节（不足则补 0）
        unsigned char byte1 = (i < input_len) ? input[i++] : 0;
        unsigned char byte2 = (i < input_len) ? input[i++] : 0;
        unsigned char byte3 = (i < input_len) ? input[i++] : 0;

        // 位运算拆分 24 位为 4 个 6 位
        // 第一个 6 位：byte1 的前 6 位
        int idx1 = (byte1 >> 2) & 0x3F; // 0x3F = 63 = 0b111111
        // 第二个 6 位：byte1 的后 2 位 + byte2 的前 4 位
        int idx2 = ((byte1 & 0x03) << 4) | ((byte2 >> 4) & 0x0F);
        // 第三个 6 位：byte2 的后 4 位 + byte3 的前 2 位
        int idx3 = ((byte2 & 0x0F) << 2) | ((byte3 >> 6) & 0x03);
        // 第四个 6 位：byte3 的后 6 位
        int idx4 = byte3 & 0x3F;

        // 转换为 Base64 字符
        output += base64_char(idx1);
        output += base64_char(idx2);

        // 处理补位（不足 3 字节时用 = 替代）
        if (i - 3 < input_len - 2) { // 缺少 1 个字节（仅 byte1、byte2 有效）
            output += base64_char(idx3);
        } else {
            output += '='; // 补位符
        }

        if (i - 3 < input_len - 1) { // 缺少 2 个字节（仅 byte1 有效）
            output += base64_char(idx4);
        } else {
            output += '='; // 补位符
        }
    }

    return output;
}
// Base64 核心解码逻辑
static inline std::string base64_decode(const std::string& input) {
    std::string output;
    std::string filtered_input;

    // 第一步：过滤无效字符（仅保留 Base64 字符和 =）
    for (char c : input) {
        if (std::isalnum(c) || c == '+' || c == '/' || c == '=') {
            filtered_input += c;
        }
    }

    int input_len = filtered_input.size();
    // 校验长度是否为 4 的倍数（Base64 规范）
    if (input_len % 4 != 0) {
        throw std::invalid_argument("invalid Base64 length");
    }

    int i = 0;
    // 预分配空间，减少内存重分配
    output.reserve((input_len / 4) * 3);

    // 第二步：按 4 字符分组解码
    while (i < input_len) {
        // 读取 4 个 Base64 字符，转换为索引
        int idx1 = base64_index(filtered_input[i++]);
        int idx2 = base64_index(filtered_input[i++]);
        int idx3 = base64_index(filtered_input[i++]);
        int idx4 = base64_index(filtered_input[i++]);

        // 处理补位（= 对应的索引为 -1，替换为 0）
        if (idx3 == -1) idx3 = 0;
        if (idx4 == -1) idx4 = 0;

        // 位运算拼接 4 个 6 位为 24 位，拆分为 3 个字节
        unsigned char byte1 = (idx1 << 2) | (idx2 >> 4); // 前 8 位
        unsigned char byte2 = ((idx2 & 0x0F) << 4) | (idx3 >> 2); // 中间 8 位
        unsigned char byte3 = ((idx3 & 0x03) << 6) | idx4; // 后 8 位

        // 添加到输出（根据补位数量决定添加多少字节）
        output += byte1;
        // 有第 3 个有效字符（无 = 或仅最后 1 个 =）→ 添加第二个字节
        if (filtered_input[i-2] != '=') {
            output += byte2;
        }
        // 有第 4 个有效字符（无 =）→ 添加第三个字节
        if (filtered_input[i-1] != '=') {
            output += byte3;
        }
    }

    return output;
}

#endif // BASE64_H
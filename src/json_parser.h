#pragma once

#include <cstdint>
#include <vector>

enum TokenType
{
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_ARRAY,
    TOKEN_TYPE_OBJECT,
    TOKEN_TYPE_PRIMITIVE
};

struct Token
{
    TokenType type;
    uint32_t start;
    uint32_t end;
    uint32_t size;
};

bool parse_json(const char *buffer, const uint32_t length, std::vector<Token> &tokens);

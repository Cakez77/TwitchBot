#include "json_parser.h"
#include "logger.h"

static void parse_string(const char *buffer, const uint32_t length, uint32_t &pos, Token &t)
{
    t.start = pos;

    for (; pos < length && buffer[pos] != '\0'; pos++)
    {
        if (buffer[pos - 1] != '\\' && buffer[pos] == '\"')
            break;
    }

    t.end = pos;
}

static void parse_primitive(const char *buffer, const uint32_t length, uint32_t &pos, Token &t)
{
    for (; pos < length && buffer[pos] != '\0'; pos++)
    {
        char c = buffer[pos];

        switch (c)
        {
        case '\n':
        case '\r':
        case '\t':
        case ']':
        case '}':
        case ',':
        case ' ':
        {
            t.end = pos;
            pos--; // closing ']', '}' have to be parsed again by the main loop
            return;
            break;
        }
        }
    }
}

bool parse_json(const char *buffer, const uint32_t length, std::vector<Token> &tokens)
{
    uint32_t pos = 0;
    std::vector<uint32_t> parentStack;
    parentStack.reserve(10);

    for (; pos < length && buffer[pos] != '\0'; pos++)
    {
        char c = buffer[pos];

        switch (c)
        {
        case '\"': // String start
        {
            Token t = {TOKEN_TYPE_STRING};
            pos++; // Skip starting \"
            parse_string(buffer, length, pos, t);
            tokens.push_back(t);
            tokens[parentStack.back()].size++;
            break;
        }
        case '{': // Object start
        case '[': // Array start
        {
            if (parentStack.size() > 0)
                tokens[parentStack.back()].size++;

            Token t = {c == '{' ? TOKEN_TYPE_OBJECT : TOKEN_TYPE_ARRAY};
            t.start = pos;
            parentStack.push_back(tokens.size());
            tokens.push_back(t);
            break;
        }
        case '}':
        case ']':
        {
            if (tokens[parentStack.back()].type != (c == '}' ? TOKEN_TYPE_OBJECT : TOKEN_TYPE_ARRAY))
            {
                CAKEZ_WARN("Closing Identifier: %s does not match opening Identifier", c);
                return false;
            }

            tokens[parentStack.back()].end = pos;

            parentStack.pop_back();
        }
        case ' ': // Ignore
        case '\t':
        case '\r':
        case '\n':
        case ':':
        case ',':
        {
            break;
        }
        default: // Everything else is a primitive
            Token t = {TOKEN_TYPE_PRIMITIVE};
            t.start = pos;
            parse_primitive(buffer, length, pos, t);
            tokens.push_back(t);
            tokens[parentStack.back()].size++;
        }
    }

    if (parentStack.size() != 0)
    {
        CAKEZ_ASSERT(0, "Still some parents open without closing Identifiers");
        return true;
    }
    return true;
}
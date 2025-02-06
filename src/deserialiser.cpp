#include "deserialiser.h"

#include "debug.h"

using namespace std;

vector<PTDeserialiser::Token> PTDeserialiser::tokenise(const string& content)
{
    size_t offset = 0;
    vector<Token> tokens;
    if (content.length() == 0) return tokens;

    string current_token = "";
    TokenType current_type = getType(content[0]);
    size_t start_offset = 0;
    if (current_type != TokenType::TEXT && current_type != TokenType::COMMENT && current_type != TokenType::WHITESPACE && current_type != TokenType::NEWLINE)
        reportError("invalid first token", offset, content);

    current_type = TokenType::WHITESPACE;

    while (offset < content.length())
    {
        char cur = content[offset];
        TokenType next_type = getType(cur);

        if (next_type == INVALID)
            reportError("illegal character", offset, content);

        TokenType new_type = current_type;
        bool append_cur = true;
        bool reset_token = false;
        Token finished_token = Token(current_type);
        finished_token.start_offset = start_offset;

        switch (current_type)
        {
        case TEXT:
            if (next_type == TEXT || next_type == INT)
                break;
            else if (!isSeparator(next_type))
                reportError("invalid conjoined tokens", offset, content);

            finished_token.s_value = current_token;
            reset_token = true;
            break;
        case STRING:
            if (next_type == STRING)
            {
                append_cur = false;
                finished_token.s_value = current_token.substr(1);
                reset_token = true;
                next_type = INVALID;
                break;
            }
            else
                break;
        case INT:
            if (next_type == INT)
                break;
            else if (next_type == FLOAT)
            {
                new_type = FLOAT;
                break;
            }
            else if (next_type == TEXT)
            {
                new_type = TEXT;
                break;
            }
            else if (!isSeparator(next_type))
                reportError("invalid conjoined tokens", offset, content);

            finished_token.i_value = stoi(current_token);
            reset_token = true;
            break;
        case FLOAT:
            if (next_type == INT)
                break;
            else if (next_type == FLOAT)
                reportError("invalid float literal", offset, content);
            else if (!isSeparator(next_type))
                reportError("invalid conjoined tokens", offset, content);

            finished_token.f_value = stof(current_token);
            reset_token = true;
            break;
        case TAG:
            if (next_type == TEXT || next_type == INT)
                break;
            else if (!isSeparator(next_type))
                reportError("invalid conjoined tokens", offset, content);

            finished_token.s_value = current_token;
            reset_token = true;
            break;
        case VECTOR4:
            if (next_type == VECTOR4)
            {
                // TODO: handle the three vector types
                //finished_token.c_value = ...
                append_cur = false;
                reset_token = true;
                new_type = WHITESPACE;
                break;
            }
            if (next_type == VECTOR2)
                reportError("invalid nested vector token", offset, content);
            else
                break;
        case COMMENT:
            if (next_type != COMMENT && current_token.size() < 2)
                reportError("incomplete comment initiator", offset, content);
            else if (next_type != NEWLINE)
                break;
            
            finished_token.s_value = current_token;
            reset_token = true;
            break;
        case WHITESPACE:
            if (next_type == WHITESPACE)
                break;
            
            reset_token = true;
            break;
        case INVALID:
            if (!isSeparator(next_type))
                reportError("invalid conjoined tokens", offset, content);
            
            reset_token = true;
            break;
        default:
            reset_token = true;
            break;
        }

        if (reset_token)
        {
            if (current_type != INVALID)
                tokens.push_back(finished_token);
            current_token = "";
            start_offset = offset;
            new_type = next_type;
        }
        current_type = new_type;

        if (append_cur)
            current_token.push_back(cur);

        offset++;
    }

    return tokens;
}

inline PTDeserialiser::TokenType PTDeserialiser::getType(const char c)
{
    if (isAlphabetic(c) || c == '_') return TEXT;
    if (c == '"') return STRING;
    if (c == '-' || (c >= '0' && c <= '9')) return INT;
    if (c == '.') return FLOAT;
    if (c == '@') return TAG;
    if (c == '[') return VECTOR2;
    if (c == ']') return VECTOR4;
    if (c == '(') return OPEN_ROUND;
    if (c == ')') return CLOSE_ROUND;
    if (c == '{') return OPEN_CURLY;
    if (c == '}') return CLOSE_CURLY;
    if (c == '\n') return NEWLINE;
    if (c == ':') return COLON;
    if (c == '=') return EQUALS;
    if (c == ',') return COMMA;
    if (c == '/') return COMMENT;
    if (c == ' ' || c == '\t') return WHITESPACE;
    return INVALID;
}

void PTDeserialiser::reportError(const string err, size_t off, const string& str)
{
    int32_t extract_start = max(0, (int32_t)off - 16);
    int32_t extract_end = extract_start + 32;
    while (true)
    {
        size_t find = str.find('\n', extract_start);
        if (find >= off) break;
        extract_start = find + 1;
    }
    size_t find = str.find('\n', off);
    if (find != string::npos)
    {
        if ((int32_t)find < extract_end)
            extract_end = find;
    }
    string extract = str.substr(extract_start, extract_end - extract_start);

    size_t ln = 0;
    size_t last = 0;
    size_t next = 0;
    while (next < off)
    {
        ln++;
        last = next;
        next = str.find('\n', next + 1);
    }
    size_t col = off - last;
    if (ln > 0) col--;
    if (col > 0 && ln > 0) ln--;

    string error = "parsing error:\n\t" + err
        + "\n\tat character " + to_string(off) + " (ln " + to_string(ln + 1) + ", col " + to_string(col + 1) + ")"
        + "\n\t-> '..." + extract + "...'"\
        + "\n\t" + string(7 + ((int32_t)off - extract_start), ' ') + "^"\
        + "\n\tterminating.";

    debugLog(error);

    throw runtime_error(error);
}
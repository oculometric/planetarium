#include "deserialiser.h"

#include "debug.h"

using namespace std;

vector<PTDeserialiser::Token> PTDeserialiser::tokenise(const string& content)
{
    // TODO: make a flow diagram for this!
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
                finished_token.s_value = current_token;
                reset_token = true;
                next_type = INVALID; // TODO: if a non-separator character is encountered after INVALID, then error
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
            else if (!isSeparator(next_type))
                reportError("invalid conjoined tokens", offset, content);

            finished_token.i_value = itos(current_token);
            reset_token = true;
            break;
        case FLOAT:
            if (next_type == INT)
                break;
            else if (next_type == FLOAT)
                reportError("invalid float literal", offset, content);
            else if (!isSeparator(next_type))
                reportError("invalid conjoined tokens", offset, content);

            finished_token.f_value = itof(current_token);
            reset_token = true;
            break;
        case TAG:
            // TODO: here...
        }

        if (reset_token)
        {
            tokens.push_back(finished_token);
            current_token = "";
            start_offset = offset;
            new_type = next_type;
        }
        current_type = new_type;

        if (append_cur)
            current_token.push_back(cur);

        //if (current_type == TokenType::STRING)
        //{
        //    if (new_type == TokenType::STRING)
        //    {
        //        Token finished_token = Token(current_type);
        //        finished_token.start_offset = start_offset;
        //        finished_token.s_value = current_token;
        //        tokens.push_back(finished_token);

        //        start_offset = ++offset;
        //        current_token = "";
        //        current_type = TokenType::WHITESPACE;
        //        continue;
        //    }

        //    current_token += cur;
        //}
        //else if (current_type == TokenType::COMMENT)
        //{
        //    if (new_type != TokenType::COMMENT && current_token.length() < 2)
        //        reportError("incomplete comment initiator", offset, content);
        //    else if (new_type == TokenType::NEWLINE)
        //    {
        //        Token finished_token = Token(TokenType::COMMENT);
        //        finished_token.start_offset = start_offset;
        //        finished_token.s_value = current_token.substr(2);
        //        tokens.push_back(finished_token);

        //        Token extra_token = Token(new_type);
        //        extra_token.start_offset = offset;
        //        tokens.push_back(extra_token);

        //        current_token = "";

        //        start_offset = offset + 1;
        //        current_type = TokenType::WHITESPACE;
        //    }
        //    else
        //    {
        //        current_token += cur;
        //    }
        //}
        //else if (current_type == TokenType::VECTOR4)
        //{
        //    size_t comma;
        //    switch (new_type)
        //    {
        //    case INT:
        //    case COMMA:
        //        current_token += cur;
        //        break;
        //    case FLOAT:
        //        reportError("coordinates may not be floating-point", offset, content);
        //        break;
        //    case COMMENT:
        //        reportError("incomplete coordinate token", offset, content);
        //        break;
        //    case WHITESPACE:
        //        break;
        //    case VECTOR4:
        //        comma = current_token.find(',');
        //        if (comma == string::npos)
        //            reportError("incomplete coordinate token", offset, content);
        //        else
        //        {
        //            // TODO: split count the number of parts, make an appropriate token (v2/v3/v4)
        //            string cx = current_token.substr(0, comma);
        //            string cy = current_token.substr(comma + 1);
        //            try
        //            {
        //                int co_x = stoi(cx);
        //                int co_y = stoi(cy);

        //                Token finished_token = Token(TokenType::COORDINATE);
        //                finished_token.start_offset = start_offset;
        //                finished_token.c_value = Coordinate{ co_x, co_y };
        //                tokens.push_back(finished_token);

        //                current_token = "";
        //                current_type = TokenType::WHITESPACE;
        //            }
        //            catch (const exception& _)
        //            {
        //                reportError("invalid integer token within coordinate token", start_offset, content);
        //            }
        //        }
        //        break;
        //    default:
        //        reportError("invalid token within coordinate token", offset, content);
        //        break;
        //    }
        //}
        //else if (new_type != current_type)
        //{
        //    if ((current_type == TokenType::FLOAT && new_type == TokenType::INT) || (current_type == TokenType::INT && new_type == TokenType::FLOAT))
        //    {
        //        current_type = TokenType::FLOAT;
        //        current_token += cur;

        //        offset++;
        //        continue;
        //    }

        //    if (current_type != TokenType::WHITESPACE)
        //    {
        //        Token finished_token = Token(current_type);
        //        finished_token.start_offset = start_offset;
        //        switch (current_type)
        //        {
        //        case TEXT:
        //            if (new_type == TokenType::INT || new_type == TokenType::FLOAT || new_type == TokenType::STRING || new_type == TokenType::COORDINATE)
        //                reportError("invalid conjoined token", offset, content);
        //            finished_token.s_value = current_token;
        //            break;
        //        case INT:
        //            if (new_type == TokenType::TEXT || new_type == TokenType::STRING || new_type == TokenType::COORDINATE)
        //                reportError("invalid conjoined token", offset, content);
        //            try
        //            {
        //                finished_token.i_value = stoi(current_token);
        //            }
        //            catch (const exception& _)
        //            {
        //                reportError("invalid int token", start_offset, content);
        //            }
        //            break;
        //        case FLOAT:
        //            if (new_type == TokenType::TEXT || new_type == TokenType::STRING || new_type == TokenType::COORDINATE)
        //                reportError("invalid conjoined token", offset, content);
        //            try
        //            {
        //                finished_token.f_value = stof(current_token);
        //            }
        //            catch (const exception& _)
        //            {
        //                reportError("invalid float token", start_offset, content);
        //            }
        //            break;
        //        default:
        //            reportError("invalid tokeniser state", offset, content);
        //        }
        //        tokens.push_back(finished_token);
        //    }

        //    start_offset = offset;
        //    current_token = "";
        //    if (!(new_type == TokenType::STRING || new_type == TokenType::COORDINATE || new_type == TokenType::WHITESPACE)) current_token += cur;

        //    Token extra_token = Token(new_type);
        //    switch (new_type)
        //    {
        //    case OPEN_ROUND:
        //    case CLOSE_ROUND:
        //    case OPEN_CURLY:
        //    case CLOSE_CURLY:
        //    case EQUALS:
        //    case COLON:
        //    case COMMA:
        //    case NEWLINE:
        //        extra_token.start_offset = offset;
        //        tokens.push_back(extra_token);

        //        start_offset = offset + 1;
        //        current_type = TokenType::WHITESPACE;
        //        break;
        //    default:
        //        current_type = new_type;
        //        break;
        //    }
        //}
        //else if (current_type != TokenType::WHITESPACE)
        //{
        //    current_token += cur;
        //}

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
    if (c == '[' || c == ']') return VECTOR4;
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
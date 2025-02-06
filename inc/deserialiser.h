#pragma once

#include <vector>
#include <string>
#include <map>

#include "vector4.h"
#include "vector3.h"
#include "vector2.h"

class PTResource;
class PTObject;
class PTScene;

class PTDeserialiser
{
public:
    enum TokenType
    {
        STRING,
        INT,
        FLOAT,
        VECTOR2,
        VECTOR3,
        VECTOR4,
        TAG,
        TEXT,
        OPEN_ROUND,
        CLOSE_ROUND,
        OPEN_CURLY,
        CLOSE_CURLY,
        COLON,
        COMMA,
        SEMICOLON,
        EQUALS,
        NEWLINE,
        COMMENT,
        WHITESPACE,
        INVALID
    };

    struct Token
    {
        TokenType type;
        std::string s_value = "";
        union
        {
            int i_value = 0;
            float f_value;
            PTVector4f c_value;
        };
        size_t start_offset = 0;

        inline Token(TokenType ttype)
        {
            type = ttype;
            start_offset = 0;

            switch (ttype)
            {
            case TEXT:
            case STRING:
            case COMMENT:
                s_value = "";
                break;
            case VECTOR2:
            case VECTOR3:
            case VECTOR4:
                c_value = PTVector4f{ 0, 0, 0, 0 };
                break;
            case INT:
                i_value = 0;
                break;
            case FLOAT:
                f_value = 0.0f;
                break;
            default:
                break;
            }
        }

        inline Token(const Token& other)
        {
            type = other.type;
            start_offset = other.start_offset;
            s_value = other.s_value;

            switch (type)
            {
            case VECTOR2:
            case VECTOR3:
            case VECTOR4:
                c_value = other.c_value;
                break;
            case INT:
                i_value = other.i_value;
                break;
            case FLOAT:
                f_value = other.f_value;
                break;
            default:
                break;
            }
        }

        inline Token operator=(const Token& other)
        {
            type = other.type;
            start_offset = other.start_offset;
            s_value = other.s_value;

            switch (type)
            {
            case VECTOR2:
            case VECTOR3:
            case VECTOR4:
                c_value = other.c_value;
                break;
            case INT:
                i_value = other.i_value;
                break;
            case FLOAT:
                f_value = other.f_value;
                break;
            default:
                break;
            }

            return *this;
        }

        inline ~Token()
        {
        }
    };

    enum ArgType
    {
        STRING_ARG,
        INT_ARG,
        FLOAT_ARG,
        VECTOR2_ARG,
        VECTOR3_ARG,
        VECTOR4_ARG,
        RESOURCE_ARG,
        ARRAY_ARG
    };

    struct Argument
    {
        ArgType type;

        std::string s_val;
        std::vector<Argument> a_val;
        union
        {
            int i_val;
            float f_val;
            PTVector2f v2_val;
            PTVector3f v3_val;
            PTVector4f v4_val = PTVector4f(0,0,0,0);
            PTResource* r_val;
        };

        inline Argument& operator=(const Argument& other)
        {
            type = other.type;
            s_val = other.s_val;
            f_val = other.f_val;
            v4_val = other.v4_val;

            return *this;
        }

        inline ~Argument()
        {
        }
    };

public:
	static std::vector<Token> tokenise(const std::string& content);
    static std::vector<Token> prune(const std::vector<Token>& tokens);
    // TODO: supply the scene instead of the map of resources
    static std::pair<std::string, PTResource*> deserialiseResourceDescriptor(const std::vector<Token>& tokens, size_t& first_token, const std::map<std::string, PTResource*>& resources, const std::string& content);
    static PTObject* deserialiseObject(const std::vector<Token>& tokens, size_t& first_token, const std::map<std::string, PTResource*>& resources, const std::string& content);
    static PTScene* deserialiseScene(const std::vector<Token>& tokens, size_t& first_token, const std::string& content);

private:
    static inline TokenType getType(const char c);

    static inline bool isAlphabetic(const char c);

    static inline bool isSeparator(TokenType t);

    static void reportError(const std::string err, size_t off, const std::string& str);

    static size_t findClosingBracket(const std::vector<Token>& tokens, size_t open_index, bool allow_semicolons, const std::string& content);

    static Argument compileArgument(const std::vector<Token>& tokens, const std::map<std::string, PTResource*>& resources, const std::string& content);
    static std::pair<std::string, Argument> compileNamedArgument(const std::vector<Token>& tokens, const std::map<std::string, PTResource*>& resources, const std::string& content);

    static TokenType decodeVectorToken(const std::string token, PTVector4f& vector_out, size_t offset, const std::string& content);
};
#pragma once

#include <vector>
#include <string>

#include "vector4.h"

/* example scene file:

Resource(mesh, "suzanne.obj") : 4a3b825f

// comment

Node() : parent
{
	Mesh(data = @4a3b825f, position = [0.5, 1.0, 0.0]) : mesh
	DirectionalLight() : sun_lamp
}

*/

class PTDeserialiser
{
public:
    enum TokenType
    {
        TEXT,
        STRING,
        INT,
        FLOAT,
        TAG,
        VECTOR2,
        VECTOR3,
        VECTOR4,
        OPEN_ROUND,
        CLOSE_ROUND,
        OPEN_CURLY,
        CLOSE_CURLY,
        COLON,
        COMMA,
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

            switch (type)
            {
            case TEXT:
            case STRING:
            case COMMENT:
                s_value = other.s_value;
                break;
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

            switch (type)
            {
            case TEXT:
            case STRING:
            case COMMENT:
                s_value = other.s_value;
                break;
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

public:
	static std::vector<Token> tokenise(const std::string& content);

private:
    static inline TokenType getType(const char c);

    static inline bool isAlphabetic(const char c)
    {
        if (c >= 'a' && c <= 'z') return true;
        if (c >= 'A' && c <= 'Z') return true;

        return false;
    }

    static inline bool isSeparator(TokenType t)
    {
        switch (t)
        {
        case TEXT:
        case STRING:
        case INT:
        case TAG:
        case VECTOR2:
        case VECTOR3:
        case VECTOR4:
            return false;
        default:
            return true;
        }
    }

    static void reportError(const std::string err, size_t off, const std::string& str);
};
#include "deserialiser.h"

#include "debug.h"
#include "resource_manager.h"
#include "image.h"
#include "scene.h"

using namespace std;

template <typename T>
PTNode* instantiateNode(PTScene* scene, string name, PTDeserialiser::ArgMap args)
{
    return (PTNode*)(scene->instantiate<T>(name, args));
}

typedef PTNode*(*PTNodeInstantiateFunc)(PTScene*, string, PTDeserialiser::ArgMap);

#define INSTANTIATE_FUNC(type) pair<string, PTNodeInstantiateFunc>(#type, instantiateNode<PT##type>)

#include "node_list.generated.h"

vector<PTDeserialiser::Token> PTDeserialiser::tokenise(const string& content)
{
    string copy_content = content + "\n";
    size_t offset = 0;
    vector<Token> tokens;
    if (content.length() == 0) return tokens;

    string current_token = "";
    TokenType current_type = getType(copy_content[0]);
    size_t start_offset = 0;
    if (current_type != TokenType::TEXT && current_type != TokenType::COMMENT && current_type != TokenType::WHITESPACE && current_type != TokenType::NEWLINE)
        reportError("invalid first token", offset, copy_content);

    current_type = TokenType::WHITESPACE;

    while (offset < copy_content.length() + 1)
    {
        char cur = ' ';
        if (offset < copy_content.length())
            cur = copy_content[offset];
        
        TokenType next_type = getType(cur);

        if (next_type == TokenType::INVALID)
            reportError("illegal character", offset, copy_content);

        TokenType new_type = current_type;
        bool append_cur = true;
        bool reset_token = false;
        Token finished_token = Token(current_type);
        finished_token.start_offset = start_offset;

        switch (current_type)
        {
        case TEXT:
            if (next_type == TokenType::TEXT || next_type == TokenType::INT)
                break;
            else if (!isSeparator(next_type))
                reportError("invalid conjoined tokens", offset, copy_content);

            finished_token.s_value = current_token;
            reset_token = true;
            break;
        case STRING:
            if (next_type == TokenType::STRING)
            {
                append_cur = false;
                finished_token.s_value = current_token.substr(1);
                reset_token = true;
                next_type = TokenType::INVALID;
                break;
            }
            else
                break;
        case INT:
            if (next_type == TokenType::INT)
                break;
            else if (next_type == TokenType::FLOAT)
            {
                new_type = TokenType::FLOAT;
                break;
            }
            else if (next_type == TokenType::TEXT)
            {
                new_type = TokenType::TEXT;
                break;
            }
            else if (!isSeparator(next_type))
                reportError("invalid conjoined tokens", offset, copy_content);

            finished_token.i_value = stoi(current_token);
            reset_token = true;
            break;
        case FLOAT:
            if (next_type == TokenType::INT)
                break;
            else if (next_type == TokenType::FLOAT)
                reportError("invalid float literal", offset, copy_content);
            else if (!isSeparator(next_type))
                reportError("invalid conjoined tokens", offset, content);

            finished_token.f_value = stof(current_token);
            reset_token = true;
            break;
        case TAG:
            if (next_type == TokenType::TEXT || next_type == TokenType::INT)
                break;
            else if (!isSeparator(next_type))
                reportError("invalid conjoined tokens", offset, copy_content);

            finished_token.s_value = current_token.substr(1);
            reset_token = true;
            break;
        case VECTOR4:
            reportError("unexpected vector closing token", offset, copy_content);
            break;
        case VECTOR2:
            if (next_type == TokenType::VECTOR4)
            {
                finished_token.type = decodeVectorToken(current_token.substr(1), finished_token.c_value, offset, copy_content);            
                append_cur = false;
                reset_token = true;
                next_type = TokenType::WHITESPACE;
                break;
            }
            if (next_type == TokenType::VECTOR2)
                reportError("invalid nested vector token", offset, copy_content);
            else
                break;
        case COMMENT:
            if (next_type != TokenType::COMMENT && current_token.size() < 2)
                reportError("incomplete comment initiator", offset, copy_content);
            else if (next_type != TokenType::NEWLINE)
                break;
            
            finished_token.s_value = current_token;
            reset_token = true;
            break;
        case WHITESPACE:
            if (next_type == TokenType::WHITESPACE)
                break;
            
            reset_token = true;
            break;
        case INVALID:
            if (!isSeparator(next_type))
                reportError("invalid conjoined tokens", offset, copy_content);
            
            reset_token = true;
            break;
        default:
            reset_token = true;
            break;
        }

        if (reset_token)
        {
            if (current_type != TokenType::INVALID)
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

    if (current_type != TokenType::WHITESPACE)
        reportError("invalid unclosed token at end of content", offset - 2, copy_content);

    return tokens;
}

vector<PTDeserialiser::Token> PTDeserialiser::prune(const vector<Token>& tokens)
{
    vector<Token> pruned;
    for (Token t : tokens)
        if (t.type != TokenType::WHITESPACE && t.type != TokenType::NEWLINE && t.type != TokenType::COMMENT && t.type != TokenType::INVALID)
            pruned.push_back(t);

    return pruned;
}

pair<string, PTResource*> PTDeserialiser::deserialiseResourceDescriptor(const std::vector<Token>& tokens, size_t& first_token, ResourceMap& res_map, const std::string& content)
{
    if (tokens.size() <= first_token + 4)
    {
        size_t o = 0;
        if (!tokens.empty())
            o = tokens[tokens.size() - 1].start_offset;
        reportError("no resource tokens to decode", o, content);
    }
    
    if (tokens[first_token].type != TokenType::TEXT || tokens[first_token].s_value != "Resource")
        reportError("malformed resource descriptor", tokens[first_token].start_offset, content);

    if (tokens[first_token + 1].type != TokenType::OPEN_ROUND)
        reportError("malformed resource descriptor", tokens[first_token].start_offset, content);

    size_t close_bracket = findClosingBracket(tokens, first_token + 1, false, content);

    if (tokens.size() <= close_bracket + 3)
        reportError("missing resource identifier", tokens[close_bracket].start_offset, content);
    if (tokens[close_bracket + 3].type != TokenType::SEMICOLON)
        reportError("missing semicolon", tokens[close_bracket].start_offset, content);

    size_t semicolon = close_bracket + 3;
    
    string name = "";

    if (tokens[close_bracket + 1].type != TokenType::COLON)
        reportError("expected colon after resource", tokens[close_bracket].start_offset, content);
    else if (tokens[close_bracket + 2].type != TokenType::TEXT)
        reportError("expected resource identifier after colon", tokens[close_bracket+1].start_offset, content);
    
    name = tokens[close_bracket + 2].s_value;

    vector<Token> bracket_contents;
    for (size_t i = first_token + 2; i <= close_bracket - 1; i++)
        bracket_contents.push_back(tokens[i]);

    if (bracket_contents.size() < 1)
        reportError("missing resource type name", tokens[first_token + 1].start_offset, content);
    if (bracket_contents[0].type != TokenType::TEXT)
        reportError("first argument of resource descriptor must be an identifier", bracket_contents[0].start_offset, content);
    string resource_type = bracket_contents[0].s_value;

    // split the rest into a list of lists of tokens
    if (bracket_contents.size() > 1 && bracket_contents[1].type != TokenType::COMMA)
        reportError("expected comma separated argument list", bracket_contents[1].start_offset, content);
    if (bracket_contents.size() == 2)
        reportError("token expected after comma", bracket_contents[1].start_offset, content);
    vector<vector<Token>> arguments;
    vector<Token> current_argument;
    vector<Token> bracket_stack;
    for (size_t i = 2; i < bracket_contents.size(); i++)
    {
        current_argument.push_back(bracket_contents[i]);
        switch(bracket_contents[i].type)
        {
            case OPEN_ROUND:
            case OPEN_CURLY:
                bracket_stack.push_back(bracket_contents[i]);
                break;
            case CLOSE_ROUND:
                if (!bracket_stack.empty() && bracket_stack[bracket_stack.size() - 1].type == TokenType::OPEN_ROUND)
                    bracket_stack.pop_back();
                else
                    reportError("invalid closing bracket", bracket_contents[i].start_offset, content);
                break;
            case CLOSE_CURLY:
                if (!bracket_stack.empty() && bracket_stack[bracket_stack.size() - 1].type == TokenType::OPEN_CURLY)
                    bracket_stack.pop_back();
                else
                    reportError("invalid closing curly brace", bracket_contents[i].start_offset, content);
                break;
            case COMMA:
                if (!bracket_stack.empty())
                    break;
                
                current_argument.pop_back();
                if (current_argument.empty())
                    reportError("missing argument before comma", bracket_contents[i].start_offset, content);
                arguments.push_back(current_argument);
                current_argument.clear();
            default:
                break;
        }
    }

    if (!bracket_stack.empty())
        reportError("missing closing bracket/brace", tokens[close_bracket - 1].start_offset, content);
    
    if (current_argument.empty())
        reportError("missing argument before comma", bracket_contents[bracket_contents.size() - 1].start_offset, content);
    arguments.push_back(current_argument);

    vector<Argument> initialiser_args;
    for (auto arg : arguments)
        initialiser_args.push_back(compileArgument(arg, res_map, content));

    PTResource* resource = PTResourceManager::get()->createGeneric(resource_type, initialiser_args);

    first_token = semicolon;
    return pair<string, PTResource*>{ name, resource };
}

PTNode* PTDeserialiser::deserialiseObject(const std::vector<Token>& tokens, size_t& first_token, PTScene* scene, ResourceMap& res_map, const std::string& content)
{
    if (tokens.size() <= first_token + 4)
    {
        size_t o = 0;
        if (!tokens.empty())
            o = tokens[tokens.size() - 1].start_offset;
        reportError("no tokens to decode", o, content);
    }
    
    if (tokens[first_token].type != TokenType::TEXT)
        reportError("expected node type name", tokens[first_token].start_offset, content);

    string object_type = tokens[first_token].s_value;

    if (tokens[first_token + 1].type != TokenType::OPEN_ROUND)
        reportError("malformed object descriptor", tokens[first_token].start_offset, content);

    size_t close_bracket = findClosingBracket(tokens, first_token + 1, false, content);
    size_t next_step = close_bracket + 1;

    string object_name = object_type;
    size_t semicolon = close_bracket;

    if (tokens[next_step].type == COLON)
    {
        if (tokens.size() > next_step + 1 && tokens[next_step + 1].type == TEXT)
            object_name = tokens[next_step + 1].s_value;
        else
            reportError("expected node name identifier after colon", tokens[next_step].start_offset, content);
        next_step += 2;
    }
    
    vector<PTNode*> children;

    if (tokens[next_step].type == OPEN_CURLY)
    {
        size_t open_curly = next_step;
        size_t close_curly = findClosingBracket(tokens, open_curly, true, content);
        if (tokens.size() <= close_curly + 1 || tokens[close_curly + 1].type != SEMICOLON)
            reportError("expected semicolon after close curly brace", tokens[close_curly].start_offset, content);
        semicolon = close_curly + 1;
        vector<Token> current_child;
        vector<Token> bracket_stack;
        for (size_t i = open_curly + 1; i < close_curly; i++)
        {
            current_child.push_back(tokens[i]);
            size_t tmp = 0;
            switch(tokens[i].type)
            {
                case OPEN_ROUND:
                case OPEN_CURLY:
                    bracket_stack.push_back(tokens[i]);
                    break;
                case CLOSE_ROUND:
                    if (!bracket_stack.empty() && bracket_stack[bracket_stack.size() - 1].type == TokenType::OPEN_ROUND)
                        bracket_stack.pop_back();
                    else
                        reportError("invalid closing bracket", tokens[i].start_offset, content);
                    break;
                case CLOSE_CURLY:
                    if (!bracket_stack.empty() && bracket_stack[bracket_stack.size() - 1].type == TokenType::OPEN_CURLY)
                        bracket_stack.pop_back();
                    else
                        reportError("invalid closing curly brace", tokens[i].start_offset, content);
                    break;
                case SEMICOLON:
                    if (!bracket_stack.empty())
                        break;
                    
                    if (current_child.empty())
                        reportError("missing child before semicolon", tokens[i].start_offset, content);
                    children.push_back(deserialiseObject(current_child, tmp, scene, res_map, content));
                    current_child.clear();
                    break;
                default:
                    break;
            }
        }
    }
    else if (tokens[next_step].type == SEMICOLON)
    {
        semicolon = next_step;
    }
    else
        reportError("expected node name, open curly brace, or semicolon after node definition", tokens[next_step].start_offset, content);

    size_t tmp_first = first_token + 1;
    auto args_list = deserialiseStatement(tokens, tmp_first, false, true, res_map, content);
    ArgMap initialiser_args;
    for (auto arg : args_list)
        initialiser_args[arg.first] = arg.second;

    PTNodeInstantiateFunc ptr = node_instantiators[object_type];
    
    if (ptr == nullptr)
        reportError("invalid node type", tokens[first_token].start_offset, content);

    PTNode* node = ptr(scene, object_name, initialiser_args);
    
    for (PTNode* child : children)
        child->getTransform()->setParent(node->getTransform());

    // TODO: if any errors occur (INCLUDING PREVIOUS REPORTERRORS), destroy child nodes in scene

    first_token = semicolon;
    return node;
}

void PTDeserialiser::deserialiseScene(PTScene* scene, const std::string& content)
{
    vector<Token> tokens = prune(tokenise(content));
    
    if (tokens.size() == 0) return;

    if (tokens.size() < 4)
        reportError("not enough tokens provided", 0, content);

    if (tokens[0].type != TokenType::TEXT)
        reportError("invalid first token", tokens[0].start_offset, content);
    
    ResourceMap res_map;
    size_t statement_first = 0;
    while (statement_first < tokens.size() - 1)
    {
        if (tokens[statement_first].type != TokenType::TEXT)
            reportError("invalid token", tokens[statement_first].start_offset, content);
        
        if (tokens[statement_first].s_value == "Resource")
        {
            size_t tmp_first = statement_first;
            auto res = deserialiseResourceDescriptor(tokens, statement_first, res_map, content);
            if (res.second == nullptr)
                debugLog("resource '" + res.first + "' with type " + tokens[tmp_first + 2].s_value + " could not be loaded");
            else
            {
                res_map[res.first] = res.second;
                scene->addResource(res.first, res.second);
                res.second->removeReferencer();
            }
        }
        else
        {
            deserialiseObject(tokens, statement_first, scene, res_map, content);
        }
        statement_first++;
    }

    // TODO: if any errors occur (INCLUDING PREVIOUS REPORTERRORS), destroy scene? no actually! resource manager will do that
}

vector<pair<string, PTDeserialiser::Argument>> PTDeserialiser::deserialiseStatement(const vector<Token>& tokens, size_t& first_token, bool allow_unnamed, bool allow_named, ResourceMap& res_map, const string& content)
{
    if (!allow_named && !allow_unnamed)
        reportError("at least one type of argument must be allowed for deserialisation", 0, content);

    // structure must be (something, something, thing = something);
    // (i.e. we need open bracket, close bracket, semicolon)
    if (tokens.size() <= first_token + 2)
    {
        size_t o = 0;
        if (!tokens.empty())
            o = tokens[tokens.size() - 1].start_offset;
        reportError("no tokens to decode", o, content);
    }
    
    if (tokens[first_token].type != TokenType::OPEN_ROUND && tokens[first_token].type != TokenType::OPEN_CURLY)
        reportError("expected open bracket/brace", tokens[first_token].start_offset, content);
    
    size_t end_bracket = findClosingBracket(tokens, first_token, false, content);

    vector<Token> bracket_contents;
    for (size_t i = first_token + 1; i <= end_bracket - 1; i++)
        bracket_contents.push_back(tokens[i]);

    // split the rest into a list of lists of tokens
    vector<vector<Token>> arguments;
    vector<Token> current_argument;
    vector<Token> bracket_stack;
    for (size_t i = 0; i < bracket_contents.size(); i++)
    {
        current_argument.push_back(bracket_contents[i]);
        switch(bracket_contents[i].type)
        {
            case OPEN_ROUND:
            case OPEN_CURLY:
                bracket_stack.push_back(bracket_contents[i]);
                break;
            case CLOSE_ROUND:
                if (!bracket_stack.empty() && bracket_stack[bracket_stack.size() - 1].type == TokenType::OPEN_ROUND)
                    bracket_stack.pop_back();
                else
                    reportError("invalid closing bracket", bracket_contents[i].start_offset, content);
                break;
            case CLOSE_CURLY:
                if (!bracket_stack.empty() && bracket_stack[bracket_stack.size() - 1].type == TokenType::OPEN_CURLY)
                    bracket_stack.pop_back();
                else
                    reportError("invalid closing curly brace", bracket_contents[i].start_offset, content);
                break;
            case COMMA:
                if (!bracket_stack.empty())
                    break;
                
                current_argument.pop_back();
                if (current_argument.empty())
                    reportError("missing argument before comma", bracket_contents[i].start_offset, content);
                arguments.push_back(current_argument);
                current_argument.clear();
            default:
                break;
        }
    }

    if (!bracket_stack.empty())
        reportError("missing closing bracket/brace", tokens[end_bracket].start_offset, content);
    
    if (current_argument.empty())
    {
        if (!bracket_contents.empty())
            reportError("missing argument after comma", bracket_contents[bracket_contents.size() - 1].start_offset, content);
    }
    else
        arguments.push_back(current_argument);

    vector<pair<string, PTDeserialiser::Argument>> args;
    for (auto arg : arguments)
    {
        pair<string, PTDeserialiser::Argument> pr;
        if (arg.size() < 3 || arg[1].type != PTDeserialiser::TokenType::EQUALS)
        {
            if (!allow_unnamed)
                reportError("invalid unnamed argument", arg[0].start_offset, content);
            else
                pr = { "", compileArgument(arg, res_map, content) };
        }
        else
        {
            if (!allow_named)
                reportError("invalid named argument", arg[0].start_offset, content);
            else
                pr = compileNamedArgument(arg, res_map, content);
        }
        args.push_back(pr);
    }

    first_token = end_bracket + 1;

    return args;
}

void PTDeserialiser::deserialiseMaterial(const std::string& content, MaterialParams& params, PTShader*& shader, std::vector<UniformParam>& uniforms, std::map<uint16_t, PTImage*>& textures)
{
    vector<Token> tokens = prune(tokenise(content));
    
    if (tokens.size() == 0) return;

    if (tokens.size() < 4)
        reportError("not enough tokens provided", 0, content);

    if (tokens[0].type != TokenType::TEXT)
        reportError("invalid first token", tokens[0].start_offset, content);
    
    ResourceMap res_map;
    size_t statement_first = 0;
    while (statement_first < tokens.size() - 1)
    {
        if (tokens[statement_first].type != TokenType::TEXT)
            reportError("invalid token", tokens[statement_first].start_offset, content);
        
        if (tokens[statement_first].s_value == "Resource")
        {
            auto res = deserialiseResourceDescriptor(tokens, statement_first, res_map, content);
            res_map[res.first] = res.second;
        }
        else if (tokens[statement_first].s_value == "Depth")
        {
            auto args = deserialiseStatement(tokens, ++statement_first, false, true, res_map, content);
            for (auto arg : args)
            {
                if (arg.first == "operation" && arg.second.type == ArgType::STRING_ARG)
                    params.depth_op = arg.second.s_val;
                else if (arg.first == "test" && arg.second.type == ArgType::INT_ARG)
                    params.depth_test = arg.second.i_val;
                else if (arg.first == "write" && arg.second.type == ArgType::INT_ARG)
                    params.depth_write = arg.second.i_val;
            }
            if (tokens[statement_first].type != TokenType::SEMICOLON)
                reportError("expected semicolon", tokens[statement_first].start_offset, content);
        }
        else if (tokens[statement_first].s_value == "Culling")
        {
            auto args = deserialiseStatement(tokens, ++statement_first, false, true, res_map, content);
            for (auto arg : args)
            {
                if (arg.first == "mode" && arg.second.type == ArgType::STRING_ARG)
                    params.culling = arg.second.s_val;
            }
            if (tokens[statement_first].type != TokenType::SEMICOLON)
                reportError("expected semicolon", tokens[statement_first].start_offset, content);
        }
        else if (tokens[statement_first].s_value == "Polygon")
        {
            auto args = deserialiseStatement(tokens, ++statement_first, false, true, res_map, content);
            for (auto arg : args)
            {
                if (arg.first == "mode" && arg.second.type == ArgType::STRING_ARG)
                    params.polygon_mode = arg.second.s_val;
            }
            if (tokens[statement_first].type != TokenType::SEMICOLON)
                reportError("expected semicolon", tokens[statement_first].start_offset, content);
        }
        else if (tokens[statement_first].s_value == "Shader")
        {
            auto args = deserialiseStatement(tokens, ++statement_first, false, true, res_map, content);
            for (auto arg : args)
            {
                if (arg.first == "resource" && arg.second.type == ArgType::RESOURCE_ARG)
                    shader = dynamic_cast<PTShader*>(arg.second.r_val);
            }
            if (tokens[statement_first].type != TokenType::SEMICOLON)
                reportError("expected semicolon", tokens[statement_first].start_offset, content);
        }
        else if (tokens[statement_first].s_value == "Uniform")
        {
            uint16_t binding = -1;
            auto args = deserialiseStatement(tokens, ++statement_first, false, true, res_map, content);
            for (auto arg : args)
            {
                if (arg.first == "binding" && arg.second.type == ArgType::INT_ARG)
                    binding = arg.second.i_val;
            }
            if (tokens[statement_first].type == TokenType::OPEN_CURLY)
            {
                size_t first_variable = statement_first + 1;
                auto variables = deserialiseStatement(tokens, statement_first, true, false, res_map, content);
                size_t offset = 0;
                for (auto var : variables)
                {
                    UniformParam uniform;
                    uniform.binding = binding;
                    uniform.offset = offset;
                    uniform.identifier = var.first;
                    uniform.value = var.second;

                    switch (uniform.value.type)
                    {
                        case ArgType::RESOURCE_ARG:
                        case ArgType::STRING_ARG:
                        case ArgType::ARRAY_ARG:
                            reportError("invalid uniform variable type", tokens[first_variable].start_offset, content);
                            break;
                        case ArgType::FLOAT_ARG: uniform.size = sizeof(float); break;
                        case ArgType::INT_ARG: uniform.size = sizeof(int); break;
                        case ArgType::VECTOR2_ARG: uniform.size = sizeof(PTVector2f); break;
                        case ArgType::VECTOR3_ARG: uniform.size = sizeof(PTVector3f); break;
                        case ArgType::VECTOR4_ARG: uniform.size = sizeof(PTVector4f); break;
                    }
                    offset += uniform.size;

                    uniforms.push_back(uniform);
                }
            }
            else if (tokens[statement_first].type != TokenType::SEMICOLON)
                reportError("expected semicolon", tokens[statement_first].start_offset, content);
        }
        else if (tokens[statement_first].s_value == "Texture")
        {
            uint16_t binding = -1;
            PTImage* tex = nullptr;
            auto args = deserialiseStatement(tokens, ++statement_first, false, true, res_map, content);
            for (auto arg : args)
            {
                if (arg.first == "binding" && arg.second.type == ArgType::INT_ARG)
                    binding = arg.second.i_val;
                else if (arg.first == "resource" && arg.second.type == ArgType::RESOURCE_ARG)
                    tex = dynamic_cast<PTImage*>(arg.second.r_val);
            }
            textures[binding] = tex;
        }
        else
        {
            reportError("invalid statement", tokens[statement_first].start_offset, content);
        }
        statement_first++;
    }
}

inline PTDeserialiser::TokenType PTDeserialiser::getType(const char c)
{
    if (isAlphabetic(c) || c == '_') return TokenType::TEXT;
    if (c == '"') return TokenType::STRING;
    if (c == '-' || (c >= '0' && c <= '9')) return TokenType::INT;
    if (c == '.') return TokenType::FLOAT;
    if (c == '@') return TokenType::TAG;
    if (c == '[') return TokenType::VECTOR2;
    if (c == ']') return TokenType::VECTOR4;
    if (c == '(') return TokenType::OPEN_ROUND;
    if (c == ')') return TokenType::CLOSE_ROUND;
    if (c == '{') return TokenType::OPEN_CURLY;
    if (c == '}') return TokenType::CLOSE_CURLY;
    if (c == '\n') return TokenType::NEWLINE;
    if (c == ':') return TokenType::COLON;
    if (c == ';') return TokenType::SEMICOLON;
    if (c == '=') return TokenType::EQUALS;
    if (c == ',') return TokenType::COMMA;
    if (c == '/') return TokenType::COMMENT;
    if (c == ' ' || c == '\t') return TokenType::WHITESPACE;
    return TokenType::INVALID;
}

inline bool PTDeserialiser::isAlphabetic(const char c)
{
    if (c >= 'a' && c <= 'z') return true;
    if (c >= 'A' && c <= 'Z') return true;

    return false;
}

inline bool PTDeserialiser::isSeparator(TokenType t)
{
    switch (t)
    {
    case TEXT:
    case STRING:
    case INT:
    case TAG:
        return false;
    default:
        return true;
    }
}

void PTDeserialiser::reportError(const string err, size_t off, const string& str)
{
    int32_t extract_start = max(0, (int32_t)off - 16);
    int32_t extract_end = extract_start + 32;
    while (true)
    {
        size_t find = str.find('\n', extract_start);
        if (find >= off) break;
        extract_start = static_cast<int32_t>(find + 1);
    }
    size_t find = str.find('\n', off);
    if (find != string::npos)
    {
        if ((int32_t)find < extract_end)
            extract_end = static_cast<int32_t>(find);
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

size_t PTDeserialiser::findClosingBracket(const vector<Token>& tokens, size_t open_index, bool allow_semicolons, const string& content)
{
    vector<Token> brackets;
    string bracket_name = string(tokens[open_index].type == TokenType::OPEN_ROUND ? "bracket" : "curly brace");
    size_t index = open_index;

    while (index < tokens.size())
    {
        switch(tokens[index].type)
        {
            case SEMICOLON:
                if (!allow_semicolons)
                    reportError("missing closing " + bracket_name, tokens[open_index].start_offset, content);
                break;
            case OPEN_ROUND:
            case OPEN_CURLY:
                brackets.push_back(tokens[index]);
                break;
            case CLOSE_ROUND:
                if (!brackets.empty() && brackets[brackets.size() - 1].type == TokenType::OPEN_ROUND)
                    brackets.pop_back();
                else
                    reportError("invalid closing bracket", tokens[index].start_offset, content);
                break;
            case CLOSE_CURLY:
                if (!brackets.empty() && brackets[brackets.size() - 1].type == TokenType::OPEN_CURLY)
                    brackets.pop_back();
                else
                    reportError("invalid closing curly brace", tokens[index].start_offset, content);
                break;
            default:
                break;
        }

        if (brackets.size() == 0)
            break;

        index++;
    }

    if (index >= tokens.size())
        reportError("missing closing " + bracket_name, tokens[open_index].start_offset, content);

    return index;
}

PTDeserialiser::Argument PTDeserialiser::compileArgument(const std::vector<Token>& tokens, ResourceMap& res_map, const std::string& content)
{
    Argument arg;

    if (tokens.empty())
        reportError("empty argument passed to compiler", 0, content);

    if (tokens.size() == 1)
    {
        PTResource* res = nullptr;
        Token t = tokens[0];
        switch (t.type)
        {
        case STRING:
            arg.s_val = t.s_value;
        case INT:
            arg.i_val = t.i_value;
        case FLOAT:
            arg.f_val = t.f_value;
        case VECTOR2:
            arg.v2_val = PTVector2f{ t.c_value.x, t.c_value.y };
        case VECTOR3:
            arg.v3_val = PTVector3f{ t.c_value.x, t.c_value.y, t.c_value.z };
        case VECTOR4:
            arg.v4_val = PTVector4f{ t.c_value.x, t.c_value.y, t.c_value.z, t.c_value.w };
            arg.type = (ArgType)t.type;
            break;
        case TAG:
            if (!res_map.contains(t.s_value))
                reportError("reference to undefined resource", t.start_offset, content);
            res = res_map[t.s_value];
            arg.r_val = res;
            arg.type = RESOURCE_ARG;
            break;
        default:
            reportError("invalid token type", t.start_offset, content);
        }
    }
    else
    {
        // TODO: handle arrays
    }

    return arg;
}

pair<string, PTDeserialiser::Argument> PTDeserialiser::compileNamedArgument(const vector<PTDeserialiser::Token>& tokens, ResourceMap& res_map, const string& content)
{
    Argument arg;

    if (tokens.size() < 3)
        reportError("not enough tokens to construct named argument", 0, content);

    if (tokens[0].type != TokenType::TEXT)
        reportError("token before equals must be an identifier", tokens[0].start_offset, content);
    if (tokens[1].type != TokenType::EQUALS)
        reportError("expected identifier followed by equals", tokens[1].start_offset, content);
    vector<Token> tmp = tokens;
    tmp.erase(tmp.begin());
    tmp.erase(tmp.begin());

    arg = compileArgument(tmp, res_map, content);

    return pair<string, Argument>{ tokens[0].s_value, arg };
}

PTDeserialiser::TokenType PTDeserialiser::decodeVectorToken(const std::string token, PTVector4f& vector_out, size_t offset, const std::string& content)
{
    vector<string> axes;
    size_t i = -1;
    bool set = true;
    while (i != string::npos || set)
    {
        set = false;
        size_t i_old = ++i;
        i = token.find(',', i + 1);
        axes.push_back(token.substr(i_old,i_old - i));
    }

    if (axes.size() < 2 || axes.size() > 4)
        reportError("incorrect number of vector components", offset, content);
    
    TokenType type = TokenType::VECTOR2;
    vector_out.x = stof(axes[0]);
    vector_out.y = stof(axes[1]);
    if (axes.size() > 2)
    {
        vector_out.z = stof(axes[2]);
        type = TokenType::VECTOR3;
    }
    if (axes.size() > 3)
    {
        vector_out.w = stof(axes[3]);
        type = TokenType::VECTOR4;
    }

    return type;
}

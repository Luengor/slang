#pragma once

#include <string>
#include <vector>

struct Token {
    enum Type {
        // Single-character tokens.
        LeftParen, RightParen, LeftBrace, RightBrace,
        Comma, Dot, Minus, Plus, Semicolon, Slash, Star,
        Bang, Equal, Greater, Less,

        // Literals.
        Identifier, String, Number,

        // Keywords.
        And, Else, False, Fn, For, If, Nil, Or,
        Print, Return, True, Var, While,

        Error, Eof
    } type;

    std::string lexeme;
    int line;
};

class Scanner {
    int line = 1;
    unsigned start = 0;
    unsigned current = 0;
    std::string source;

    char peek();
    void skipWhitespace();

    Token scanToken();
    Token makeToken(Token::Type type);
    Token makeString();
    Token makeNumber();
    Token makeIdentifier();

public:
    std::vector<Token> scan(const std::string& input);
};


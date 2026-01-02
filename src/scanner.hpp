#pragma once

#include <string>
#include <vector>

struct Token {
    enum Type : unsigned {
        // Single-character tokens.
        LeftParen, RightParen,
        LeftBrace, RightBrace,
        Minus, Plus, Slash, Star,
        Greater, Less, Semicolon,
        Equal, Comma, Question, Colon,

        // Two-character tokens.
        EqualEqual, BangEqual,
        GreaterEqual, LessEqual,
        Arrow,

        // Literals.
        Identifier, String, Number,

        // Keywords.
        And, Or, Not, True, False,
        Fixed, Float, Bool, Str, None, Auto,
        If, Else, While, For, Return,
        
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


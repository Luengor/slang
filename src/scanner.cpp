#include "scanner.hpp"
#include <cctype>

char Scanner::peek() {
    if (this->current >= this->source.length())
        return '\0';
    return this->source[this->current];
}

void Scanner::skipWhitespace() {
    for (;;) {
        char c = this->source[this->current];
        switch (c) {
            case '\n':
                this->line++;
            case ' ':
            case '\r':
            case '\t':
                this->current++;
                break;
            case '#':
                // A comment goes until the end of the line.
                while (this->current < this->source.length() &&
                       this->peek() != '\n') {
                    this->current++;
                }
                break;
            default:
                return;
        }
    }
}

Token Scanner::makeToken(Token::Type type) {
    return Token{
        .type = type,
        .lexeme = this->source.substr(this->start, this->current - this->start),
        .line = this->line};
}

Token Scanner::scanToken() {
    // Skip whitespace
    this->skipWhitespace();

    this->start = this->current;

    // eof
    if (this->current >= this->source.length()) {
        return this->makeToken(Token::Eof);
    }

    char c = this->source[this->current++];

    // check for keywords/identifiers
    if (std::isalpha(c) || c == '_')
        return this->makeIdentifier();

    // check for digits
    if (std::isdigit(c))
        return this->makeNumber();

    // letters, symbols...
    switch (c) {
        case '(':
            return this->makeToken(Token::LeftParen);
        case ')':
            return this->makeToken(Token::RightParen);
        case '{':
            return this->makeToken(Token::LeftBrace);
        case '}':
            return this->makeToken(Token::RightBrace);
        case '-':
            if (this->peek() == '>') {
                this->current++;
                return this->makeToken(Token::Arrow);
            }
            return this->makeToken(Token::Minus);
        case '+':
            return this->makeToken(Token::Plus);
        case '*':
            return this->makeToken(Token::Star);
        case '/':
            return this->makeToken(Token::Slash);
        case ',':
            return this->makeToken(Token::Comma);
        case ';':
            return this->makeToken(Token::Semicolon);
        case ':':
            return this->makeToken(Token::Colon);
        case '>':
            if (this->peek() == '=') {
                this->current++;
                return this->makeToken(Token::GreaterEqual);
            }
            return this->makeToken(Token::Greater);
        case '<':
            if (this->peek() == '=') {
                this->current++;
                return this->makeToken(Token::LessEqual);
            }
            return this->makeToken(Token::Less);
        case '=':
            if (this->peek() == '=') {
                this->current++;
                return this->makeToken(Token::EqualEqual);
            }
            return this->makeToken(Token::Equal);
        case '?':
            return this->makeToken(Token::Question);
        case '!':
            if (this->peek() == '=') {
                this->current++;
                return this->makeToken(Token::BangEqual);
            }
            break;
        // Literal
        case '"':
            return this->makeString();
    }

    return Token{Token::Error, "Unrecognized character.", this->line};
}

Token Scanner::makeString() {
    while (this->current < this->source.length() && this->peek() != '"') {
        if (this->source[this->current] == '\n') {
            this->line++;
        }
        this->current++;
    }

    if (this->current >= this->source.length()) {
        return Token{Token::Error, "Unterminated string.", this->line};
    }

    // The closing "
    this->current++;

    return this->makeToken(Token::String);
}

Token Scanner::makeNumber() {
    // Scan digits
    while (std::isdigit(this->peek())) {
        this->current++;
    }

    // fractional part?
    if (this->peek() == '.') {
        // consume . and continue
        this->current++;

        while (std::isdigit(this->peek())) {
            this->current++;
        }
    }

    return this->makeToken(Token::Number);
}

Token Scanner::makeIdentifier() {
    while (std::isalnum(this->peek()) || this->peek() == '_') {
        this->current++;
    }

    Token token = makeToken(Token::Identifier);

    // Choose token type based on keywords
    switch (token.lexeme[0]) {
        case 'a':
            if (token.lexeme == "and")
                token.type = Token::And;
            else if (token.lexeme == "auto")
                token.type = Token::Auto;
            break;
        case 'b':
            if (token.lexeme == "bool")
                token.type = Token::Bool;
            break;
        case 'e':
            if (token.lexeme == "else")
                token.type = Token::Else;
            break;
        case 'f':
            if (token.lexeme == "false")
                token.type = Token::False;
            else if (token.lexeme == "float")
                token.type = Token::Float;
            else if (token.lexeme == "fixed")
                token.type = Token::Fixed;
            else if (token.lexeme == "for")
                token.type = Token::For;
            break;
        case 'i':
            if (token.lexeme == "if")
                token.type = Token::If;
            break;
        case 'n':
            if (token.lexeme == "not")
                token.type = Token::Not;
            else if (token.lexeme == "none")
                token.type = Token::None;
            break;
        case 'o':
            if (token.lexeme == "or")
                token.type = Token::Or;
            break;
        case 'r':
            if (token.lexeme == "return")
                token.type = Token::Return;
            break;
        case 's':
            if (token.lexeme == "str")
                token.type = Token::Str;
            break;
        case 't':
            if (token.lexeme == "true")
                token.type = Token::True;
            break;
        case 'w':
            if (token.lexeme == "while")
                token.type = Token::While;
            break;
    }

    return token;
}

std::vector<Token> Scanner::scan(const std::string &input) {
    std::vector<Token> tokens;
    this->source = std::move(input);

    for (;;) {
        Token token = scanToken();
        tokens.push_back(token);
        if (token.type == Token::Eof)
            break;
    }

    return tokens;
}

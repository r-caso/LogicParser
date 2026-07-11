#pragma once

#include <string>

namespace iif_sadaf::talk::LogicParser {
    enum class TokenType {
        OPERATOR,
        QUANTIFIER,
        IDENTIFIER,
        LPAREN,
        RPAREN,
        COMMA,
        EQUAL,
        EOI
    };

    struct Token {
        TokenType type;
        std::string literal;
        std::string mapped_name; // Contains semantic name for operators or quantifiers, if applicable
    };
} // namespace iif_sadaf::talk::LogicParser


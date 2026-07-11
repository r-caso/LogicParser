#include "LogicParser/lexer.hpp"

#include <algorithm>
#include <cctype>

namespace iif_sadaf::talk::LogicParser {

namespace {

bool is_ident_start(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool is_ident_char(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

struct LiteralMapping {
    std::string literal;
    std::string mapped_name;
    bool is_quantifier;
};

} // namespace

std::expected<std::vector<Token>, std::string> Lexer::tokenize(
    std::string_view input,
    const ParserConfig& config
) {
    std::vector<Token> tokens;

    // Combine all operator and quantifier literals into a single list sorted by length descending
    // to ensure we do maximal-munch matching (e.g. "<->" before "<->").
    std::vector<LiteralMapping> mappings;
    for (const auto& [lit, name] : config.operator_mappings) {
        mappings.push_back({lit, name, false});
    }
    for (const auto& [lit, name] : config.quantifier_mappings) {
        mappings.push_back({lit, name, true});
    }

    std::sort(mappings.begin(), mappings.end(), [](const LiteralMapping& a, const LiteralMapping& b) {
        return a.literal.length() > b.literal.length();
    });

    size_t pos = 0;
    const size_t len = input.length();

    while (pos < len) {
        // Skip whitespace
        if (std::isspace(static_cast<unsigned char>(input[pos]))) {
            pos++;
            continue;
        }

        // Try to match registered operators or quantifiers
        bool matched = false;
        for (const auto& mapping : mappings) {
            if (pos + mapping.literal.length() <= len &&
                input.compare(pos, mapping.literal.length(), mapping.literal) == 0) {

                // If the literal is alphanumeric, verify it is a full word match (maximal munch)
                bool is_alphanumeric_literal = true;
                for (char c : mapping.literal) {
                    if (!is_ident_char(c)) {
                        is_alphanumeric_literal = false;
                        break;
                    }
                }

                if (is_alphanumeric_literal) {
                    size_t next_char_pos = pos + mapping.literal.length();
                    if (next_char_pos < len && is_ident_char(input[next_char_pos])) {
                        // This matches a prefix of a longer identifier (e.g., "forall_x" starts with "forall")
                        // Skip it so it is lexed as an identifier
                        continue;
                    }
                }

                TokenType ttype = mapping.is_quantifier ? TokenType::QUANTIFIER : TokenType::OPERATOR;
                tokens.push_back(Token{
                    .type = ttype,
                    .literal = mapping.literal,
                    .mapped_name = mapping.mapped_name
                });
                pos += mapping.literal.length();
                matched = true;
                break;
            }
        }

        if (matched) {
            continue;
        }

        // Match standard single character punctuations
        char c = input[pos];
        if (c == '(') {
            tokens.push_back(Token{.type = TokenType::LPAREN, .literal = "(", .mapped_name = ""});
            pos++;
        } else if (c == ')') {
            tokens.push_back(Token{.type = TokenType::RPAREN, .literal = ")", .mapped_name = ""});
            pos++;
        } else if (c == ',') {
            tokens.push_back(Token{.type = TokenType::COMMA, .literal = ",", .mapped_name = ""});
            pos++;
        } else if (c == '=') {
            tokens.push_back(Token{.type = TokenType::EQUAL, .literal = "=", .mapped_name = ""});
            pos++;
        } else if (is_ident_start(c)) {
            // Lex identifier
            size_t start = pos;
            while (pos < len && is_ident_char(input[pos])) {
                pos++;
            }
            std::string ident(input.substr(start, pos - start));
            tokens.push_back(Token{.type = TokenType::IDENTIFIER, .literal = std::move(ident), .mapped_name = ""});
        } else {
            size_t char_len = 1;
            if (pos < len) {
                unsigned char first = static_cast<unsigned char>(input[pos]);
                if ((first & 0xE0) == 0xC0) char_len = 2;
                else if ((first & 0xF0) == 0xE0) char_len = 3;
                else if ((first & 0xF8) == 0xF0) char_len = 4;
            }
            if (pos + char_len > len) {
                char_len = len - pos;
            }
            std::string bad_char(input.substr(pos, char_len));
            return std::unexpected("Lexical error: unexpected character '" + bad_char + "' at position " + std::to_string(pos));
        }
    }

    // Add EndOfFile token
    tokens.push_back(Token{.type = TokenType::EOI, .literal = "", .mapped_name = ""});
    return tokens;
}

} // namespace iif_sadaf::talk::LogicParser


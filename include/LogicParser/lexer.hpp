#pragma once

#include <expected>
#include <string_view>
#include <vector>

#include "parser_config.hpp"
#include "token.hpp"

namespace iif_sadaf::talk::LogicParser {
    class Lexer {
    public:
        static std::expected<std::vector<Token>, std::string> tokenize(
            std::string_view input,
            const ParserConfig& config
        );
    };
} // namespace iif_sadaf::talk::LogicParser


#pragma once

#include <expected>
#include <string>
#include <string_view>

#include <LogicAST/LogicAST.hpp>

#include "parser_config.hpp"

namespace iif_sadaf::talk::LogicParser {

using ParseResult = std::expected<LogicAST::Expression, std::string>;
    ParseResult parse_propositional(
        std::string_view input,
        const LogicAST::ParseContext& ctx,
        const ParserConfig& config
    );

    ParseResult parse_first_order(
        std::string_view input,
        const LogicAST::ParseContext& ctx,
        const ParserConfig& config
    );
} // namespace iif_sadaf::talk::LogicParser


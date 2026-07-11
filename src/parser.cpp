#include "LogicParser/parser.hpp"

#include <cctype>
#include <regex>

#include "LogicParser/lexer.hpp"

namespace iif_sadaf::talk::LogicParser {
    namespace {
        class ParserInstance {
        public:
            ParserInstance(std::vector<Token> tokens, const LogicAST::ParseContext& ctx, const ParserConfig& config, bool first_order_mode)
                : tokens_(std::move(tokens)), ctx_(ctx), config_(config), first_order_mode_(first_order_mode), pos_(0),
                  var_rx_(config.variable_pattern), const_rx_(config.constant_pattern) {}

            ParseResult parse() {
                if (tokens_.empty() || tokens_[0].type == TokenType::EOI) {
                    return std::unexpected("Syntax error: empty input");
                }
                auto expr = parse_expression(0);
                if (!expr) {
                    return std::unexpected(expr.error());
                }
                if (peek().type != TokenType::EOI) {
                    return std::unexpected("Syntax error: trailing tokens starting with " + format_token(peek()));
                }
                return expr;
            }

        private:
            std::string format_token(const Token& t) const {
                if (t.type == TokenType::EOI) {
                    return "end of input";
                }
                return "'" + t.literal + "'";
            }

            const Token& peek() const {
                if (pos_ >= tokens_.size()) {
                    static const Token eof_token{.type = TokenType::EOI, .literal = "", .mapped_name = ""};
                    return eof_token;
                }
                return tokens_[pos_];
            }

            Token consume() {
                const Token& t = peek();
                if (t.type != TokenType::EOI) {
                    pos_++;
                }
                return t;
            }

            ParseResult parse_expression(int min_precedence) {
                auto lhs = parse_prefix();
                if (!lhs) return lhs;

                while (true) {
                    const Token& t = peek();
                    if (t.type != TokenType::OPERATOR) {
                        break;
                    }

                    auto it = config_.operator_properties.find(t.mapped_name);
                    if (it == config_.operator_properties.end() || it->second.arity != Arity::BINARY) {
                        // If the operator has no properties or is not binary, it cannot be an infix operator
                        break;
                    }

                    const auto& props = it->second;
                    if (props.precedence < min_precedence) {
                        break;
                    }

                    Token op_token = consume();

                    int next_min_prec = (props.associativity == Associativity::RIGHT) ? props.precedence : props.precedence + 1;
                    auto rhs = parse_expression(next_min_prec);
                    if (!rhs) return rhs;

                    LogicAST::Operator op{.name = op_token.mapped_name, .literal = op_token.literal};
                    try {
                        lhs = LogicAST::make_binary(ctx_, std::move(op), std::move(*lhs), std::move(*rhs));
                    } catch (const std::exception& e) {
                        return std::unexpected("Validation error: " + std::string(e.what()));
                    }
                }

                return lhs;
            }

            ParseResult parse_prefix() {
                const Token& t = peek();

                if (t.type == TokenType::OPERATOR) {
                    auto it = config_.operator_properties.find(t.mapped_name);
                    if (it == config_.operator_properties.end() || it->second.arity != Arity::UNARY) {
                        return std::unexpected("Syntax error: operator " + format_token(t) + " cannot be used in prefix position");
                    }

                    const auto& props = it->second;
                    Token op_token = consume();

                    // Parse operand with the operator's precedence
                    auto operand = parse_expression(props.precedence);
                    if (!operand) return operand;

                    LogicAST::Operator op{.name = op_token.mapped_name, .literal = op_token.literal};
                    try {
                        return LogicAST::make_unary(ctx_, std::move(op), std::move(*operand));
                    } catch (const std::exception& e) {
                        return std::unexpected("Validation error: " + std::string(e.what()));
                    }
                }

                if (t.type == TokenType::QUANTIFIER) {
                    if (!first_order_mode_) {
                        return std::unexpected("Syntax error: quantifier '" + t.literal + "' is not allowed in propositional mode");
                    }
                    Token q_token = consume();

                    const Token& var_token = peek();
                    if (var_token.type != TokenType::IDENTIFIER) {
                        return std::unexpected("Syntax error: expected variable identifier after quantifier " + format_token(q_token) + ", got " + format_token(var_token));
                    }

                    std::string var_literal = var_token.literal;
                    consume(); // Consume variable identifier

                    LogicAST::Term variable{.literal = var_literal, .type = ctx_.variable_tag};

                    // Quantifiers bind like unary operators, with highest precedence (4)
                    auto scope = parse_expression(4);
                    if (!scope) return scope;

                    LogicAST::Quantifier quant{.name = q_token.mapped_name, .literal = q_token.literal};
                    try {
                        return LogicAST::make_quantification(ctx_, std::move(quant), std::move(variable), std::move(*scope));
                    } catch (const std::exception& e) {
                        return std::unexpected("Validation error: " + std::string(e.what()));
                    }
                }

                if (t.type == TokenType::LPAREN) {
                    consume(); // Consume LParen
                    auto expr = parse_expression(0);
                    if (!expr) return expr;

                    if (peek().type != TokenType::RPAREN) {
                        return std::unexpected("Syntax error: expected ')', got " + format_token(peek()));
                    }
                    consume(); // Consume RParen
                    return expr;
                }

                if (t.type == TokenType::IDENTIFIER) {
                    if (!first_order_mode_) {
                        Token ident_token = consume();
                        return LogicAST::make_atomic(ident_token.literal);
                    } else {
                        std::string first_ident = t.literal;

                        // Peek at next token
                        if (pos_ + 1 < tokens_.size() && tokens_[pos_ + 1].type == TokenType::EQUAL) {
                            auto lhs_type = determine_term_type(first_ident);
                            if (!lhs_type) return std::unexpected(lhs_type.error());

                            LogicAST::Term lhs_term{.literal = first_ident, .type = *lhs_type};
                            consume(); // Consume LHS identifier
                            consume(); // Consume '='

                            const Token& rhs_tok = peek();
                            if (rhs_tok.type != TokenType::IDENTIFIER) {
                                return std::unexpected("Syntax error: expected term identifier on RHS of identity, got " + format_token(rhs_tok));
                            }
                            std::string rhs_ident = rhs_tok.literal;
                            auto rhs_type = determine_term_type(rhs_ident);
                            if (!rhs_type) return std::unexpected(rhs_type.error());

                            LogicAST::Term rhs_term{.literal = rhs_ident, .type = *rhs_type};
                            consume(); // Consume RHS identifier

                            try {
                                return LogicAST::make_identity(ctx_, std::move(lhs_term), std::move(rhs_term));
                            } catch (const std::exception& e) {
                                return std::unexpected("Validation error: " + std::string(e.what()));
                            }
                        } else {
                            std::string pred_name = first_ident;
                            consume(); // Consume predicate identifier

                            std::vector<LogicAST::Term> arguments;
                            if (peek().type == TokenType::LPAREN) {
                                consume(); // Consume '('
                                while (peek().type != TokenType::RPAREN && peek().type != TokenType::EOI) {
                                    const Token& arg_tok = peek();
                                    if (arg_tok.type != TokenType::IDENTIFIER) {
                                        return std::unexpected("Syntax error: expected term identifier in predication arguments, got " + format_token(arg_tok));
                                    }
                                    auto arg_type = determine_term_type(arg_tok.literal);
                                    if (!arg_type) return std::unexpected(arg_type.error());

                                    arguments.push_back(LogicAST::Term{.literal = arg_tok.literal, .type = *arg_type});
                                    consume(); // Consume term identifier

                                    if (peek().type == TokenType::COMMA) {
                                        consume(); // Consume ','
                                        if (peek().type == TokenType::RPAREN) {
                                            return std::unexpected("Syntax error: trailing comma in predication arguments");
                                        }
                                    } else if (peek().type != TokenType::RPAREN) {
                                        return std::unexpected("Syntax error: expected ',' or ')' in predication arguments, got " + format_token(peek()));
                                    }
                                }
                                if (peek().type != TokenType::RPAREN) {
                                    return std::unexpected("Syntax error: expected ')' at the end of predication arguments");
                                }
                                consume(); // Consume ')'
                            }

                            try {
                                return LogicAST::make_predication(ctx_, std::move(pred_name), std::move(arguments));
                            } catch (const std::exception& e) {
                                return std::unexpected("Validation error: " + std::string(e.what()));
                            }
                        }
                    }
                }

                return std::unexpected("Syntax error: unexpected token " + format_token(t) + " in prefix position");
            }

            std::expected<std::string, std::string> determine_term_type(const std::string& literal) {
                if (std::regex_match(literal, var_rx_)) {
                    if (ctx_.variable_tag.empty()) {
                        return std::unexpected("Variable tag is empty in ParseContext.");
                    }
                    if (!ctx_.term_types.is_registered(ctx_.variable_tag)) {
                        return std::unexpected("Variable tag \"" + ctx_.variable_tag + "\" is not registered in term_types.");
                    }
                    return ctx_.variable_tag;
                }

                if (!std::regex_match(literal, const_rx_)) {
                    return std::unexpected("Identifier \"" + literal + "\" matches neither variable nor constant patterns.");
                }

                if (!config_.default_constant_type.empty()) {
                    if (!ctx_.term_types.is_registered(config_.default_constant_type)) {
                        return std::unexpected("Configured default_constant_type \"" + config_.default_constant_type + "\" is not registered in term_types.");
                    }
                    return config_.default_constant_type;
                }

                // Fallback: try common tags
                for (const std::string& tag : {"CONSTANT", "INDIVIDUAL", "TERM", "constant", "individual", "term"}) {
                    if (ctx_.term_types.is_registered(tag)) {
                        return tag;
                    }
                }

                return std::unexpected("Could not determine term type for constant \"" + literal + "\". Please specify default_constant_type in ParserConfig.");
            }

            std::vector<Token> tokens_;
            const LogicAST::ParseContext& ctx_;
            const ParserConfig& config_;
            bool first_order_mode_;
            size_t pos_;
            std::regex var_rx_;
            std::regex const_rx_;
        };
    } // namespace

    ParseResult parse_propositional(
        std::string_view input,
        const LogicAST::ParseContext& ctx,
        const ParserConfig& config
    ) {
        auto tokens_res = Lexer::tokenize(input, config);
        if (!tokens_res) {
            return std::unexpected(tokens_res.error());
        }
        ParserInstance parser(std::move(*tokens_res), ctx, config, false);
        return parser.parse();
    }

    ParseResult parse_first_order(
        std::string_view input,
        const LogicAST::ParseContext& ctx,
        const ParserConfig& config
    ) {
        auto tokens_res = Lexer::tokenize(input, config);
        if (!tokens_res) {
            return std::unexpected(tokens_res.error());
        }
        ParserInstance parser(std::move(*tokens_res), ctx, config, true);
        return parser.parse();
    }
} // namespace iif_sadaf::talk::LogicParser


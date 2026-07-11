#include <gtest/gtest.h>

#include <memory>
#include <variant>

#include "LogicAST/LogicAST.hpp"

#include "LogicParser/parser.hpp"
#include "LogicParser/lexer.hpp"

using namespace iif_sadaf::talk;
using namespace iif_sadaf::talk::LogicParser;

class PropositionalParserTest : public ::testing::Test {
protected:
    LogicAST::ParseContext ctx;
    ParserConfig config = ParserConfig::create_standard_logic_config();

    void SetUp() override {
        ctx.term_types.register_type("CONSTANT");
        ctx.term_types.register_type("VARIABLE");
        ctx.variable_tag = "VARIABLE";
        ctx.operator_names.register_type("NEGATION");
        ctx.operator_names.register_type("CONJUNCTION");
        ctx.operator_names.register_type("DISJUNCTION");
        ctx.operator_names.register_type("CONDITIONAL");
        ctx.operator_names.register_type("CONDITIONAL_ALT");
        ctx.operator_names.register_type("BICONDITIONAL");
    }
};

TEST_F(PropositionalParserTest, SimpleAtomic) {
    auto res = parse_propositional("p", ctx, config);
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Atomic>>(*res));
    EXPECT_EQ(std::get<std::unique_ptr<LogicAST::Atomic>>(*res)->literal, "p");
}

TEST_F(PropositionalParserTest, UnaryOperator) {
    auto res = parse_propositional("~p", ctx, config);
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Unary>>(*res));
    const auto& unary = std::get<std::unique_ptr<LogicAST::Unary>>(*res);
    EXPECT_EQ(unary->op.name, "NEGATION");
    EXPECT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Atomic>>(unary->scope));
}

TEST_F(PropositionalParserTest, BinaryOperator) {
    auto res = parse_propositional("p & q", ctx, config);
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Binary>>(*res));
    const auto& bin = std::get<std::unique_ptr<LogicAST::Binary>>(*res);
    EXPECT_EQ(bin->op.name, "CONJUNCTION");
    EXPECT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Atomic>>(bin->lhs));
    EXPECT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Atomic>>(bin->rhs));
}

TEST_F(PropositionalParserTest, PrecedenceUnaryBindsTighterThanConjunction) {
    auto res = parse_propositional("~p & q", ctx, config);
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Binary>>(*res));
    const auto& bin = std::get<std::unique_ptr<LogicAST::Binary>>(*res);
    EXPECT_EQ(bin->op.name, "CONJUNCTION");
    EXPECT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Unary>>(bin->lhs));
    EXPECT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Atomic>>(bin->rhs));
}

TEST_F(PropositionalParserTest, PrecedenceConjunctionDisjunction) {
    auto res = parse_propositional("p & q | r", ctx, config);
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Binary>>(*res));
    const auto& bin = std::get<std::unique_ptr<LogicAST::Binary>>(*res);
    EXPECT_EQ(bin->op.name, "DISJUNCTION"); // parsed as (p & q) | r
    EXPECT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Binary>>(bin->lhs));
    EXPECT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Atomic>>(bin->rhs));
}

TEST_F(PropositionalParserTest, AssociativityConditionalIsRightAssociative) {
    auto res = parse_propositional("p -> q -> r", ctx, config);
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Binary>>(*res));
    const auto& bin = std::get<std::unique_ptr<LogicAST::Binary>>(*res);
    EXPECT_EQ(bin->op.name, "CONDITIONAL"); // parsed as p -> (q -> r)
    EXPECT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Atomic>>(bin->lhs));
    EXPECT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Binary>>(bin->rhs));
}

TEST_F(PropositionalParserTest, ParenthesizedExpressionsOverridePrecedence) {
    auto res = parse_propositional("~(p & q)", ctx, config);
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Unary>>(*res));
    const auto& unary = std::get<std::unique_ptr<LogicAST::Unary>>(*res);
    EXPECT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Binary>>(unary->scope));
}

TEST_F(PropositionalParserTest, SyntaxError) {
    auto res = parse_propositional("p &", ctx, config);
    EXPECT_FALSE(res.has_value());
}

TEST_F(PropositionalParserTest, LexicalErrorUnexpectedCharacter) {
    auto res = parse_propositional("p @ q", ctx, config);
    EXPECT_FALSE(res.has_value());
}

TEST_F(PropositionalParserTest, LexicalErrorUtf8Character) {
    auto res = parse_propositional("p ★ q", ctx, config);
    EXPECT_FALSE(res.has_value());
}

TEST_F(PropositionalParserTest, TwoConditionalsWithEqualPrecedence) {
    ParserConfig local_config = config;
    // Map alternative conditional symbol ">" to "CONDITIONAL_ALT"
    local_config.operator_mappings[">"] = "CONDITIONAL_ALT";
    // Give it the same precedence and associativity as CONDITIONAL
    local_config.operator_properties["CONDITIONAL_ALT"] = { Arity::BINARY, 2, Associativity::RIGHT };

    auto res = parse_propositional("p -> q > r", ctx, local_config);
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Binary>>(*res));
    const auto& bin = std::get<std::unique_ptr<LogicAST::Binary>>(*res);
    EXPECT_EQ(bin->op.name, "CONDITIONAL"); // parsed as p -> (q > r) because both are right-associative and equal precedence
    EXPECT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Atomic>>(bin->lhs));
    
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Binary>>(bin->rhs));
    const auto& sub_bin = std::get<std::unique_ptr<LogicAST::Binary>>(bin->rhs);
    EXPECT_EQ(sub_bin->op.name, "CONDITIONAL_ALT");
    EXPECT_EQ(std::get<std::unique_ptr<LogicAST::Atomic>>(sub_bin->lhs)->literal, "q");
    EXPECT_EQ(std::get<std::unique_ptr<LogicAST::Atomic>>(sub_bin->rhs)->literal, "r");
}

class FirstOrderParserTest : public ::testing::Test {
protected:
    LogicAST::ParseContext ctx;
    ParserConfig config = ParserConfig::create_standard_logic_config();

    void SetUp() override {
        ctx.term_types.register_type("CONSTANT");
        ctx.term_types.register_type("VARIABLE");
        ctx.variable_tag = "VARIABLE";
        ctx.operator_names.register_type("NEGATION");
        ctx.operator_names.register_type("CONJUNCTION");
        ctx.quantifier_names.register_type("UNIVERSAL");
        ctx.quantifier_names.register_type("EXISTENTIAL");

        config.default_constant_type = "CONSTANT";
    }
};

TEST_F(FirstOrderParserTest, PredicationWithArguments) {
    auto res = parse_first_order("P(x, c)", ctx, config);
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Predication>>(*res));
    const auto& pred = std::get<std::unique_ptr<LogicAST::Predication>>(*res);
    EXPECT_EQ(pred->predicate, "P");
    ASSERT_EQ(pred->arguments.size(), 2);
    EXPECT_EQ(pred->arguments[0].literal, "x");
    EXPECT_EQ(pred->arguments[0].type, "VARIABLE");
    EXPECT_EQ(pred->arguments[1].literal, "c");
    EXPECT_EQ(pred->arguments[1].type, "CONSTANT");
}

TEST_F(FirstOrderParserTest, PredicationWithoutArguments) {
    auto res = parse_first_order("P", ctx, config);
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Predication>>(*res));
    const auto& pred = std::get<std::unique_ptr<LogicAST::Predication>>(*res);
    EXPECT_EQ(pred->predicate, "P");
    EXPECT_TRUE(pred->arguments.empty());
}

TEST_F(FirstOrderParserTest, Identity) {
    auto res = parse_first_order("x = c", ctx, config);
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Identity>>(*res));
    const auto& id = std::get<std::unique_ptr<LogicAST::Identity>>(*res);
    EXPECT_EQ(id->lhs.literal, "x");
    EXPECT_EQ(id->lhs.type, "VARIABLE");
    EXPECT_EQ(id->rhs.literal, "c");
    EXPECT_EQ(id->rhs.type, "CONSTANT");
}

TEST_F(FirstOrderParserTest, QuantifiersBindNonGreedily) {
    // "forall x P(x) & Q" should be parsed as "(forall x P(x)) & Q"
    auto res = parse_first_order("forall x P(x) & Q", ctx, config);
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Binary>>(*res));
    const auto& bin = std::get<std::unique_ptr<LogicAST::Binary>>(*res);
    EXPECT_EQ(bin->op.name, "CONJUNCTION");
    EXPECT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Quantification>>(bin->lhs));
    EXPECT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Predication>>(bin->rhs));

    const auto& quant = std::get<std::unique_ptr<LogicAST::Quantification>>(bin->lhs);
    EXPECT_EQ(quant->quantifier.name, "UNIVERSAL");
    EXPECT_EQ(quant->variable.literal, "x");
    EXPECT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Predication>>(quant->scope));
}

TEST_F(FirstOrderParserTest, NestedQuantifiers) {
    auto res = parse_first_order("forall x exists y P(x, y)", ctx, config);
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Quantification>>(*res));
    const auto& q1 = std::get<std::unique_ptr<LogicAST::Quantification>>(*res);
    EXPECT_EQ(q1->quantifier.name, "UNIVERSAL");
    EXPECT_EQ(q1->variable.literal, "x");
    
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Quantification>>(q1->scope));
    const auto& q2 = std::get<std::unique_ptr<LogicAST::Quantification>>(q1->scope);
    EXPECT_EQ(q2->quantifier.name, "EXISTENTIAL");
    EXPECT_EQ(q2->variable.literal, "y");
    EXPECT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Predication>>(q2->scope));
}

TEST_F(FirstOrderParserTest, AttemptingToUseAtomicInFolModeShouldFail) {
    // Standalone identifier in FOL is Predication of 0-arity, not Atomic.
    auto res = parse_first_order("P", ctx, config);
    ASSERT_TRUE(res.has_value());
    EXPECT_FALSE(std::holds_alternative<std::unique_ptr<LogicAST::Atomic>>(*res));
    EXPECT_TRUE(std::holds_alternative<std::unique_ptr<LogicAST::Predication>>(*res));
}

TEST_F(FirstOrderParserTest, TrailingCommaInPredication) {
    auto res = parse_first_order("P(x, )", ctx, config);
    EXPECT_FALSE(res.has_value());
}


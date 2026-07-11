#pragma once

#include <string>
#include <unordered_map>

namespace iif_sadaf::talk::LogicParser {
    enum class Associativity {
        LEFT,
        RIGHT,
        NONE
    };

    enum class Arity {
        UNARY,
        BINARY
    };

    struct OperatorProperties {
        Arity arity;
        int precedence;
        Associativity associativity;
    };

    struct ParserConfig {
        // Maps operator literals (e.g. "~", "&", "->") to their semantic names in ParseContext (e.g. "NEGATION", "CONJUNCTION")
        std::unordered_map<std::string, std::string> operator_mappings;

        // Maps quantifier literals (e.g. "forall", "exists") to their semantic names (e.g. "UNIVERSAL", "EXISTENTIAL")
        std::unordered_map<std::string, std::string> quantifier_mappings;

        // Maps operator semantic names to their parsing properties (arity, precedence, associativity)
        std::unordered_map<std::string, OperatorProperties> operator_properties;

        // Regex pattern to identify variable terms (e.g., x, y, z, x_1)
        std::string variable_pattern = "^[x-z](_\\d+)?$";

        // Regex pattern to identify constant terms and predicate names (default matching standard alphanumeric identifiers)
        std::string constant_pattern = "^[a-wA-Z]\\w*$";

        // Default term type for constants/non-variables (if empty, we will try to deduce it from the ParseContext)
        std::string default_constant_type = "";

        // Helper to generate a standard configuration
        static ParserConfig create_standard_logic_config() {
            ParserConfig config;

            // Standard operator mappings
            config.operator_mappings["~"] = "NEGATION";
            config.operator_mappings["¬"] = "NEGATION";
            config.operator_mappings["!"] = "NEGATION";

            config.operator_mappings["&"] = "CONJUNCTION";
            config.operator_mappings["&&"] = "CONJUNCTION";
            config.operator_mappings["∧"] = "CONJUNCTION";

            config.operator_mappings["|"] = "DISJUNCTION";
            config.operator_mappings["||"] = "DISJUNCTION";
            config.operator_mappings["∨"] = "DISJUNCTION";

            config.operator_mappings["->"] = "CONDITIONAL";
            config.operator_mappings["→"] = "CONDITIONAL";

            config.operator_mappings["<->"] = "BICONDITIONAL";
            config.operator_mappings["↔"] = "BICONDITIONAL";

            // Standard quantifier mappings
            config.quantifier_mappings["forall"] = "UNIVERSAL";
            config.quantifier_mappings["exists"] = "EXISTENTIAL";
            config.quantifier_mappings["∀"] = "UNIVERSAL";
            config.quantifier_mappings["∃"] = "EXISTENTIAL";

            // Standard operator properties
            // Precedence levels: Unary (4) > Conjunction/Disjunction (3) > Conditional (2) > Biconditional (1)
            config.operator_properties["NEGATION"] = { Arity::UNARY, 4, Associativity::RIGHT };
            config.operator_properties["CONJUNCTION"] = { Arity::BINARY, 3, Associativity::LEFT };
            config.operator_properties["DISJUNCTION"] = { Arity::BINARY, 3, Associativity::LEFT };
            config.operator_properties["CONDITIONAL"] = { Arity::BINARY, 2, Associativity::RIGHT };
            config.operator_properties["BICONDITIONAL"] = { Arity::BINARY, 1, Associativity::LEFT};

            return config;
        }
    };
} // namespace iif_sadaf::talk::LogicParser


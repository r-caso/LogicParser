# LogicParser

A flexible, high-performance, and fully configurable C++23 parser library for propositional and first-order logic formulas. This library parses input strings into a `LogicAST` representation (automatically retrieved via CMake).

## Features

- **Modern C++23 Standard**: Utilizes modern C++ features, including `std::expected` for type-safe value-or-error propagation (avoiding exceptions for normal parse errors).
- **Dynamic Pratt Parsing**: Fully configurable operator precedence and associativity resolved at runtime. This allows callers to register custom operators and supports having multiple operators of the same type with equal precedence (e.g. counterfactual vs. material conditionals).
- **Non-Greedy Quantifiers**: Quantifiers bind tightly (similar to unary operators), ensuring they do not greedily consume the remainder of the formula. For example, `forall x P(x) & Q` parses as `(forall x P(x)) & Q` rather than `forall x (P(x) & Q)`.
- **Flexible Identifier Matching**: Customizable regex patterns to distinguish variable terms from constant terms and predicates.
- **Separate Language Modes**:
  - **Propositional Mode**: Restricts the syntax to use only `Atomic` propositions, `Unary` operators, and `Binary` operators.
  - **First-Order Mode**: Restricts the syntax to exclude `Atomic` propositions, using `Identity` (`t1 = t2`), `Predication` (`P(t1, ..., tn)`), `Unary` operators, `Binary` operators, and `Quantification`.

---

## Codebase Map

- **Headers**:
  - [parser.hpp](include/LogicParser/parser.hpp): The main library entry point exposing `parse_propositional` and `parse_first_order`.
  - [parser_config.hpp](include/LogicParser/parser_config.hpp): Configuration structure for operator properties (precedence, associativity, arity), quantifier mappings, and term regexes.
  - [lexer.hpp](include/LogicParser/lexer.hpp): Lexer declarations.
  - [token.hpp](include/LogicParser/token.hpp): Token structures and categories.
- **Source**:
  - [parser.cpp](src/parser.cpp): Implementation of the precedence climbing parser.
  - [lexer.cpp](src/lexer.cpp): Implementation of the maximal-munch tokenizer.
- **Tests**:
  - [parser_tests.cpp](tests/parser_tests.cpp): Complete suite of unit tests.

---

## Prerequisites

This library requires [r-caso/LogicAST](https://github.com/r-caso/LogicAST), which should be provided separately.

---

## Build and Installation Instructions

This project is configured using **CMake 3.24+** and requires a compiler supporting **C++23** (e.g. MSVC 19.30+, GCC 13+, Clang 16+).

### 1. Build

#### Manually

```bash
# Configure CMake
cmake -B out -S . -DCMAKE_PREFIX_PATH="/path/to/LogicAST"

# Compile
cmake --build out 
```

#### Using CMake presets

In [CMakePresets.json](CMakePresets.json), there is support for different platforms:

- Windows
- MacOS (arm and x64)
- Linux
- FreeBSD

To compile, e.g. with Linux in a debug build:

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
```

Notice that this requires that LogicAST be available as a system library. If you don't want to install it that way, you can generate your own presets, following the template in [CMakeUserPresets.json.example](CMakeUserPresets.json.example): just copy this file to `CMakeUserPresets.json`, and adapt to your own system.

### 2. Install

You can install `LogicParser` into a local directory or system path. The install step generates CMake export targets (`LogicParserTargets.cmake`) and package versioning helpers to allow seamless integration into other projects via `find_package(LogicParser)`.

#### Manually

```bash
cmake --install out --prefix <your-install-directory>
```

This installs:
- The compiled binary library target under `lib/`.
- The library public headers under `include/LogicParser/`.
- The `LogicAST` dependency headers under `include/LogicAST/`.
- The CMake config and version files for both libraries under `lib/cmake/`.

#### If using CMake presets

Presets generate code under `out/build/{PLATFORM}-{BUILD_TYPE}`. To install from e.g. a linux debug preset:

```bash
cmake --install out/build/linux-debug
```

---

## Running the Unit Tests

### Manually

To run the unit tests when building as a top-level project:

```bash
out/Release/LogicParserTests.exe
```

### If using CMake presets

Once again, with Linux debug as example:

```bash
ctest --preset linux-debug
```

The tests cover:
- Propositional logic syntax, operator precedence, and associativity.
- Parenthesized expressions.
- First-order logic predicates, arity, and term variable/constant classification.
- First-order identities (`t1 = t2`).
- Non-greedy quantifier binding rules.
- Custom equal-precedence operators (e.g., matching standard `->` conditional and alternative conditional `>` with the same precedence).

---

## Usage Example

### Parsing Propositional Logic

```cpp
#include "LogicAST/LogicAST.hpp"
#include "LogicParser/parser.hpp"

// 1. Define standard ParseContext
iif_sadaf::talk::LogicAST::ParseContext ctx;
ctx.operator_names.register_type("NEGATION");
ctx.operator_names.register_type("CONJUNCTION");

// 2. Generate standard operator configurations
auto config = iif_sadaf::talk::LogicParser::ParserConfig::create_standard_logic_config();

// 3. Parse formula
auto result = iif_sadaf::talk::LogicParser::parse_propositional("~p & q", ctx, config);

if (result) {
    // result.value() holds the AST Expression
} else {
    std::cerr << "Parse error: " << result.error() << std::endl;
}
```

### Parsing First-Order Logic with Custom Operators

```cpp
#include "LogicAST/LogicAST.hpp"
#include "LogicParser/parser.hpp"

// Setup context
iif_sadaf::talk::LogicAST::ParseContext ctx;
ctx.term_types.register_type("CONSTANT");
ctx.term_types.register_type("VARIABLE");
ctx.variable_tag = "VARIABLE";
ctx.quantifier_names.register_type("UNIVERSAL");
ctx.operator_names.register_type("CONDITIONAL_1");
ctx.operator_names.register_type("CONDITIONAL_2");

// Setup config
iif_sadaf::talk::LogicParser::ParserConfig config;
config.quantifier_mappings["forall"] = "UNIVERSAL";
config.default_constant_type = "CONSTANT";

// Map custom operators
config.operator_mappings["->"] = "CONDITIONAL_1";
config.operator_mappings[">"] = "CONDITIONAL_2";

// Configure both with equal precedence (2) and right-associativity
config.operator_properties["CONDITIONAL_1"] = { Arity::Binary, 2, Associativity::Right };
config.operator_properties["CONDITIONAL_2"] = { Arity::Binary, 2, Associativity::Right };

// Parse (parsed as forall x P(x) -> (Q(x) > R(x)))
auto result = iif_sadaf::talk::LogicParser::parse_first_order("forall x P(x) -> Q(x) > R(x)", ctx, config);
```

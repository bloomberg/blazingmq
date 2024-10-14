// Based on an example by Krzysztof Narkiewicz
// (https://github.com/ezaquarii/bison-flex-cpp-example,
// <krzysztof.narkiewicz@ezaquarii.com>, {BOSS  144538})

%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.0"
%defines
%define api.parser.class { SimpleEvaluatorParser }

%define api.token.constructor
%define api.value.type variant
%define parse.assert
%define api.namespace { BloombergLP::bmqeval }
%code requires
{
    #include <bsl_functional.h>
    #include <string>
    #include <bmqeval_simpleevaluator.h>

    namespace BloombergLP {
      namespace bmqeval {
        class SimpleEvaluatorScanner;
      }
    }
}

// Bison calls yylex() function that must be provided by us to suck tokens
// from the scanner. This block will be placed at the beginning of IMPLEMENTATION file (cpp).
// We define this function here (function! not method).
// This function is called only inside Bison, so we make it static to limit symbol visibility for the linker
// to avoid potential linking conflicts.
%code top
{
    #include <bslmf_movableref.h>

    #include <bmqeval_simpleevaluator.h>
    #include <bmqeval_simpleevaluatorscanner.h>
    #include <bmqeval_simpleevaluatorparser.hpp> // generated

    using namespace BloombergLP::bmqeval;

    // yylex() arguments are defined in parser.y
    static SimpleEvaluatorParser::symbol_type yylex(SimpleEvaluatorScanner &scanner) {
        return scanner.get_next_token();
    }

    // you can accomplish the same thing by inlining the code using preprocessor
    // x and y are same as in above static function
    // #define yylex(x, y) scanner.get_next_token()

}

%{
    bsl::ostream* d_os;
%}

%lex-param { SimpleEvaluatorScanner& scanner }
%parse-param { SimpleEvaluatorScanner& scanner }
%parse-param { CompilationContext& ctx }
%define parse.trace
%define parse.error verbose

%define api.token.prefix {TOKEN_}

%token END 0 "end of expression"
%token <char> INVALID "invalid character"
%token <bsl::string> OVERFLOW "overflow"
%token <bsl::string> PROPERTY "property";
%token <bsl::string> STRING "string";
%token <uint64_t> INTEGER "integer";
%token TRUE "true";
%token FALSE "false";
%token LPAR "(";
%token RPAR ")";
%token PLUS "+" MINUS "-";
%token TIMES "*" DIVIDES "/" MODULUS "%";
%token EQ "=" NE "<>" LT "<" LE "<=" GE ">=" GT ">";

%left OR;
%left AND;
%left EQ NE
%left LT LE GE GT;
%left PLUS MINUS;
%left TIMES DIVIDES MODULUS;
%right NOT "!";

%type<SimpleEvaluator::ExpressionPtr> expression;
%type<SimpleEvaluator::ExpressionPtr> predicate;
%start predicate

%%

predicate : expression END
        {
            ctx.d_expression = $1;
        }

expression
    : PROPERTY
        {

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) \
&& defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
            $$ = ctx.makeUnaryExpression<SimpleEvaluator::Property, bsl::string>(bsl::move($1));
#else
            $$ = ctx.makeUnaryExpression<SimpleEvaluator::Property, bsl::string>($1);
#endif

            ++ctx.d_numProperties;
        }
    | INTEGER
        { $$ = ctx.makeLiteral<SimpleEvaluator::IntegerLiteral>($1); }
    | OVERFLOW
        {
            ctx.d_os << "integer overflow at offset " << scanner.lastTokenLocation();
            YYABORT;
        }
    | STRING
        { $$ = ctx.makeLiteral<SimpleEvaluator::StringLiteral>($1); }
    | TRUE
        { $$ = ctx.makeLiteral<SimpleEvaluator::BooleanLiteral>(true); }
    | FALSE
        { $$ = ctx.makeLiteral<SimpleEvaluator::BooleanLiteral>(false); }
    | expression EQ expression
        { $$ = ctx.makeComparison<bsl::equal_to>($1, $3); }
    | expression NE expression
        { $$ = ctx.makeComparison<bsl::not_equal_to>($1, $3); }
    | expression GT expression
        { $$ = ctx.makeComparison<bsl::greater>($1, $3); }
    | expression GE expression
        { $$ = ctx.makeComparison<bsl::greater_equal>($1, $3); }
    | expression LT expression
        { $$ = ctx.makeComparison<bsl::less>($1, $3); }
    | expression LE expression
        { $$ = ctx.makeComparison<bsl::less_equal>($1, $3); }
    | expression AND expression
        { $$ = ctx.makeBooleanBinaryExpression<SimpleEvaluator::And>($1, $3); }
    | expression OR expression
        { $$ = ctx.makeBooleanBinaryExpression<SimpleEvaluator::Or>($1, $3); }
    | NOT expression
        { $$ = ctx.makeUnaryExpression<SimpleEvaluator::Not>($2); }
    | expression PLUS expression
        { $$ = ctx.makeNumBinaryExpression<bsl::plus>($1, $3); }
    | expression MINUS expression
        { $$ = ctx.makeNumBinaryExpression<bsl::minus>($1, $3); }
    | expression TIMES expression
        { $$ = ctx.makeNumBinaryExpression<bsl::multiplies>($1, $3); }
    | expression DIVIDES expression
        { $$ = ctx.makeNumBinaryExpression<bsl::divides>($1, $3); }
    | expression MODULUS expression
        { $$ = ctx.makeNumBinaryExpression<bsl::modulus>($1, $3); }
    | MINUS %prec NOT expression
        { $$ = ctx.makeUnaryExpression<SimpleEvaluator::UnaryMinus>($2); }
    | LPAR expression RPAR
        { $$ = $2; }

%%

// Bison expects us to provide implementation - otherwise linker complains.
// We cannot use 'bsl::string' here because bison interface expects exactly
// 'std::string', but it's okay.
void SimpleEvaluatorParser::error(const std::string& message) {
    ctx.d_os << message << " at offset " << scanner.lastTokenLocation();
}

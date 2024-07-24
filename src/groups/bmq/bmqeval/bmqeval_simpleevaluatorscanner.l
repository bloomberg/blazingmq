/* Based on an example by Krzysztof Narkiewicz
(https://github.com/ezaquarii/bison-flex-cpp-example,
<krzysztof.narkiewicz@ezaquarii.com>, {BOSS  144538})
*/

%{
    #include <bmqeval_simpleevaluator.h>
    #include <bmqeval_simpleevaluatorparser.hpp>
    #include <bmqeval_simpleevaluatorscanner.h>

	using namespace BloombergLP::bmqeval;

	#define yyterminate() SimpleEvaluatorParser::make_END();
%}

%option nodefault
%option noyywrap
%option c++
%option yyclass="SimpleEvaluatorScanner"
%option prefix="BloombergLP_bmqeval_simpleevaluator_"

%%


"true" {
	updatePosition();
	return SimpleEvaluatorParser::make_TRUE();
}

"false" {
	updatePosition();
	return SimpleEvaluatorParser::make_FALSE();
}

[a-zA-Z][a-zA-Z0-9_]* {
	updatePosition();
	return SimpleEvaluatorParser::make_PROPERTY(yytext);
}

-?[0-9]+ {
	updatePosition();
	try {
	    // There is a problem with parsing Int64 on Solaris with string
	    // streams: fail flag is not set when overflow expected.  'bsl::stoll'
	    // is used as a workaround.
	    bsls::Types::Int64 value = bsl::stoll(yytext);
        return SimpleEvaluatorParser::make_INTEGER(value);
	} catch (...) {
	    // 'bsl::out_of_range' or 'bsl::invalid_argument' might be thrown, make
	    // overflow for simplicity for both cases
        return SimpleEvaluatorParser::make_OVERFLOW(yytext);
	}
}

\"([^\\\"]|\\.)*\" {
	updatePosition();
	return SimpleEvaluatorParser::make_STRING(bsl::string(yytext + 1, strlen(yytext) - 2));
}

\( {
	updatePosition();
	return SimpleEvaluatorParser::make_LPAR();
}

\) {
	updatePosition();
	return SimpleEvaluatorParser::make_RPAR();
}

\|\| {
	updatePosition();
	return SimpleEvaluatorParser::make_OR();
}

&& {
	updatePosition();
	return SimpleEvaluatorParser::make_AND();
}

== {
	updatePosition();
	return SimpleEvaluatorParser::make_EQ();
}

"!=" {
	updatePosition();
	return SimpleEvaluatorParser::make_NE();
}

"<" {
	updatePosition();
	return SimpleEvaluatorParser::make_LT();
}

"<=" {
	updatePosition();
	return SimpleEvaluatorParser::make_LE();
}

">" {
	updatePosition();
	return SimpleEvaluatorParser::make_GT();
}

">=" {
	updatePosition();
	return SimpleEvaluatorParser::make_GE();
}

[!~] {
	updatePosition();
	return SimpleEvaluatorParser::make_NOT();
}

"+" {
	updatePosition();
	return SimpleEvaluatorParser::make_PLUS();
}

"-" {
	updatePosition();
	return SimpleEvaluatorParser::make_MINUS();
}

"*" {
	updatePosition();
	return SimpleEvaluatorParser::make_TIMES();
}

"/" {
	updatePosition();
	return SimpleEvaluatorParser::make_DIVIDES();
}

"%" {
	updatePosition();
	return SimpleEvaluatorParser::make_MODULUS();
}

[ \t\n] {
	updatePosition();
}

. {
	updatePosition();
	return SimpleEvaluatorParser::make_INVALID(*yytext);
}

<<EOF>> {
	return yyterminate();
}


%%

// Copyright 2023 Bloomberg Finance L.P.
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// bmqeval_simpleevaluator.cpp                                        -*-C++-*-
#include <bmqeval_simpleevaluator.h>

#include <bmqeval_simpleevaluatorparser.hpp>
#include <bmqeval_simpleevaluatorscanner.h>

namespace BloombergLP {
namespace bmqeval {

// ----------------------
// class PropertiesReader
// ----------------------

// CREATORS
PropertiesReader::~PropertiesReader()
{
    // NOTHING
}

// ---------------------
// class SimpleEvaluator
// ---------------------

SimpleEvaluator::SimpleEvaluator()
: d_expression(0)
, d_isCompiled(false)
{
    // NOTHING
}

SimpleEvaluator::~SimpleEvaluator()
{
    // NOTHING
}

int SimpleEvaluator::compile(const bsl::string&  expression,
                             CompilationContext& context)
{
    context.d_validationOnly = false;
    parse(expression, context);

    if (context.hasError()) {
        d_expression.reset();
    }
    else {
        d_expression = context.d_expression;
    }
    d_isCompiled = true;

    return context.lastError();
}

bool SimpleEvaluator::validate(const bsl::string&  expression,
                               CompilationContext& context)
{
    context.d_validationOnly = true;
    parse(expression, context);

    return !context.hasError();
}

void SimpleEvaluator::parse(const bsl::string&  expression,
                            CompilationContext& context)
{
    context.d_lastError     = ErrorType::e_OK;
    context.d_numOperators  = 0;
    context.d_numProperties = 0;
    context.d_os.reset();

    bsl::istringstream     is(expression, context.d_allocator);
    SimpleEvaluatorScanner scanner;
    scanner.switch_streams(&is, &context.d_os);

    SimpleEvaluatorParser parser(scanner, context);

    if (parser.parse() != 0) {
        context.d_lastError = ErrorType::e_SYNTAX;

        return;  // RETURN
    }

    if (context.d_numOperators > k_MAX_OPERATORS) {
        context.d_os << "too many operators";
        context.d_lastError = ErrorType::e_TOO_COMPLEX;

        return;  // RETURN
    }

    if (context.d_numProperties == 0) {
        context.d_os << "expression does not use any properties";
        context.d_lastError = ErrorType::e_NO_PROPERTIES;

        return;  // RETURN
    }

    if (context.d_numProperties > k_MAX_PROPERTIES) {
        context.d_os << "expression uses too many properties";
        context.d_lastError = ErrorType::e_TOO_COMPLEX;

        return;  // RETURN
    }
}

bool SimpleEvaluator::evaluate(EvaluationContext& context) const
{
    BSLS_ASSERT_SAFE(d_expression.get());
    BSLS_ASSERT_SAFE(context.d_propertiesReader);

    context.reset();

    bdld::Datum value = d_expression->evaluate(context);

    if (context.d_stop) {
        return false;  // RETURN
    }

    if (!value.isBoolean()) {
        context.d_stop      = true;
        context.d_lastError = ErrorType::e_TYPE;

        return false;  // RETURN
    }

    return value.theBoolean();
}

// ---------------------------------
// class SimpleEvaluator::Expression
// ---------------------------------

SimpleEvaluator::Expression::~Expression()
{
    // NOTHING
}

// -------------------------------
// class SimpleEvaluator::Property
// -------------------------------

SimpleEvaluator::Property::Property(const bsl::string& name)
: d_name(name)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
SimpleEvaluator::Property::Property(bsl::string&& name) noexcept
: d_name(bsl::move(name))
{
}
#endif

bdld::Datum
SimpleEvaluator::Property::evaluate(EvaluationContext& context) const
{
    bdld::Datum value = context.d_propertiesReader->get(d_name,
                                                        context.d_allocator);

    if (value.isError()) {
        context.d_stop = true;
        int rc         = value.theError().code();

        if (rc >= ErrorType::e_EVALUATION_FIRST &&
            ErrorType::e_EVALUATION_LAST <= rc) {
            context.d_lastError = static_cast<ErrorType::Enum>(rc);
        }
        else {
            context.d_lastError = ErrorType::e_UNDEFINED;
        }
    }

    return value;
}

// -------------------------------------
// class SimpleEvaluator::IntegerLiteral
// -------------------------------------

bdld::Datum
SimpleEvaluator::IntegerLiteral::evaluate(EvaluationContext& context) const
{
    // It looks like we could save a bit of time by storing a *datum* in
    // 'd_value', but it would have to use the compile-time allocator. We don't
    // want to require that that allocator is still alive at runtime.
    return bdld::Datum::createInteger64(d_value, context.d_allocator);
}

// -------------------------------------
// class SimpleEvaluator::BooleanLiteral
// -------------------------------------

bdld::Datum
SimpleEvaluator::BooleanLiteral::evaluate(EvaluationContext& context) const
{
    return bdld::Datum::createBoolean(d_value);
}

// ---------------------------------
// class SimpleEvaluator::UnaryMinus
// ---------------------------------

bdld::Datum
SimpleEvaluator::UnaryMinus::evaluate(EvaluationContext& context) const
{
    bdld::Datum expr = d_expression->evaluate(context);

    if (context.d_stop) {
        return bdld::Datum::createNull();  // RETURN
    }

    bsls::Types::Int64 value;

    if (expr.isInteger64()) {
        value = expr.theInteger64();
    }
    else if (expr.isInteger()) {
        value = expr.theInteger();
    }
    else {
        context.d_lastError = ErrorType::e_TYPE;

        return context.stop();  // RETURN
    }

    return bdld::Datum::createInteger64(-value, context.d_allocator);
}

// ------------------------------------
// class SimpleEvaluator::StringLiteral
// ------------------------------------

SimpleEvaluator::StringLiteral::StringLiteral(const bsl::string& value)
: d_value(value)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
SimpleEvaluator::StringLiteral::StringLiteral(bsl::string&& value) noexcept
: d_value(bsl::move(value))
{
}
#endif

bdld::Datum
SimpleEvaluator::StringLiteral::evaluate(EvaluationContext& context) const
{
    // Same remark as above wrt allocators.

    return bdld::Datum::createStringRef(d_value.data(),
                                        d_value.length(),
                                        context.d_allocator);
}

// -------------------------
// class SimpleEvaluator::Or
// -------------------------

bdld::Datum SimpleEvaluator::Or::evaluate(EvaluationContext& context) const
{
    bdld::Datum left = d_left->evaluate(context);

    if (context.d_stop) {
        return bdld::Datum::createNull();  // RETURN
    }

    if (!left.isBoolean()) {
        context.d_lastError = ErrorType::e_TYPE;
        return context.stop();  // RETURN
    }

    if (left.theBoolean()) {
        return bdld::Datum::createBoolean(true);  // RETURN
    }

    bdld::Datum right = d_right->evaluate(context);

    if (context.d_stop) {
        return bdld::Datum::createNull();  // RETURN
    }

    if (!right.isBoolean()) {
        context.d_lastError = ErrorType::e_TYPE;
        return context.stop();  // RETURN
    }

    return right;
}

// --------------------------
// class SimpleEvaluator::And
// --------------------------

bdld::Datum SimpleEvaluator::And::evaluate(EvaluationContext& context) const
{
    bdld::Datum left = d_left->evaluate(context);

    if (context.d_stop) {
        return bdld::Datum::createNull();  // RETURN
    }

    if (!left.isBoolean()) {
        context.d_lastError = ErrorType::e_TYPE;
        return context.stop();  // RETURN
    }

    if (!left.theBoolean()) {
        return bdld::Datum::createBoolean(false);  // RETURN
    }

    bdld::Datum right = d_right->evaluate(context);

    if (context.d_stop) {
        return bdld::Datum::createNull();  // RETURN
    }

    if (!right.isBoolean()) {
        context.d_lastError = ErrorType::e_TYPE;
        return context.stop();  // RETURN
    }

    return right;
}

// --------------------------
// class SimpleEvaluator::Not
// --------------------------

bdld::Datum SimpleEvaluator::Not::evaluate(EvaluationContext& context) const
{
    bdld::Datum value = d_expression->evaluate(context);

    if (context.d_stop) {
        return bdld::Datum::createNull();  // RETURN
    }

    if (!value.isBoolean()) {
        context.d_lastError = ErrorType::e_TYPE;

        return context.stop();  // RETURN
    }

    return bdld::Datum::createBoolean(!value.theBoolean());
}

}  // close package namespace
}  // close enterprise namespace

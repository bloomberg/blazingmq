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

// bmqeval_simpleevaluator.h                                          -*-C++-*-
#ifndef INCLUDED_BMQEVAL_SIMPLEEVALUATOR
#define INCLUDED_BMQEVAL_SIMPLEEVALUATOR

//@PURPOSE: Provide mechanism for evaluating simple logical expressions.
//
//@CLASSES:
//  SimpleEvaluator: Mechanism for validating, compiling, and evaluating
//  logical expressions.
//  PropertiesReader: Interface for obtaining property values given their
//  names.
//  CompilationContext: Contains data used during parsing.
//  EvaluationContext: Contains data used during evaluation.
//
//@DESCRIPTION: 'SimpleEvaluator' handles expression evaluation.
//
/// Thread Safety
///-------------
//: o SimpleEvaluator is thread safe
//: o PropertiesReader is NOT thread safe
//: o CompilationContext is NOT thread safe
//: o EvaluationContext is NOT thread safe
//
/// Basic Usage Example
///-------------------
//
// SimpleEvaluator evaluator;
// bsl::string     expression = "foo < 3 | bar > 4";
//
// CompilationContext compilationContext(s_allocator_p);
//
// if (int rc = evaluator.compile(expression, compilationContext)) {
//     bsl::cerr << compilationContext.lastErrorMessage() << "\n";
//     // don't use evaluator
// }
//
// MockPropertiesReader reader(s_allocator_p);
// EvaluationContext evaluationContext(&reader, s_allocator_p);
// bool result = evaluator.evaluate(evaluationContext);
//..

// BDE
#include <bdld_datum.h>
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_issame.h>
#include <bsls_types.h>

// MWC
#include <mwcu_memoutstream.h>

namespace BloombergLP {

namespace bmqeval {

// FORWARD DECLARATIONS
class CompilationContext;
class EvaluationContext;

struct ErrorType {
    enum Enum {
        e_OK = 0  // no error
        ,
        e_COMPILATION_FIRST = -100,
        e_SYNTAX            = -100  // syntax error
        ,
        e_NO_PROPERTIES = -101  // expression does not use any property
        ,
        e_TOO_COMPLEX = -102  // too many properties
                              // or operators in expression
        ,
        e_COMPILATION_LAST = -102,
        e_EVALUATION_FIRST = -1,
        e_NAME             = -1,
        e_TYPE             = -2,
        e_BINARY           = -3,
        e_UNDEFINED        = -4,
        e_EVALUATION_LAST  = -4
    };

    /// Return the non-modifiable string error description corresponding to
    /// the specified enumeration `value`.
    static const char* toString(ErrorType::Enum value);
};

// ======================
// class PropertiesReader
// ======================

/// Interface for reading message properties.
class PropertiesReader {
  public:
    // CREATORS

    /// Destroy this object.
    virtual ~PropertiesReader();

    // MANIPULATORS

    /// Return a `bdld::Datum` object with value for the specified `name`.
    /// Use the specified `allocator` for any memory allocation.
    virtual bdld::Datum get(const bsl::string& name,
                            bslma::Allocator*  allocator) = 0;
};

// =====================
// class SimpleEvaluator
// =====================

/// Evaluator for boolean expressions based on message properties.
class SimpleEvaluator {
  private:
    // PRIVATE TYPES

    // ----------
    // Expression
    // ----------

    /// Private abstract base class, from which all Expression classes are
    /// derived.
    class Expression {
      public:
        virtual ~Expression();

        /// Evaluate a Expression.
        virtual bdld::Datum evaluate(EvaluationContext& context) const = 0;
    };

    // Bison generates different code for different available standards:
    // - For C++ >= 11 it generates the 'emplace' with a move constructor, so
    // it's possible to use 'bslma::ManagedPtr'.
    // - For C++ < 11 (AIX/Solaris) it generates the 'emplace' with a const&
    // copy constructor, but this constructor is deleted for
    // 'bslma::ManagedPtr', so it's impossible to use here. Use
    // 'bsl::shared_ptr' instead.
#if 201103L <= BSLS_COMPILERFEATURES_CPLUSPLUS
    typedef bslma::ManagedPtr<Expression> ExpressionPtr;
#else
    typedef bsl::shared_ptr<Expression> ExpressionPtr;
#endif

    // --------
    // Property
    // --------

    class Property : public Expression {
      private:
        // DATA

        // The name of the property.
        bsl::string d_name;

      public:
        // CREATORS

        /// Create an object that evaluates property `name`, in the
        /// evaluation context, as a boolean.
        explicit Property(const bsl::string& name);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
        /// Create an object that evaluates property `name`, in the
        /// evaluation context, as a boolean.
        explicit Property(bsl::string&& name) noexcept;
#endif

        // ACCESSORS

        /// Return the boolean value of the property. If the property is not
        /// a boolean, stop evaluation, set last error to e_TYPE, and return
        /// `false`;
        bdld::Datum
        evaluate(EvaluationContext& context) const BSLS_KEYWORD_OVERRIDE;
    };

    // --------------
    // IntegerLiteral
    // --------------

    class IntegerLiteral : public Expression {
      private:
        // DATA
        bsls::Types::Int64 d_value;

      public:
        // PUBLIC TYPES
        typedef bsls::Types::Int64 ValueType;

        // CREATORS

        /// Create an object that evaluates to Int64 `value`.
        explicit IntegerLiteral(bsls::Types::Int64 value);

        // ACCESSORS

        /// Return the integer passed to the constructor, as an Int64 Datum.
        bdld::Datum
        evaluate(EvaluationContext& context) const BSLS_KEYWORD_OVERRIDE;
    };

    // -------------
    // StringLiteral
    // -------------

    class StringLiteral : public Expression {
      private:
        // DATA
        bsl::string d_value;

      public:
        // PUBLIC TYPES
        typedef bsl::string ValueType;

        // CREATORS

        /// Create an object that evaluates to string `value`.
        explicit StringLiteral(const bsl::string& name);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
        /// Create an object that evaluates to string `value`.
        explicit StringLiteral(bsl::string&& name) noexcept;
#endif

        // ACCESSORS

        /// Return the string passed to the constructor, as StringRef Datum.
        bdld::Datum
        evaluate(EvaluationContext& context) const BSLS_KEYWORD_OVERRIDE;
    };

    // --------------
    // BooleanLiteral
    // --------------

    class BooleanLiteral : public Expression {
      private:
        // DATA
        bool d_value;

      public:
        // PUBLIC TYPES
        typedef bool ValueType;

        // CREATORS

        /// Create an object that evaluates to boolean `value`.
        explicit BooleanLiteral(bool value);

        // ACCESSORS

        /// Return the boolean value passed to the constructor, as a boolean
        /// Datum.
        bdld::Datum
        evaluate(EvaluationContext& context) const BSLS_KEYWORD_OVERRIDE;

        /// Return `d_value`.
        bool value() const;
    };

    // ----------
    // Comparison
    // ----------

    template <template <typename> class Op>
    class Comparison : public Expression {
      private:
        // DATA

        // Left operand.
        ExpressionPtr d_left;

        // Right operand.
        ExpressionPtr d_right;

      public:
        // CREATORS

        /// Create an object that compares `left` and `right` using `Op`,
        /// one of the standard comparison functors (`equal_to`, etc).
        Comparison(ExpressionPtr left, ExpressionPtr right);

        // ACCESSORS

        /// Evaluate the `left` and `right` expressions passed to the
        /// constructor.  If they have the same type, compare them using
        /// `Op`, and return the result as a boolean Datum. Otherwise, set
        /// the error in the context to  e_TYPE, stop the evaluation, and
        /// return a null datum.
        bdld::Datum
        evaluate(EvaluationContext& context) const BSLS_KEYWORD_OVERRIDE;
    };

    // --
    // Or
    // --

    class Or : public Expression {
      private:
        // DATA

        // Left operand.
        ExpressionPtr d_left;

        // Right operand.
        ExpressionPtr d_right;

      public:
        // CREATORS

        /// Create an object that returns the logical disjunction of `left`
        /// and `right`.
        Or(ExpressionPtr left, ExpressionPtr right);

        // ACCESSORS

        /// Evaluate the `left` expression passed to the constructor. If it
        /// is `true`, return `true` as a boolean Datum. If it is `false`,
        /// evaluate the `right` expression and return its value as a
        /// boolean Datum. If an expression evaluates to a non-boolean
        /// Datum, set the error in the context to  e_TYPE, stop the
        /// evaluation, and return a null datum. Note that the `right`
        /// expression is not evaluated if `left` evaluates to `true`, and
        /// its type is not checked.
        bdld::Datum
        evaluate(EvaluationContext& context) const BSLS_KEYWORD_OVERRIDE;
    };

    // ---
    // And
    // ---

    class And : public Expression {
      private:
        // DATA

        // Left operand.
        ExpressionPtr d_left;

        // Right operand.
        ExpressionPtr d_right;

      public:
        // CREATORS

        /// Create an object that returns the logical conjunction of `left`
        /// and `right`.
        And(ExpressionPtr left, ExpressionPtr right);

        // ACCESSORS

        /// Evaluate the `left` expression passed to the constructor. If it
        /// is `false`, return `false` as a boolean Datum. If it is `true`,
        /// evaluate the `right` expression and return its value as a
        /// boolean Datum. If an expression evaluates to a non-boolean
        /// Datum, set the error in the context to  e_TYPE, stop the
        /// evaluation, and return a null datum. Note that the `right`
        /// expression is not evaluated if `left` evaluates to `false`, and
        /// its type is not checked.
        bdld::Datum
        evaluate(EvaluationContext& context) const BSLS_KEYWORD_OVERRIDE;
    };

    // ------------------
    // NumBinaryOperation
    // ------------------

    template <template <typename> class Op>
    class NumBinaryOperation : public Expression {
      private:
        // DATA

        // Left operand.
        ExpressionPtr d_left;

        // Right operand.
        ExpressionPtr d_right;

      public:
        // CREATORS

        /// Create an object that returns the value of 'Op<Int64>(left,
        /// right).  `Op` is one of the standard binary arithmetic operation
        /// functors.
        NumBinaryOperation(ExpressionPtr left, ExpressionPtr right);

        // ACCESSORS

        /// Evaluate the `left` and `right` expressions passed to the
        /// constructor. If they have are both integers, apply `Op`, and
        /// return the result as a Int64 Datum. Otherwise, set the error in
        /// the context to  e_TYPE, stop the evaluation, and return a null
        /// datum.
        bdld::Datum
        evaluate(EvaluationContext& context) const BSLS_KEYWORD_OVERRIDE;
    };

    // ----------
    // UnaryMinus
    // ----------

    class UnaryMinus : public Expression {
      private:
        // DATA

        // The expression to negate.
        ExpressionPtr d_expression;

      public:
        // CREATORS

        /// Create an object that implements arithmetic negation.
        explicit UnaryMinus(ExpressionPtr expression);

        // ACCESSORS

        /// Evaluate `expression` passed to the constructor. If it is an
        /// integer, return the negated value as an Int64 Datum. Otherwise,
        /// set the error in the context to  e_TYPE, stop the evaluation,
        /// and return a null datum.
        bdld::Datum
        evaluate(EvaluationContext& context) const BSLS_KEYWORD_OVERRIDE;
    };

    // ---
    // Not
    // ---

    class Not : public Expression {
      private:
        // DATA

        // The expression to negate.
        ExpressionPtr d_expression;

      public:
        // CREATORS

        /// Create an object that implements logical negation.
        explicit Not(ExpressionPtr expression);

        // ACCESSORS

        /// Evaluate `expression` passed to the constructor. If it is a
        /// boolean, return the negated value as a boolean Datum.
        /// Otherwise, set the error in the context to  e_TYPE, stop the
        /// evaluation, and return a null datum.
        bdld::Datum
        evaluate(EvaluationContext& context) const BSLS_KEYWORD_OVERRIDE;
    };

  private:
    // SimpleEvaluator(const SimpleEvaluator& other) BSLS_KEYWORD_DELETED;
    // SimpleEvaluator& operator=(const SimpleEvaluator& other)
    // BSLS_KEYWORD_DELETED;

    // DATA

    // The expression to evaluate.
    bsl::shared_ptr<Expression> d_expression;

    // The flag indicating that `compile` was called for this expression.
    bool d_isCompiled;

    // PRIVATE FUNCTIONS

    /// Parse `expression`, using `context`. Generate an AST only if
    /// `context.d_validationOnly` is `false`.
    static void parse(const bsl::string&  expression,
                      CompilationContext& context);

  public:
    // PUBLIC CONSTANTS
    enum {
        /// The maximum number of operators allowed in a single expression.
        k_MAX_OPERATORS  = 10,
        k_MAX_PROPERTIES = 10
        // The maximum number of properties allowed in a single expression.
    };

    // CREATORS
    SimpleEvaluator();

    // DESTRUCTOR

    /// Destructor.
    virtual ~SimpleEvaluator();

    // PUBLIC FUNCTIONS

    /// Compile `expression`, using `context`.  Return 0 on success or
    /// non-zero return code otherwise.  See also: `ErrorType::Enum`.
    int compile(const bsl::string& expression, CompilationContext& context);

    /// Evaluate the expression, compiled from the `expression` passed to
    /// the constructor.
    bool evaluate(EvaluationContext& context) const;

    /// Return `true` if the `compile` was called for this object.
    bool isCompiled() const;

    /// Return `true` if the object contains an expression, resulting from
    /// the successful compilation of a string. `evaluate` can be called
    /// only if `isValid()` returns `true`.
    bool isValid() const;

    // PUBLIC STATIC FUNCTIONS

    /// Check `expression`. Return true if it is syntactically correct, and
    /// it does not contain too many properties or expressions.
    static bool validate(const bsl::string&  expression,
                         CompilationContext& context);

    // FRIENDS
    friend class SimpleEvaluatorParser;   // for access to Expression hierarchy
    friend class SimpleEvaluatorScanner;  // for access to Expression hierarchy
    friend class CompilationContext;      // for access to ExpressionPtr
};
// ========================
// class CompilationContext
// ========================

/// Contain the inputs and outputs of a compilation, as well as intermediate
/// data.
class CompilationContext {
  private:
    typedef SimpleEvaluator::ExpressionPtr ExpressionPtr;

  public:
    // PUBLIC TYPES
    enum Type { e_BOOL, e_INT, e_STRING };

    /// The type of the property, as deduced from the type of the values
    /// it is compared to - or `e_BOOL` if the property is used as a
    /// boolean.
    struct PropertyInfo {
        Type d_type;

        /// The position of the property in the list of properties, in order
        /// of first appearance in the expression.
        size_t d_index;
    };

  private:
    // PRIVATE DATA

    // The allocator to use to allocate all objects during compilation.
    bslma::Allocator* d_allocator;

    // If `true`, do not produce a AST.
    bool d_validationOnly;

    // The properties in the expression.
    bsl::unordered_map<bsl::string, PropertyInfo> d_properties;

    // The number of operators encountered during the compilation.
    size_t d_numOperators;

    // The number of properties encountered during the compilation.
    size_t d_numProperties;

    // If not zero, identifies the error that occurred during parsing.
    ErrorType::Enum d_lastError;

    // If `d_lastError` is not zero, contains a description of the error.
    mwcu::MemOutStream d_os;

    // The resulting AST, if `d_validationOnly` is set to `false`.
    ExpressionPtr d_expression;

    // PRIVATE MEMBER FUNCTIONS

    /// Return the index of the property, i.e. he position of the property
    /// in the list of properties, in order of first appearance in the
    /// expression. The first time a property is queried, it is added with
    /// the specified `type`, and its index is recorded.  Return -1 if the
    /// property is already recorded with a different `type`.
    int getPropertyIndex(const bsl::string& property, Type type);

    /// In compilation mode, create a subclass of Expression, passing
    /// `value` to the constructor, and return an ExpressionPtr to it. In
    /// validation mode, return a null ExpressionPtr. NodeType is any of
    /// IntegerLiteral, StringLiteral, and BooleanLiteral.
    template <typename NodeType>
    ExpressionPtr makeLiteral(typename NodeType::ValueType value);

    /// In compilation mode, create a Comparison<Op>, passing `a` and `b` to
    /// the constructor, and return an ExpressionPtr to it. In validation
    /// mode, return a null ExpressionPtr. In both cases, increment the
    /// operator count.
    template <template <typename> class Op>
    ExpressionPtr makeComparison(ExpressionPtr a, ExpressionPtr b);

    /// In compilation mode, create a subclass of Expression, passing
    /// `value` to the constructor, and return an ExpressionPtr to it. In
    /// validation mode, return a null ExpressionPtr. Class is any of
    /// Property, UnaryMinus, and Not; ArgType is the type required for the
    /// argument of the constructor. In both cases, increment the operator
    /// count.
    template <typename Class, typename ArgType>
    ExpressionPtr makeUnaryExpression(ArgType expr);

    /// In compilation mode, create a Class object, passing `a` and `b` to
    /// the constructor, and return an ExpressionPtr to it. In validation
    /// mode, return a null ExpressionPtr. Class is either And or Or. In
    /// both cases, increment the operator count.
    template <typename Class>
    ExpressionPtr makeBooleanBinaryExpression(ExpressionPtr a,
                                              ExpressionPtr b);

    /// In compilation mode, create a NumBinaryOperation<Op>, passing `a`
    /// and `b` to the constructor, and return an ExpressionPtr to it. In
    /// validation mode, return a null ExpressionPtr. In both cases,
    /// increment the operator count.
    template <template <typename> class Op>
    ExpressionPtr makeNumBinaryExpression(ExpressionPtr a, ExpressionPtr b);

  public:
    // CREATORS
    explicit CompilationContext(bslma::Allocator* allocator);

    // ACCESSORS

    /// Return `true` if an error occurred and `false` otherwise.
    bool hasError() const;

    /// Return the last error code.
    ErrorType::Enum lastError() const;

    /// Return the last message.
    bsl::string lastErrorMessage() const;

    friend class SimpleEvaluator;
    friend class SimpleEvaluatorParser;
};

// =======================
// class EvaluationContext
// =======================

/// Contain the inputs of an evaluation.
class EvaluationContext {
  private:
    // Where to read properties from.
    PropertiesReader* d_propertiesReader;

    // The allocator to use during evaluation.
    bslma::Allocator* d_allocator;

    // Set by `Expression::evaluate` functions when a property does not
    // have the type deduced during compilation. Stop evaluation and return
    // `true`.
    bool d_stop;

    ErrorType::Enum d_lastError;

  public:
    // CREATORS
    EvaluationContext(PropertiesReader* propertiesReader,
                      bslma::Allocator* allocator);

    void setPropertiesReader(PropertiesReader* propertiesReader);

    void reset();

    /// Stop execution.
    bdld::Datum stop();

    /// Return `true` if an error occurred and `false` otherwise.
    bool hasError() const;

    /// Return the last error code.
    ErrorType::Enum lastError() const;

    friend class SimpleEvaluator;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------
// struct ErrorType
// ----------------

inline const char* ErrorType::toString(ErrorType::Enum value)
{
    switch (value) {
    case 0: return "";                                         // RETURN
    case bmqeval::ErrorType::e_SYNTAX: return "syntax error";  // RETURN
    case bmqeval::ErrorType::e_TOO_COMPLEX:
        return "subscription expression is too complex";  // RETURN
    case bmqeval::ErrorType::e_NO_PROPERTIES:
        return "expression does not use any property";  // RETURN
    case bmqeval::ErrorType::e_NAME:
        return "undefined name in subscription expression";  // RETURN
    case bmqeval::ErrorType::e_TYPE:
        return "type error in expression";  // RETURN
    case bmqeval::ErrorType::e_BINARY:
        return "binary properties are not supported";  // RETURN
    default: return "unknown error";                   // RETURN
    }
}

// ---------------------
// class SimpleEvaluator
// ---------------------

inline bool SimpleEvaluator::isCompiled() const
{
    return d_isCompiled;
}

inline bool SimpleEvaluator::isValid() const
{
    return d_expression != 0;
}

// -------------------------------------
// class SimpleEvaluator::IntegerLiteral
// -------------------------------------

inline SimpleEvaluator::IntegerLiteral::IntegerLiteral(
    bsls::Types::Int64 value)
: d_value(value)
{
}

// -------------------------------------
// class SimpleEvaluator::BooleanLiteral
// -------------------------------------

inline SimpleEvaluator::BooleanLiteral::BooleanLiteral(bool value)
: d_value(value)
{
}

inline bool SimpleEvaluator::BooleanLiteral::value() const
{
    return d_value;
}

// ------------------------------------------
// template class SimpleEvaluator::Comparison
// ------------------------------------------

template <template <typename> class Op>
inline SimpleEvaluator::Comparison<Op>::Comparison(ExpressionPtr left,
                                                   ExpressionPtr right)
: d_left(left)
, d_right(right)
{
}

template <template <typename> class Op>
bdld::Datum
SimpleEvaluator::Comparison<Op>::evaluate(EvaluationContext& context) const
{
    bdld::Datum left = d_left->evaluate(context);

    if (context.d_stop) {
        return bdld::Datum::createNull();  // RETURN
    }

    bdld::Datum right = d_right->evaluate(context);

    if (context.d_stop) {
        return bdld::Datum::createNull();  // RETURN
    }

    if (left.isString()) {
        if (right.isString()) {
            return bdld::Datum::createBoolean(
                Op<bslstl::StringRef>()(left.theString(), right.theString()));
            // RETURN
        }

        context.d_lastError = ErrorType::e_TYPE;

        return context.stop();  // RETURN
    }

    bsls::Types::Int64 a;
    bsls::Types::Int64 b;

    if (left.isInteger64()) {
        a = left.theInteger64();
    }
    else if (left.isInteger()) {
        a = left.theInteger();
    }
    else {
        context.d_lastError = ErrorType::e_TYPE;

        return context.stop();  // RETURN
    }

    if (right.isInteger64()) {
        b = right.theInteger64();
    }
    else if (right.isInteger()) {
        b = right.theInteger();
    }
    else {
        context.d_lastError = ErrorType::e_TYPE;

        return context.stop();  // RETURN
    }

    return bdld::Datum::createBoolean(Op<bsls::Types::Int64>()(a, b));
}

// ----------------------------------
// template class SimpleEvaluator::Or
// ----------------------------------

inline SimpleEvaluator::Or::Or(ExpressionPtr left, ExpressionPtr right)
: d_left(left)
, d_right(right)
{
}

// -----------------------------------
// template class SimpleEvaluator::And
// -----------------------------------

inline SimpleEvaluator::And::And(ExpressionPtr left, ExpressionPtr right)
: d_left(left)
, d_right(right)
{
}

// --------------------------------------------------
// template class SimpleEvaluator::NumBinaryOperation
// --------------------------------------------------

template <template <typename> class Op>
inline SimpleEvaluator::NumBinaryOperation<Op>::NumBinaryOperation(
    ExpressionPtr left,
    ExpressionPtr right)
: d_left(left)
, d_right(right)
{
}

template <template <typename> class Op>
bdld::Datum SimpleEvaluator::NumBinaryOperation<Op>::evaluate(
    EvaluationContext& context) const
{
    bdld::Datum left = d_left->evaluate(context);

    if (context.d_stop) {
        return bdld::Datum::createNull();  // RETURN
    }

    bdld::Datum right = d_right->evaluate(context);

    if (context.d_stop) {
        return bdld::Datum::createNull();  // RETURN
    }

    bsls::Types::Int64 a;
    bsls::Types::Int64 b;

    if (left.isInteger64()) {
        a = left.theInteger64();
    }
    else if (left.isInteger()) {
        a = left.theInteger();
    }
    else {
        context.d_lastError = ErrorType::e_TYPE;

        return context.stop();  // RETURN
    }

    if (right.isInteger64()) {
        b = right.theInteger64();
    }
    else if (right.isInteger()) {
        b = right.theInteger();
    }
    else {
        context.d_lastError = ErrorType::e_TYPE;

        return context.stop();  // RETURN
    }

    bsls::Types::Int64 result = Op<bsls::Types::Int64>()(a, b);

    return bdld::Datum::createInteger64(result, context.d_allocator);
}

// ------------------------------------------
// template class SimpleEvaluator::UnaryMinus
// ------------------------------------------

inline SimpleEvaluator::UnaryMinus::UnaryMinus(ExpressionPtr expression)
: d_expression(expression)
{
}

// -----------------------------------
// template class SimpleEvaluator::Not
// -----------------------------------

inline SimpleEvaluator::Not::Not(ExpressionPtr expression)
: d_expression(expression)
{
}

// ------------------------
// class CompilationContext
// ------------------------

inline CompilationContext::CompilationContext(bslma::Allocator* allocator)
: d_allocator(allocator)
, d_validationOnly(false)
, d_properties(allocator)
, d_numOperators(0)
, d_numProperties(0)
, d_lastError(ErrorType::e_OK)
, d_os(allocator)
{
}

inline bool CompilationContext::hasError() const
{
    return d_lastError != 0;
}

inline ErrorType::Enum CompilationContext::lastError() const
{
    return d_lastError;
}

inline bsl::string CompilationContext::lastErrorMessage() const
{
    return d_os.str();
}

inline int CompilationContext::getPropertyIndex(const bsl::string& property,
                                                Type               type)
{
    bsl::unordered_map<bsl::string, PropertyInfo>::iterator iter =
        d_properties.find(property);
    if (iter == d_properties.end()) {
        PropertyInfo info = {type, d_properties.size()};
        d_properties.insert(iter, bsl::make_pair(property, info));
        return info.d_index;  // RETURN
    }
    if (iter->second.d_type == type) {
        return iter->second.d_index;  // RETURN
    }
    return -1;
}

template <typename NodeType>
SimpleEvaluator::ExpressionPtr
CompilationContext::makeLiteral(typename NodeType::ValueType value)
{
    if (d_validationOnly) {
        return ExpressionPtr();  // RETURN
    }

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    return SimpleEvaluator::ExpressionPtr(new (*d_allocator)
                                              NodeType(bsl::move(value)),
                                          d_allocator);  // RETURN
#else
    return SimpleEvaluator::ExpressionPtr(new (*d_allocator) NodeType(value),
                                          d_allocator);  // RETURN
#endif
}

template <template <typename> class Op>
SimpleEvaluator::ExpressionPtr
CompilationContext::makeComparison(ExpressionPtr a, ExpressionPtr b)
{
    ++d_numOperators;

    if (d_validationOnly) {
        return ExpressionPtr();  // RETURN
    }

    // Handle comparisons with boolean literals at compile time.
    //   - for 'expr = true' and 'true = expr' return 'expr'
    //   - for 'expr = false' and 'false = expr' return '!expr'
    //   - if both sides are boolean literals, return
    //   'BooleanLiteral(a && b)'

    typedef SimpleEvaluator::BooleanLiteral BooleanLiteral;
    typedef SimpleEvaluator::Not            Not;

    // Booleans can only be compared for (in)equality.
    const bool isEquality =
        bsl::is_same<Op<int>, bsl::equal_to<int> >::value ||
        bsl::is_same<Op<int>, bsl::not_equal_to<int> >::value;

    if /*constexpr*/ (isEquality) {
        const BooleanLiteral* boolean_a = dynamic_cast<const BooleanLiteral*>(
            a.get());
        const BooleanLiteral* boolean_b = dynamic_cast<const BooleanLiteral*>(
            b.get());

        if (boolean_a) {
            if (boolean_b) {
                return ExpressionPtr(
                    new (*d_allocator) BooleanLiteral(
                        Op<bool>()(boolean_a->value(), boolean_b->value())),
                    d_allocator);  // RETURN
            }

            if (boolean_a->value()) {
                return b;  // RETURN
            }

            return ExpressionPtr(new (*d_allocator) Not(b),
                                 d_allocator);  // RETURN
        }

        if (boolean_b) {
            if (boolean_b->value()) {
                return a;  // RETURN
            }

            return ExpressionPtr(new (*d_allocator) Not(a),
                                 d_allocator);  // RETURN
        }
    }

    return ExpressionPtr(new (*d_allocator)
                             SimpleEvaluator::Comparison<Op>(a, b),
                         d_allocator);
}

template <typename Class>
SimpleEvaluator::ExpressionPtr
CompilationContext::makeBooleanBinaryExpression(ExpressionPtr a,
                                                ExpressionPtr b)
{
    ++d_numOperators;

    if (d_validationOnly) {
        return ExpressionPtr();  // RETURN
    }

    return ExpressionPtr(new (*d_allocator) Class(a, b), d_allocator);
}

template <template <typename> class Op>
SimpleEvaluator::ExpressionPtr
CompilationContext::makeNumBinaryExpression(ExpressionPtr a, ExpressionPtr b)
{
    ++d_numOperators;

    if (d_validationOnly) {
        return ExpressionPtr();  // RETURN
    }

    return ExpressionPtr(new (*d_allocator)
                             SimpleEvaluator::NumBinaryOperation<Op>(a, b),
                         d_allocator);
}

template <typename Class, typename ArgType>
SimpleEvaluator::ExpressionPtr
CompilationContext::makeUnaryExpression(ArgType expr)
{
    ++d_numOperators;

    if (d_validationOnly) {
        return ExpressionPtr();  // RETURN
    }

    return ExpressionPtr(new (*d_allocator) Class(expr), d_allocator);
}

// -----------------------
// class EvaluationContext
// -----------------------

inline EvaluationContext::EvaluationContext(PropertiesReader* propertiesReader,
                                            bslma::Allocator* allocator)
: d_propertiesReader(propertiesReader)
, d_allocator(allocator)
, d_stop(false)
, d_lastError(ErrorType::e_OK)
{
}

inline void
EvaluationContext::setPropertiesReader(PropertiesReader* propertiesReader)
{
    d_propertiesReader = propertiesReader;
}

inline void EvaluationContext::reset()
{
    d_stop      = false;
    d_lastError = ErrorType::e_OK;
}

inline bdld::Datum EvaluationContext::stop()
{
    d_stop = true;

    return bdld::Datum::createNull();
}

inline bool EvaluationContext::hasError() const
{
    return d_lastError != 0;
}

inline ErrorType::Enum EvaluationContext::lastError() const
{
    return d_lastError;
}

}  // close package namespace
}  // close enterprise namespace

#endif

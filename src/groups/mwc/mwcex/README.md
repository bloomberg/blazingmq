## MWCEX

### Description
The `MWCEX` (MiddleWare Core Executors) package provides executors and other
async programming utilites and mechanisms.

### Basic Terminology
An **execution context** represents a place where function objects are
executed. A typical example of an execution context is a thread pool.

An **execution agent** is a unit of execution of a specific execution context
that is mapped to a single invocation of a callable function object. Typical
examples of an execution agent are a CPU thread or GPU execution unit.

An **executor** is an object associated with a specific execution context. It
provides one or more **execution functions** for creating execution agents from
a callable function object. The execution agents created are bound to the
executor’s context.

### Execution Properties and Execution Policies
An **execution property** is a trait defining the behavior of an execution
function. The design of this package deals with two such properties:

* **Directionality**. Does the execution function allow to retrieve the result
  of the submitted function object invocation? Lets call functions that do not
  allow that **One-Way** functions, and the ones that do **Two-Way** functions.

* **Blocking behavior**. Does the execution function blocks the calling thread
  pending completion of the submitted function object invocation? Possible
  answers to that question may be "yes", "no" and "maybe". Lets call such
  functions **Never Blocking**, **Always Blocking** and **Possibly Blocking**
  respectively. Note that the Possibly Blocking execution property is
  introduced in this model to allow execution functions to "decide" whether
  to block the calling thread while executing a submitted function object
  based on some internal knowledge. An example of use-case for that feature
  is a thread pool executing a function object in-place in case the execution
  function is called from one of the pool's threads, and storing that function
  object in an internal queue for deferred invocation otherwise.

A set of execution properties defining the behavior of an execution function is
called an **execution policy**.

### Requirements on Execution Functions
In the table below, `f` denotes a function object of type `F&&` callable as
`DECAY_COPY(std::forward<F>(f))()` and where `bsl::decay_t<F>` satisfies the
Destructible and MoveConstructible requirements as specified in the C++
standard, and `R` denotes the type of the expression `DECAY_COPY(
bsl::forward<F>(f))()`.
```
//=============================================================================
// Execution Policy | Return Type | Execution function behavior
//=============================================================================
//                  |             |
// One-Way Never    | void        | Creates an execution agent which invokes
// Blocking         |             | 'DECAY_COPY(bsl::forward<F>(f))()' at most
//                  |             | once, with the call to 'DECAY_COPY' being
//                  |             | evaluated in the current thread of
//                  |             | execution. The execution function shall not
//                  |             | block forward progress of the caller
//                  |             | pending completion of 'f's invocation. The
//                  |             | progress of 'f' may begin before the call
//                  |             | to the execution function completes. The
//                  |             | invocation of the execution function
//                  |             | synchronizes with the invocation of 'f'.
//                  |             | The execution function shall not propagate
//                  |             | any exception thrown during the invocation
//                  |             | of 'f'. [NOTE: the treatment of exceptions
//                  |             | thrown by submitted function objects is
//                  |             | implementation defined.]
//------------------|-------------|--------------------------------------------
//                  |             |
// One-Way Possibly | void        | Creates an execution agent which invokes
// Blocking         |             | 'DECAY_COPY(bsl::forward<F>(f))()' at most
//                  |             | once, with the call to 'DECAY_COPY' being
//                  |             | evaluated in the current thread of
//                  |             | execution. The execution function may block
//                  |             | forward progress of the caller pending
//                  |             | completion of 'f's invocation. The progress
//                  |             | of 'f' may begin before the call to the
//                  |             | execution function completes. The
//                  |             | invocation of the execution function
//                  |             | synchronizes with the invocation of 'f'.
//                  |             | The execution function shall not propagate
//                  |             | any exception thrown during the invocation
//                  |             | of 'f'. [NOTE: the treatment of exceptions
//                  |             | thrown by submitted function objects is
//                  |             | implementation defined.]
//------------------|-------------|--------------------------------------------
//                  |             |
// One-Way Always   | void        | Creates an execution agent which invokes
// Blocking         |             | 'DECAY_COPY(bsl::forward<F>(f))()' at most
//                  |             | once, with the call to 'DECAY_COPY' being
//                  |             | evaluated in the current thread of
//                  |             | execution. The execution function does
//                  |             | block forward progress of the caller
//                  |             | pending completion of 'f's invocation. The
//                  |             | invocation of the execution function
//                  |             | synchronizes with the invocation of 'f'.
//                  |             | The execution function shall not propagate
//                  |             | any exception thrown during the invocation
//                  |             | of 'f'. [NOTE: the treatment of exceptions
//                  |             | thrown by submitted function objects is
//                  |             | implementation defined.]
//------------------|-------------|--------------------------------------------
//                  |             |
// Two-Way Never    | Future<R>   | Creates an execution agent which invokes
// Blocking         |             | 'DECAY_COPY(bsl::forward<F>(f))()' at most
//                  |             | once, with the call to 'DECAY_COPY' being
//                  |             | evaluated in the current thread of
//                  |             | execution. Stores the result of 'f's
//                  |             | invocation (or any resulting exception) in
//                  |             | in the associated shared state of the
//                  |             | returned 'Future'. The execution function
//                  |             | shall not block forward progress of the
//                  |             | caller pending completion of 'f's
//                  |             | invocation. The progress of 'f' may begin
//                  |             | before the call to the execution function
//                  |             | completes. The invocation of the execution
//                  |             | function synchronizes with the invocation
//                  |             | of 'f'.
//------------------|-------------|--------------------------------------------
//                  |             |
// Two-Way Possibly | Future<R>   | Creates an execution agent which invokes
// Blocking         |             | 'DECAY_COPY(bsl::forward<F>(f))()' at most
//                  |             | once, with the call to 'DECAY_COPY' being
//                  |             | evaluated in the current thread of
//                  |             | execution. Stores the result of 'f's
//                  |             | invocation (or any resulting exception) in
//                  |             | in the associated shared state of the
//                  |             | returned 'Future'. The execution function
//                  |             | may block forward progress of the caller
//                  |             | pending completion of 'f's invocation. The
//                  |             | progress of 'f' may begin before the call
//                  |             | to the execution function completes. The
//                  |             | invocation of the execution function
//                  |             | synchronizes with the invocation of 'f'.
//------------------|-------------|--------------------------------------------
//                  |             |
// Two-Way Always   | Future<R>   | Creates an execution agent which invokes
// Blocking         |             | 'DECAY_COPY(bsl::forward<F>(f))()' at most
//                  |             | once, with the call to 'DECAY_COPY' being
//                  |             | evaluated in the current thread of
//                  |             | execution. Stores the result of 'f's
//                  |             | invocation (or any resulting exception) in
//                  |             | in the associated shared state of the
//                  |             | returned 'Future'. The execution function
//                  |             | does block forward progress of the caller
//                  |             | pending completion of 'f's invocation. The
//                  |             | invocation of the execution function
//                  |             | synchronizes with the invocation of 'f'.
```

### Requirements on Executors
A type `E` meets the Executor requirements if it satisfies the requirements of
Destructible, CopyConstructible and EqualityComparable as specified in the C++
standard, as well as the additional requirements listed below.

No comparison operation, copy operation, move operation or swap operation on
these types shall exit via an exception.

The executor copy constructor, comparison operators, and other member functions
defined in these requirements shall not introduce data races as a result of
concurrent calls to those functions from different threads. `dispatch` may be
recursively reentered.

An executor type's destructor shall not block pending completion of the
submitted function objects.

In the table below:
* `e`, `e1` and `e2` denote (possibly const) values of type `E`.
* `T` is `mwcex::ExecutorTraits<E>`.
* `f` denotes a function object of type `F&&` callable as
  `DECAY_COPY(std::forward<F>(f))()` and where `bsl::decay_t<F>`
  satisfies the Destructible and MoveConstructible requirements as specified in
  the C++ standard.
```
//=============================================================================
// Expression       | Return Type | Description
//=============================================================================
//                  |             |
// e1 == e2         | bool        | Returns 'true' only if 'e1' and 'e2' can be
//                  |             | interchanged with identical effects in any
//                  |             | of the expressions defined in these type
//                  |             | requirements. [ Note: Returning 'false'
//                  |             | does not necessarily imply that the effects
//                  |             | are not identical. ]
//------------------|-------------|--------------------------------------------
//                  |             |
// T::post(e, f)    | void        | Executes 'f' according to the One-Way
//                  |             | Never Blocking policy.
//------------------|-------------|--------------------------------------------
//                  |             |
// T::dispatch(e, f)| void        | Executes 'f' according to the One-Way
//                  |             | Possibly Blocking policy.
//------------------|-------------|--------------------------------------------
```

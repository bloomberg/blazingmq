---
layout: default
title: Subscriptions
parent: Features
nav_order: 3
---

# Subscriptions (aka Topic-based Routing)
{: .no_toc }

* toc
{:toc}

## Introduction

Subscriptions provide consumer applications a powerful mechanism to express
interest in receiving only those messages which satisfy criteria specified by
them.  In the absence of subscriptions, a consumer attached to a queue can
receive any and all messages posted on the queue, and should be in a position
to process all of them.  In other words, the queue is viewed as a logical
stream of homogeneous data.  While this may work in some or most cases, there
are scenarios where this restriction prevents a more flexible or natural
arrangement of consumer applications.  For example, some users may prefer one
set of consumers to handle messages of a certain type, and another set of
consumers to handle messages of a certain other type.  This is where
subscriptions come in -- they enable consumer applications to "subscribe" to
messages of a certain type, thereby *logically* converting a queue into a
stream of heterogeneous data.

Concretely speaking, producer applications can put any interesting message
attributes in the *message properties* section of the message (*message
properties* are a list of key/value pairs that a producer can associate with a
message), and consumers can specify filters using one or more *message
properties*.  For example, if a message contains these three properties:

- `CustomerId = 1234`
- `DestinationId = "ABC"`
- `OrderType = EXPRESS`

A consumer can provide a filter ("subscription expression") like so when
attaching to a queue:

- `CustomerId == 1234 && OrderType == EXPRESS`

In this case, a message having three properties as shown above will be routed
to the consumer with above filter (note that if a property is not specified by
the consumer, it is considered to be a wildcard).

Similarly, users can spin up any number of consumers, each with different
filters.  Users have to ensure that every message can be processed by at least
one consumer.

## Detailed Overview

This section provides an in-depth overview of subscriptions -- motivation, high
level design, selective implementation details, etc.  This section assumes that
reader is familiar with various routing strategies (aka 'queue modes') as well
as general BlazingMQ terminology like *PUT*, *PUSH*, *ACK*, *CONFIRM* messages,
etc.

### Background
{:.no_toc}

1. Consider a scenario where multiple consumers are attached to a queue in work
   queue or priority mode. Without subscriptions, there is no way for consumers
   to specify any restrictions on the messages that BlazingMQ will route to
   them. All consumers must be in a position to process any message posted on
   the queue. As an example, let’s assume that all messages being posted to the
   queue have an attribute called `CustomerId` with various possible values
   representing different customers. There could be a case where some consumers
   can process only those messages where `CustomerId == 1000` while others can
   process messages only where `CustomerId == 2000` and so on.

2. Consider a slight variation of above scenario, such that although every
   consumer can process message with any `CustomerId`, but in order to provide
   better isolation and QoS, owners of the consumer application want to
   dedicate a set of consumers for every firm. So *M* consumers are dedicated
   for `CustomerId == 1000` , *N* consumers for `CustomerId == 2000` , etc.

3. Consider another scenario where one or more consumers are attached to a
   queue, but none of them are ever interested in messages where `CustomerId`
   is different from 1000 or 2000 . Such a scenario can occur where one team is
   producing on the queue and other teams are consuming from it, and the later
   don’t control the types of messages being posted on the queue.

Without subscriptions, there is no way in BlazingMQ in which users can
implement a solution for these scenario. Some workarounds exist. Let's look at
them:

- Scenarios (1) and (2) can be solved by creating one BlazingMQ queue for each
  `CustomerId` and having producer(s) post a message on the appropriate
  queue. This solution can work as long as the total number of `CustomerId`s
  (and thus, queues) is limited to a few hundred (a large number of queues
  impacts applications’ and BlazingMQ cluster’s startup and failover
  times). Moreover, producer and consumer applications would have to agree on
  the naming convention of the queues.

- Scenario (3) can be implemented by consumer applications by simply confirming
  those messages which they are not interested in. However, such messages are
  still routed to the consumers, which leads to wasted network bandwidth and
  CPU.

### Design
{:.no_toc}

Scenarios described in the previous section can be solved by introducing the
notion of subscriptions in BlazingMQ, and routing a message to consumer(s) with
matching subscription(s). Here’s how subscriptions work at a high level:

- Producers add any ‘interesting’ attributes of the message in its *message
  properties*.

- Consumers specify one or more boolean expressions when opening the
  queue. Each expression can contain one or more message properties. As an
  example, an expression can look like:

  - `CustomerId == 1000`
  - `CustomerId == 2000 && DestinationId == "ABC"`
  - `CustomerId >= 1000 && CustomerId <= 2000`

  where `CustomerId` and `DestinationId` are message properties. Every such
  expression will be a subscription and we will be using these two terms
  interchangeably. Of course, subscriptions are optional and if a consumer does
  not specify any subscriptions, it will receive all of the messages posted on
  the queue.

- In addition to specifying subscriptions when opening the queue (using the
  `openQueue` API), consumers will also be able to specify them when
  configuring the queue (`configureQueue` API). Consumers will be able to tweak
  existing subscriptions, remove an existing subscription or add a new
  subscription using the `configureQueue` API. In that regard, subscriptions
  will be completely dynamic.

- In addition, consumers will also be able to specify certain options like
  priority and flow-control parameters for each subscription. Recall that
  currently, these options – `bmqt::QueueOptions` – are specified for the
  entire queue. However, after the introduction of subscriptions, consumers
  will be able to specify different values for each subscription. This means
  that priorities will now be applicable at the subscription level. For
  example, a consumer application can advertise priority 10 for one
  subscription, and priority 5 for another. These priorities will be taken into
  consideration by BlazingMQ when routing messages (as is currently the
  case). In addition, it will also help BlazingMQ provide some determinism when
  routing a message in a queue having multiple subscriptions (see *Order of
  Evaluation* bullet in *Implementation Details* section below).

- Existing APIs will continue to work and consumer applications which do not
  use subscriptions will not need to make any changes.

- In the BlazingMQ backend, upon the arrival of a new message, BlazingMQ
  primary node will try to match the message with a subscription and route the
  message to the consumer with that subscription. See *Implementation Details*
  section below for more info.

### Implementation Details
{:.no_toc}

The *Design* section above gives a high level overview of the feature. There
are, however, some additional details which are worth specifying.

1. **Overlapping Subscriptions**: in case if consumers specify overlapping
   subscriptions (e.g., `CustomerId == 0` and `CustomerId >= 0` ), BlazingMQ
   will not make any attempt to merge those subscriptions, and the two
   subscriptions will be treated independently of each other. **NOTE**: While
   overlapping subscriptions are supported as described in this and next
   bullet, having overlapping subscriptions is questionable and can be
   confusing, and generally speaking, should be avoided.

2. **Order of Evaluation**: A very common scenario would be a queue having
   consumers with different subscriptions. When a message arrives in a queue,
   BlazingMQ primary node takes the greedy approach and routes the message to
   the first matching subscription. A natural question that arises is in what
   order will the primary node evaluate subscriptions? BlazingMQ primary node
   will attempt to order subscriptions by priority and evaluate them from
   highest to lowest priority. This will help provide some determinism to the
   order of evaluation and can also help users implement interesting
   scenarios. As an example, taking the case of overlapping subscriptions in
   the previous bullet, if `CustomerId == 0` subscription has priority 10 and
   `CustomerId >= 0` subscription has priority 5, BlazingMQ will always try to
   route a message with `CustomerId = 0` attribute to the `CustomerId == 0`
   subscription.

3. **Order of Message Delivery**: Messages will be evaluated for matching
   subscriptions in the order that they arrive on the queue. So if a matching
   subscription exists for every message, messages will be dispatched to
   consumer(s) in order. Of course, if there are multiple consumers, each with
   different subscriptions, messages could be processed by out of order. This
   behavior will be same as priority as well as fan-out modes. Order of
   delivery will not be guaranteed in case messages are re-routed due to
   consumer crashes. Again, this behavior will be same as priority as well as
   fan-out modes.

4. **Spillover to Lower Priority Consumers**: If multiple consumers attach to a
   queue with the same subscription but with different priorities, BlazingMQ
   will route messages only to the consumer with highest priority. If there are
   multiple consumers with highest priority, BlazingMQ will round-robin
   messages across them. More importantly, if highest priority consumers reach
   capacity (flow-controlled), messages will not be spilled to lower priority
   consumers. This behavior is similar to the existing logic in various queue
   modes in BlazingMQ.

5. **Merging of Subscriptions**: In case multiple consumers specify the same
   subscription, BlazingMQ backend will seamlessly merge them by combining the
   options advertised by the consumers for that subscription. For example, if
   consumer A subscribes to `CustomerId == 1000` with priority 10 and capacity
   X , and consumer B subscribes to `CustomerId == 1000` with priority 10 and
   capacity Y , the two subscriptions will be merged by an intermediate hop in
   BlazingMQ backend and advertised to the primary node as `CustomerId == 1000`
   with priority 10 and capacity X + Y . Two subscriptions will be determined
   to be same if they compare equal lexicographically. Merging of subscriptions
   will ensure that load-balancing across consumers having same subscription
   works seamlessly.

6. **Evaluating Expressions**: BlazingMQ primary node will match subscriptions
   by reading corresponding message properties from the message and evaluating
   the expression. For example, while evaluating the `CustomerId == 1000`
   expression, primary node will read `CustomerId` property from the message
   and compare its value against `1000` to determine a match. It is worth
   noting that BlazingMQ backend can read a property in a message extremely
   efficiently (in `O(1)` time) – we essentially build a hashtable on the wire.

## Expression Language

The expression language for subscriptions implements basic string and integer
manipulation, as a tiny subset of the C programming language.

### Expression
{:.no_toc}

- An expression consists of a combination of:
  - Identifiers
  - Integer, string and boolean literals
  - Arithmetic, relational and boolean operators
  - Parentheses
- Spaces, tabs and line feeds are ignored
- The language has three types: integer, string, and boolean
- The final result of an expression must be a boolean

### Identifiers
{:.no_toc}

- Identifiers consist of a combination of upper and lower-case alphabetical
  letters, digits, and underscores.

- The first character may not be an underscore.

- Property names are case sensitive. Example of valid identifiers are: `ask`,
  `Ask`, `askPrice`, `ask_price`, `sp500`; note that `ask` and `Ask` are
  different identifiers.

- Examples of _invalid_ identifiers are:
  - `_ask` (begins with underscore)
  - `1Y` (begins with digit)
  - `a+b` (would be parsed as three tokens: `a`, `+`, and `b`).

- When an identifier is evaluated, the value of the eponymous property, in the
  current message, is returned.

- Note that the identifiers in an expression are not necessarily all evaluated
  (see boolean operators below).

### Integer Literals
{:.no_toc}

- Integer literals consist of a sequence of digits, possibly preceded by the
  minus sign.

### String Literals
{:.no_toc}

- String literals consist of a sequence of characters, enclosed in double
  quotes.

- To insert a double quote or a backslash in the sequence, prefix it
  with a backslash (thus: `"a\"b"` is the string `a"b`, and `"a\\b"` is the
  string `a\b`).

- No character other than `"` or `\` is allowed after a `\`. This is the syntax
  of strings in the C language.

### Boolean Literals
{:.no_toc}

- Boolean literals are `true` and `false`.

### Arithmetic Operators
{:.no_toc}

- Arithmetic operators are:
  - `+`
  - `-`
  - `*`
  - `/`
  - `%` (modulus)
- They require two integer arguments.

### Relational Operators
{:.no_toc}

- Relational operators are:
  - `=`
  - `!=`
  - `<`
  - `<=`
  - `>`
  - `>=`

- They require two arguments of the same type.

### Boolean Operators
{:.no_toc}

- Boolean operators are:
  - `&&`
  - `||` (junctions)
  - `!` (negation).

- Negation requires one argument
- Junctions require two boolean arguments
- Junctions are short-circuiting: if the left side of `&&` is `false`, or if
  the left side of `||` is `true`, the right side is not evaluated. As a
  consequence, identifiers in an expression do not all need to have a
  corresponding property of the right type. For example, `type == "i" && shares
  > 1000 || shares == "all"` is valid, as long as the value of `type` and the
  type of `shares` are properly correlated. `order == "limit" and limit == 0`
  can be used to detect the messages that represent a Limit Order with a silly
  limit value, and e.g. log an error message.

### Operator Precedence
{:.no_toc}

 - Operator precedence and associativity are the same as in C; from high
   precedence to low:

| Operator          | Associativity |
| ----------------- | ------------- |
| `!`               | Right         |
| `*` `/` `%`       | Left          |
| `+` `-`           | Left          |
| `<` `<=` `>` `>=` | Left          |
| `==` `!=`         | Left          |
| `&&`              | Left          |
| `||`              | Left          |

- Expressions between parentheses are evaluated first, starting with the
  innermost.

- Operations with higher precedence are evaluated before operations
  with lower precedence. For example, `a*x+b>0` is evaluated as `((a*x)+b)>0`.

- When two operators have the same precedence, they are evaluated from left to
  right, except for `!` which is evaluated right to left. For example, in
  `a-b-c` is evaluated as `(a-b)-c`. `!!ok` is evaluated as `!(!ok)`.

For a formal specification, see [Flex
rules](https://github.com/bloomberg/blazingmq/blob/main/src/groups/bmq/bmqeval/bmqeval_simpleevaluatorscanner.l)
for the tokenizer and the [Bison
grammar](https://github.com/bloomberg/blazingmq/blob/main/src/groups/bmq/bmqeval/bmqeval_simpleevaluatorparser.y).

---

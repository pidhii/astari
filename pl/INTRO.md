TODO:
- [ ] Embedding queries into C++ (programatic retreival of query results).
- [ ] Defining predicates with C++.
- [ ] Programatic Prolog code generation.

This package provides a (hopefuly) easy to use embedable Prolog interpreter
for C++ projects.
The main interface is a `class interpreter`. It represents an encapsulated
interpreter state that can be created or destroyed at will, and allows for
simultaneous existance of multiple independent Prologs (interpreters) in your
project.

# Quick Start

## 1. Create An Interpreter
```cpp
#include "pl/core/interpreter.hpp"

interpreter pl;
```
Done. We have created a fresh and pristine interpreter.

## 2. Basic Query
The fastest way to "just run a query" is `eval`:
```cpp
#include "pl/core/interpreter.hpp"

interpreter pl;

pl.eval("fail");
```
This query will simply fail. But at the moment we can't write any more
sophisticated query because, as mentioned above, we are really dealing with a
pristine interpreter, that doesn't have any useful predicates in its database.
If we tried to run even something like `pl.eval("X = foo");` we would trigger an
exception saying `"no such predicate (=/2)"`. So let's improve our example:
```cpp
#include "pl/core/interpreter.hpp"

interpreter pl;

pl << "X = X."; // define the `=`/2
pl.eval("X = foo");
```
```
yes: X = foo
```
Here we used the `operator <<` of the interpreter. The `operator <<` provides an
easy way to supply the interpreter with top-level Prolog expressions such as
predicate definitions. So let's make yet something more useful:
```cpp
#include "pl/core/interpreter.hpp"

interpreter pl;

pl << R"(
  % '='/2
  X = X.

  % member/2
  member(X, [Y|Ys]) :-
    X = Y;
    member(X, Ys).
)";
pl.eval("member(X, [a, b, c])");
```
```
yes: X = a
yes: X = b
yes: X = c
```
Great! However, as we keep defining more predicates, it will be better to
offload the Prolog code into a separate file, which we load before querying the
interpreter. let's move the Prolog definitions from the example outside:
```prolog
% file: program.pl

X = X.

member(X, [Y|Ys]) :-
  X = Y;
  member(X, Ys).
```
```cpp
#include "pl/core/interpreter.hpp"

interpreter pl;

pl.load_file("program.pl");
pl.eval("member(X, [a, b, c])");
```
```
yes: X = a
yes: X = b
yes: X = c
```

## 3. Builtin Libraries
The examples above illustrate well that Prolog interpreter with an empty
database is pretty useless.
The set of ISO predicates does define a small-enough but fairly useful
initial database that we would like to have for any useful Prolog application.
So let's have them:
```cpp
#include "pl/core/interpreter.hpp"
#include "pl/builtins/iso.hpp" // class iso

interpreter pl;
iso _iso {pl, iso_all}; // Load all available ISO predicates

pl.eval("write(\"Hello World!\"), nl");
```
```
Hello World!
yes
```
Et voilà.  
Please note that we deliberately keep an `iso` object alive past the loading
phase (which happens in iso's constructor). The library has to manage some
internal data during operation (e.g. tables with I/O streams), and this data
should only be released once we are done using the associated interpreter.
Also note that the ISO library is still in development. Consult [pl/README.md](README.md)
for the status. Some ISO predicates may be implemented differently from
corresponding definitions in ISO. I understand the consequences of such
inconvenience, but I can't live with predicates like `asserta`, `assertz`, and
`retract` being dumped from levels of ultimate utility down to *"avoid if possible"*.

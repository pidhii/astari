# Opium
(hopefully) Universal type-system interpreter:
- maximally unhinged type inference; all more constrained systems are then
  merely restrictions of such an "unhinged" system, and can thus be implemented
  with it;
- parametric polymorphism: templates/generics;
- subtyping: inheritance/interfaces/enums/FFI for dynamically typed languages;
- ad hoc polymorphism: operator/function/template/variable overloading *(better not use this last one)*.

Other features:
- not constrained to functional languages;
- support of multiple return values (as well as none);
- configurable call- and bind- semantics.

Limitations:
- (right now) *let-polymorphism* is enforced, meaning, C++ style application with
  template arguments and further re-instantiations by the callee are not possible
  (without hacking the core);
- abusing overloading without imposing additional typing constrains may
  dramatically deteriorate performance.

This generalizes Hindley-Milner (HM) type inference to something closer to
Odersky et al.'s HM(X): a single, highly permissive constraint-based inference
core, from which more restrictive/standard type systems can be derived as
constrained instances. Unlike prior implementations of this idea (typically
embedded in a single host language's compiler), *opium* is designed as a
standalone, language-agnostic tool: any target language can plug in via a thin
*parser*/*emitter* pair.

# Algorithm

The notation is partially derived from the implementation code, pardon me for inconveniences.

*Type variables* are represented with lowecase letters in math font: $a, b, c, ...$ <br/>
*Sets of type variables* are represented with capital letters in math font: $A, B, C, ...$ <br/>
*Terminal symbols* / atoms are represented with lowercase letters in monospace font: $\mathtt{x}, \mathtt{y}, \mathtt{z}, ...$ <br/>
*Constraints* are represented with bold capital letters: $\mathbf{A}, \mathbf{B}, \mathbf{C}, ...$ <br/>
*Polymorphic types* are represented with capital letters in normal font: $\text{T}, \text{U}, \text{V}, ...$ <br/>

A *polymorphic type* (or its schema), $\text{T}$, is a form
$$ \text{T} = \mathtt{t}[E, G],\ E \cap G = \varnothing $$
where $\mathtt{t}$ is a *key* (or *tag*), $E$ is an *environment set*, $G$ is a *generator set*.

A *constraint*, $\mathbf{C}$, is an algorithm that, given a set of parametric
types, type variables, and atoms, performs a set of unifications between them. <br/>
E.g., given $ \text{T} = \mathtt{t}[E, G]$, $a$, $\mathtt{x}$:
$$ \mathbf{C}(\text{T}, a, \mathtt{x})
   \Rightarrow \text{T} = \mathtt{t}[E', G'],\ a',\ \mathtt{x}. $$

*Templates* are a particular set of polymorphics type forms which are not
transformed by the constraint algorithms directly but instead are used to create
*instances* of plymorphic types which then are indeed subject to constraints.
Creating a prametric type from a given template amounts creating
an equivalent form with environment set being the template's environment set,
and generator set being a <u>copy</u> of the template's generator set:
$$ \text{T} = \mathtt{t}[E, G]; $$
$$ Inst(\text{T}) \rightarrow \text{T}' = \mathtt{t}[E, H], $$
$$ G \cap H = \varnothing. $$

Since instantions share the environment set with their template, the constraints
imposed on those instantiations do affect the template form, as well as all
other instantiations of it (that is iff the environment set is non-empty).



# Interface
- User provides a *parser* and an *emitter*.
- The *parser* converts the *input-program* into an equivalent form in the dedicated internal representation.
- The IR program is passed through the *core* to infer annotations for all applicable elements in the program code.
- The *emitter* receives the annotated IR program and translates it into the *output-program*.

## How to make it simple to use
- **The output format of the parser has minimal difference from the input format of the emitter** <br/>
  => what you put is what you get, and you only get what you put
- **No executable expressions are moved within the program** <br/>
  => no assumptions on execution model or semantics (execution is not our business, do not interfere)
- **No executable expressions are reshaped in any way** <br/>
  => no assumptions on the true meaning of the expressions (not our business)
- **Built-in support for common syntactic constructs**


# Core Pipeline
| Step | Name | Brief description |
|-|-|-|
|1. | [*infest*](#infest)     | fix input to adhere to the internal format     |
|2. | [*rename*](#rename)     | handle name-scopes and prep overloads          |
|3. | [*annotate*](#annotate) | infer annotations and resolve overloads        |

# Auxiliary Pipeline
| Step | Name | Brief description
|-|-|-|
|4. | [*scan*](#scan)         | collect all necessary template specializations |

## infest
Inject any missing placeholders for annotations.

Subjects to annotation:
- all expressions
- function arguments
- function \[template\] definition identifiers (placeholder for return types)

## rename
Rename all identifiers so that they have unique names.
Distinct members of *overload groups* are renamed with unique identifiers as well.
Additionally, after this pass:
- all overload attributes and [overload declaraionts](#overload-declaration) are consumed and removed from the IR;
- overloaded identifiers are replaced with <code>overload(OverloadedIdent, <i>PlaceholderForResolvedIdent</i>)</code>
- template identifiers are replaced with <code>template(Ident, <i>PlaceholderForSchema</i>)</code><sup>1</sup>
- all identifiers participating in overloading are added to the database as <code>overload(Overload<b>ed</b>Ident, Overload<b>ing</b>Ident).</code>
- all template identifiers are added to the database as <code>template(Ident).</code>

Note:
- all mentions of *identifiers* in the listing above refer to *renamed identifiers*
- the <code><i>PlaceholderForResolvedIdent</i></code> may be later substituted
with `template(...)`<sup>1</sup> as above.

## annotate
TODO

## scan
TODO


# Internal Representation (IR)
The internal representation uses Lisp's symbolic expressions, in particular
borrowing some names from the Scheme dialect. S-expressions are absolutely
streight-forward to process programatically (in presense of structural matching)
and trivialize addition of new syntaxes.

There are two kinds of s-expressions in IR: expressions and statements.
Expressions are things that are producing values (including none and multiple);
and all of them are subject to typing.
Statements are the things that do not produce values, and are thus not subject
to typing; it becomes a non-trivilal choice to make when facing syntaxes like
*if-then\[-else\]*, where the two branches may or may not be required to produce
values of consistent types as their results.

**Index:**
- Expressions:
  - [literal](#literal)
  - [identifier](#identifier)
  - [if-then-else](#if-then-else-expression)
  - [if-then](#if-then-expression)
  - [application](#application)
- Statements:
  - [if-then-else](#if-then-else-statement)
  - [if-then](#if-then-statement)
  - [overload declaration](#overload-declaration)
  - [recursive variable definition](#recursive-variable-definition)
  - [recursive function definition](#recursive-function-definition)
  - [recursive function template definition](#recursive-function-template-definition)


## Expressions
### literal
Defined by predicate `literal(@Term, @Type)` to be supplied by a user.
| | Semantics |
|-|-|
| infest | ensure annotation placeholder |
| rename | - |
| annotate | type annotation according to `literal/2` |
| scan | if template identifier, register required specialization |


### identifier
All atoms that are not literals.
| | Semantics |
|-|-|
| infest | ensure annotation placeholder |
| rename | replace with associated unique identifier |
| annotate | infer type annotation |
| scan | if template identifier, register required specialization |

### if-then-else-expression
Syntax:  `(eif <cond-expr> <then-expr> <else-expr>)`
| | Semantics |
|-|-|
| infest | ensure annotation placeholder and propagate through |
| rename | propagate through |
| annotate | TODO |
| scan | propagate through |

### if-then-expression
Syntax: `(eif <cond-expr> <then-expr>)`
| | Semantics |
|-|-|
| infest | ensure annotation placeholder and propagate through |
| rename | propagate through |
| annotate | TODO |
| scan | propagate through |

### application
Syntax: `(<func-expr> <arg-expr> ...*)`
| | Semantics |
|-|-|
| infest | ensure annotation placeholder and propagate through |
| rename | propagate through |
| annotate | TODO |
| scan | propagate through |

## Statements
### if-then-else-statement
Syntax: `(sif <cond-expr> <then-stmt> <else-stmt>)`

### if-then-statement
Syntax: `(sif <cond-expr> <then-stmt>)`

### overload declaration

### recursive variable definition
Syntax: `[attrs] (define <ident> <stmt> ...*)`

### recursive function definition
Syntax: `[attrs] (define (<ident> <arg-ident> ...*) <stmt> ...*)`

### recursive function template definition
Syntax: `[attrs] (template (<ident> <arg-ident> ...*) <stmt> ...*)`


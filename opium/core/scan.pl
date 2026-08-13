:- op(100, xfx, ':').
:- op(100, xfx, '=>').


% ------------------------------------------------------------------------------
%                           TRACING (FOR DEBUG)
%
%es(X) :- write(">>> "), write(es(X)), nl, fail.
%ss(X) :- write(">>> "), write(ss(X)), nl, fail.


% ------------------------------------------------------------------------------
%                             PRIMITIVES
%
sinst(Ident/Schema) :- !,
  must(instance(Ident/Schema, Define), "sinst(instance)"),
  ensure_asserted(specialization(Ident/Schema, Define)),
  Define = [_defing, _sign |Body],
  maplist(ss, Body).
sinst(_).

% ------------------------------------------------------------------------------
%                             EXPRESSIONS
%
% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                              <literal>
%
es(Literal:_) :- literal(Literal, _), !.

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                               <ident>
%
es(Ident:T) :- atom(Ident), !, sinst(T).

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                          <overload-ident>
%
es(overload(_, _):T) :- !, sinst(T).

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                          <template-ident>
%
es(template(Ident, Schema):T) :- !,
  must(T == Ident/Schema, "es(template-ident)"),
  sinst(T).

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                 (eif <cond-expr> <then-expr> <else-expr>)
%
es([eif, Cond, Then, Else]:Ts) :- !,
  maplist(sinst, Ts),
  must(es(Cond), "es(eif-then-else/cond)"),
  must(es(Then), "es(eif-then-else/then)"),
  must(es(Else), "es(eif-then-else/else)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                     (eif <cond-expr> <then-expr>)
%
es([eif, Cond, Then]:Ts) :- !,
  maplist(sinst, Ts),
  must(es(Cond), "es(eif-then-else/cond)"),
  must(es(Then), "es(eif-then-else/then)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                          (<func-expr> <arg-expr> ...*)
%
es([Ident|Args]:Ts) :- !,
  maplist(sinst, Ts),
  must(maplist(es, [Ident|Args]), "es(fncall/?)").



% ------------------------------------------------------------------------------
%                              STATEMENTS
%
% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                 (sif <cond-expr> <then-stmt> <else-stmt>)
%
ss([sif, Cond, Then, Else]) :- !,
  must(es(Cond), "es(sif-then-else/cond)"),
  must(ss(Then), "es(sif-then-else/then)"),
  must(ss(Else), "es(sif-then-else/else)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                 (sif <cond-expr> <then-expr>)
%
ss([sif, Cond, Then]) :- !,
  must(es(Cond), "es(sif-then/cond)"),
  must(ss(Then), "es(sif-then/then)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         (overload <ident> (<ident> ...*))
%
ss([overload, Alias, Idents]) :- !.

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         (define <ident> <stmt> ...+)
%
ss([define, Ident:_ |Body]) :- !,
  must(maplist(ss, Body), "ss(define-ident/body)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         (define <sign> <stmt> ...+)
%
ss([define, [Ident:_|Parms] |Body]) :- !,
  must(maplist(ss, Body), "ss(define-func/body)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         (template <sign> <stmt> ...+)
%
ss([template, [Ident:_|Parms]:_ | Body]) :- !.


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                                <expr>
%
ss(X) :- es(X).


% ------------------------------------------------------------------------------
%                           INVALID INPUT HANDLERS
%
ss(E) :- throw(transformation_error(ss(E))).
es(E) :- throw(transformation_error(es(E))).


%-------------------------------------------------------------------------------
%                                  MAIN
%
scan(Input) :-
  info("collecting used specializations        ..."),
  (
    maplist(ss, Input) -> infonl("done");
    infonl("failure"), throw(transformation_error(scan('...')))
  ).


:- dynamic(syntax/1).

:- op(100, xfx, ':').
:- op(100, xfx, '=>').


ewrite(Term) :- write(emitter_output, Term).
ewriteq(Term) :- writeq(emitter_output, Term).
enl :- nl(emitter_output).

eindent(Lvl) :-
  Lvl =:= 0 -> true;
  ewrite(" "), eindent(Lvl - 1).
  

% ------------------------------------------------------------------------------
%                           TRACING (FOR DEBUG)
%
%ee(I, X) :- write(">>> "), write(ee(X)), nl, fail.
%se(I, X) :- write(">>> "), write(se(X)), nl, fail.


% ------------------------------------------------------------------------------
%                             EXPRESSIONS
%
% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                              <literal>
%
ee(I, Literal:T) :- literal(Literal, Ty), Ty == T, !, ewriteq(Literal).

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                              <ident>
%
ee(I, Ident:T) :- atom(Ident), !,
  (
    var(T) -> throw(typecheck_error('NONDET_identifier'(Ident)));
    % first-order function
    T = _ / _ -> ewrite(Ident);
    T = _ => _ -> ewrite(Ident);
    % basic type
    type(T) -> ewrite(Ident);
    % syntax
    syntax(Ident) -> ewrite(Ident);
    % ...
    throw(transformation_error(what_is_this(T)))
  ).

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         <overload-ident>
%
ee(I, overload(_, Ident):T) :- !, ee(I, Ident:T).

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         <template-ident>
%
ee(I, template(Ident, Schema):T) :- !, write_specname(T).

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                 (eif <cond-expr> <then-expr> <else-expr>)
%
ee(I, [eif, Cond, Then, Else]:_) :- !,
  ewrite("(if "), ee(I+4, Cond), enl,
  eindent(I+4), ee(I+4, Then), enl,
  eindent(I+4), ee(I+4, Else), ewrite(")").


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                     (eif <cond-expr> <then-expr>)
%
ee(I, [eif, Cond, Then]:_) :- !,
  ewrite("(if "), ee(I+4, Cond), enl,
  eindent(I+4), ee(I+4, Then), ewrite(")").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                          (<ident> <arg> ...*)
%
ee(I, [Ident|Args]:_) :- !,
  ewrite("("), eelis(I+2, [Ident|Args]), ewrite(")").


eelis(_, []).
eelis(I, [H]) :- !, ee(I, H).
eelis(I, [H|T]) :- ee(I, H), ewrite(" "), eelis(I, T).



% ------------------------------------------------------------------------------
%                              STATEMENTS
%
% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                 (sif <cond-expr> <then-expr> <else-expr>)
%
se(I, [sif, Cond, Then, Else]) :- !,
  eindent(I), ewrite("(if "), ee(I+4, Cond), enl,
  se(I+4, Then), enl,
  se(I+4, Else), ewrite(")").


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                     (sif <cond-expr> <then-expr>)
%
se(I, [sif, Cond, Then]) :- !,
  eindent(I), ewrite("(if "), ee(I+4, Cond), enl,
  se(I+4, Then), ewrite(")").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         (overload <ident> (<ident> ...+))
%
se(I, [overload | _]) :- !.

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                  (define <ident> <stmt> ...+)
%
se(I, [define, Ident:_ | Body]) :- atom(Ident), !,
  eindent(I), ewrite("(define "), ewrite(Ident), enl,
  selis(I+2, Body), ewrite(")").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                  (define (<ident> <parm> ...*) <stmt> ...+)
%
se(I, [define, [Ident|Args] | Body]) :- !,
  eindent(I), ewrite("(define ("), eparmlis([Ident|Args]), ewrite(")"), enl,
  selis(I+2, Body), ewrite(")").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         (template <sign> <stmt> ...+)
%
se(I, [template, [Ident:_|_]:Schema | _]) :- !,
  findall((Ident/Schema):Define, specialization(Ident/Schema, Define), Specializations),
  speclis(I, Specializations).


speclis(_, []).
speclis(I, [H]) :- !, espec(I, H).
speclis(I, [H|T]) :- espec(I, H), enl, speclis(I, T).

espec(I, Sign:[define, [_| Parms] | Body]) :-
  eindent(I), ewrite("(define ("), write_specname(Sign), ewrite(" "), eparmlis(Parms), ewrite(")"), enl,
  selis(I+2, Body), ewrite(")").

eparmlis([]).
eparmlis([H:_]) :- !, ewrite(H).
eparmlis([H:_|T]) :- ewrite(H), ewrite(" "), eparmlis(T).

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                               <expr>
%
se(I, E) :- eindent(I), ee(I, E), !.


selis(I, []) :- throw(emit_error(empty_block)).
selis(I, [H]) :- !, se(I, H).
selis(I, [H|T]) :- se(I, H), enl, selis(I, T).


% ------------------------------------------------------------------------------
%                         INVALID INPUT HANDLERS
%
ee(I, E) :- throw(transformation_error(ee(E))).
se(_, E) :- throw(transformation_error(se(E))).


% ------------------------------------------------------------------------------
%                       SPECIALIZATION NAME ENCODER
%
write_specname(Sign) :-
  tabulate(genspecname_(Sign, Rename)),
  ewrite(Rename).

genspecname_(Name / _, Rename) :-
  atom_concat(Name, '/', Tmp),
  gensym(Tmp, Rename).


% ------------------------------------------------------------------------------
%                                MAIN
%
emit(Annotated) :-
  info("emitting Scheme                        ..."),
  (
    selis(0, Annotated), enl -> infonl("done");
    infonl("failure"), throw(transformation_error("emit Scheme"))
  ).


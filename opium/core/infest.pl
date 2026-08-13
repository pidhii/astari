:- op(100, xfx, ':').


% ------------------------------------------------------------------------------
%                           TRACING (FOR DEBUG)
%
%ei(X, _) :- write(">>> "), write(ei(X)), nl, fail.
%si(X, _) :- write(">>> "), write(si(X)), nl, fail.


% ------------------------------------------------------------------------------
%                   AUTOMATIC PROPAGATION OF USER ANNOtATIONS
%                            !!! DONT MOVE !!!
%
pi(P:T, IP:T) :- !, pi(P, IP:T).
ei(E:T, IE:T) :- !, ei(E, IE:T).
si(S:T, IS:T) :- !, si(S, IS:T).


% ------------------------------------------------------------------------------
%                             PRIMITIVES
%
pi(Ident, Ident:_) :- atom(Ident).


pilis([], []).
pilis([IH|IT], [OH|OT]) :-
  pi(IH, OH),
  pilis(IT, OT).


% ------------------------------------------------------------------------------
%                             EXPRESSIONS
%
% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                        <literal> or <ident>
%
ei(Ident, Ident:_) :- atomic(Ident), !.

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                 (eif <cond-expr> <then-expr> <else-expr>)
%
ei([eif, Cond, Then, Else], [eif, OCond, OThen, OElse]:_) :- !,
  must(ei(Cond, OCond), "ei(eif-then-else/cond)"),
  must(ei(Then, OThen), "ei(eif-then-else/then)"),
  must(ei(Else, OElse), "ei(eif-then-else/else)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                     (eif <cond-expr> <then-expr>)
%
ei([eif, Cond, Then], [eif, OCond, OThen]:_) :- !,
  must(ei(Cond, OCond), "ei(eif-then-else/cond)"),
  must(ei(Then, OThen), "ei(eif-then-else/then)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                          (<ident> <arg> ...*)
%
ei([Ident|Args], List:_) :- !, eilis([Ident|Args], List).


eilis([], []).
eilis([IH|IT], [OH|OT]) :- ei(IH, OH), eilis(IT, OT).


% ------------------------------------------------------------------------------
%                              STATEMENTS
%
% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                            (overload ...)
%
si([overload|Args], [overload|Args]) :- !.

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                 (sif <cond-expr> <then-expr> <else-expr>)
%
si([sif, Cond, Then, Else], [sif, OCond, OThen, OElse]) :- !,
  must(ei(Cond, OCond), "ei(sif-then-else/cond)"),
  must(si(Then, OThen), "ei(sif-then-else/then)"),
  must(si(Else, OElse), "ei(sif-then-else/else)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                   (sif <cond-expr> <then-expr>)
%
si([sif, Cond, Then], [sif, OCond, OThen]) :- !,
  must(ei(Cond, OCond), "ei(sif-then/cond)"),
  must(si(Then, OThen), "ei(sif-then/then)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                 (overload <ident> (<ident> ...+))
%
si([overload|Args], [overload|Args]) :- !.

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                    (define <ident> <stmt> ...+)
%
si([define, Ident | Body], [define, IIdent | IBody]) :- atom(Ident), !,
  must(pi(Ident, IIdent), "si(define-ident/ident)"),
  must(silis(Body, IBody), "si(define-ident/body)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                  (define (<ident> <parm> ...*) <stmt> ...+)
%
si([define, [Ident|Args] | Body], [define, [IIdent|IArgs] | IBody]) :- !,
  must(pi(Ident, IIdent), "si(define-func/ident)"),
  must(pilis(Args, IArgs), "si(define-func/parms)"),
  must(silis(Body, IBody), "si(define-func/body)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         (template <sign> <stmt> ...+)
%
si([template, Sign | Body], [template, ISign:_ | IBody]) :- !,
  must(pilis(Sign, ISign), "si(template-func/sign)"),
  must(silis(Body, IBody), "si(template-func/body)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                               <expr>
%
si(E, OE) :- ei(E, OE), !.


silis([], []).
silis([IH|IT], [OH|OT]) :-
  si(IH, OH),
  silis(IT, OT).


% ------------------------------------------------------------------------------
%                         INVALID INPUT HANDLERS
%
ei(E, _) :- throw(transformation_error(ei(E))).
si(E, _) :- throw(transformation_error(si(E))).


% ------------------------------------------------------------------------------
%                                MAIN
%
infest(FlatIr, FlatIrWithPlaceholders) :-
  info("injecting placeholders for annotations ..."),
  (
    silis(FlatIr, FlatIrWithPlaceholders) -> infonl("done");
    infonl("failure"), throw(typecheck_error(infest))
  ).


:- op(100, xfy, ':').


rnm_genname(Ident, RIdent) :-
  must(var(RIdent), rnm_genname('non-var')),
  atom_concat(Ident, '$', Tmp),
  gensym(Tmp, RIdent).


% ------------------------------------------------------------------------------
%                           TRACING (FOR DEBUG)
%
%rnm_expr(X, _/Alist, _) :-
  %write(">>> "), write(rnm_expr(X)), nl,
  %write(" ai "), write(Alist), nl, fail.
%rnm_stmt(X, R, _/Alist, _) :-
  %write(">>> "), write(rnm_stmt(X)), nl,
  %write(" ai "), write(Alist), nl, fail.


% ------------------------------------------------------------------------------
%                             PRIMITIVES
%
rident(Ident:RIdent:Kind, Alist) :-
  once(member(Ident:RIdent:Kind, Alist);
       extern(Ident, RIdent, _), Kind = defn).


% Find rename of an identifier skipping overload groups (in case identifier
% happens to match with some overload group)
rident_noov(Alist, Ident, RIdent) :-
  (member(Ident:RIdent, Alist); extern(Ident, RIdent, _)),
  \+ overloaded(RIdent),
  !.


% ------------------------------------------------------------------------------
%                             EXPRESSIONS
%
% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                              <literal>
%
rnm_expr(Literal:T, [Literal:T|Z]/Alist, Z/Alist) :- literal(Literal, _), !.

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                               <ident>
%
rnm_expr(Ident:T, A/Alist, Z/Alist) :- atom(Ident), !,
  must(rident(Ident:RIdent:Kind, Alist), rnm_expr(ident/unknown(Ident))),
  ( Kind == ovrl ->
    findall(I, overload(RIdent, I), Is),
    A = [overload(Is, _):T|Z]
  ; Kind == temp ->
    A = [instance(RIdent):T|Z]
  ; must(Kind == defn, rnm_expr(ident/invalid_kind(Ident,Kind))),
    A = [RIdent:T|Z]
  ).


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                 (eif <cond-expr> <then-expr> <else-expr>)
%
rnm_expr([eif, Cond, Then, Else]:T, [[eif, RCond, RThen, RElse]:T|Z]/Alist, Z/Alist) :- !,
  must(rnm_expr(Cond, [RCond]/Alist, []/Alist), "rnm_expr(eif-then-else/cond)"),
  must(rnm_expr(Then, [RThen]/Alist, []/Alist), "rnm_expr(eif-then-else/then)"),
  must(rnm_expr(Else, [RElse]/Alist, []/Alist), "rnm_expr(eif-then-else/else)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                     (eif <cond-expr> <then-expr>)
%
rnm_expr([eif, Cond, Then]:T, [[eif, RCond, RThen]:T|Z]/Alist, Z/Alist) :- !,
  must(rnm_expr(Cond, [RCond]/Alist, []/Alist), "rnm_expr(eif-then-else/cond)"),
  must(rnm_expr(Then, [RThen]/Alist, []/Alist), "rnm_expr(eif-then-else/then)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                          (<func-expr> <arg-expr> ...*)
%
rnm_expr([Ident|Args]:T, [List:T|Z]/Alist, Z/Zlist) :- !,
  must(rnm_exprlist([Ident|Args], List/Alist, []/Zlist), "rnm_expr(fncall/?)").


rnm_exprlist([], A, A).
rnm_exprlist([H|T], A, Z) :-
  rnm_expr(H, A, B),
  rnm_exprlist(T, B, Z).


% ------------------------------------------------------------------------------
%                              ATTRIBUTES (parsing)
%
rattr(overload(Ident)) --> [[overload, Ident]], !.


rattrlis([H|T]) --> rattr(H), !, rattrlis(T).
rattrlis([]) --> !.


% ------------------------------------------------------------------------------
%                              STATEMENTS
%
% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         (declare <ident>)
%
rnm_stmt([[declare, Ident] |IZ], IZ, A/Alist, A/Zlist) :- !,
  must(member(decl(Ident, RIdent), Alist), rnm_stmt('declare/no-such-ident'(Ident))),
  Zlist = [Ident:RIdent:defn |Alist].

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         (declaret <ident>)
%
rnm_stmt([[declaret, Ident] |IZ], IZ, A/Alist, A/Zlist) :- !,
  must(member(decl(Ident, RIdent), Alist), rnm_stmt('declaret/no-such-ident'(Ident))),
  Zlist = [Ident:RIdent:temp |Alist].


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         (overload <ident> (<ident> ...*))
%
rnm_stmt([[overload, Alias, Idents] |IZ], IZ, A/Alist, A/Zlist) :- !,
  maplist(rident_noov(Alist), Idents, RIdents),
  rnm_requireoverloadgroup(Alias, RAlias, Alist, Zlist),
  maplist(rnm_addoverload(RAlias), RIdents).

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                 (sif <cond-expr> <then-stmt> <else-stmt>)
%
rnm_stmt([[sif, Cond, Then, Else]|Rem], Rem, [[sif, RCond, RThen, RElse]|Z]/Alist, Z/Alist) :- !,
  must(rnm_expr(Cond, [RCond]/Alist, []/Alist), "rnm_expr(sif-then-else/cond)"),
  must(rnm_stmt([Then], [], [RThen]/Alist, []/Alist), "rnm_expr(sif-then-else/then)"),
  must(rnm_stmt([Else], [], [RElse]/Alist, []/Alist), "rnm_expr(sif-then-else/else)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                 (sif <cond-expr> <then-expr>)
%
rnm_stmt([[sif, Cond, Then]|Rem], Rem, [[sif, RCond, RThen]|Z]/Alist, Z/Alist) :- !,
  must(rnm_expr(Cond, [RCond]/Alist, []/Alist), "rnm_expr(sif-then/cond)"),
  must(rnm_stmt([Then], [], [RThen]/Alist, []/Alist), "rnm_expr(sif-then/then)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         (define <ident> <stmt> ...+)
%
rnm_stmt([[define, Ident:T |Body] |IZ], IZ, A/Alist, Z/Zlist) :- atom(Ident), !,
  must(member(decl(Ident, RIdent), Alist), "rnm_stmt(define-ident/bind-decl)"),
  must(rnm_populate(Ident:RIdent:defn, Alist, Zlist), "rnm_stmt(define-ident/populate)"),
  must(rnm_stmtblock(Body, RBody/Zlist, []/_), "rnm_stmt(define-ident/body)"),
  A = [[define, RIdent:T |RBody] | Z].

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         (define <sign> <stmt> ...+)
%
rnm_stmt([[define, [Ident:T|Parms] |Body] |IZ], IZ, A/Alist, Z/Zlist) :- !,
  % Generate rename and bind with declaration
  %rnm_genname(Ident, RIdent),
  must(member(decl(Ident, RIdent), Alist), "rnm_stmt(define-func/bind-decl)"),
  % Populate in alist
  rnm_populate(Ident:RIdent:defn, Alist, Zlist),
  must(rnm_parmlist(Parms, RParms, Zlist, Flist), "rnm_stmt(define-func/parms)"),
  must(rnm_stmtblock(Body, RBody/[scope|Flist], []/_), "rnm_stmt(define-func/body)"),
  A = [[define, [RIdent:T|RParms] | RBody] | Z].

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         (template(<typarm-list>) <sign> <stmt> ...+)
%
rnm_stmt([[template(TypeParms), [Ident:T|Parms]:_ | Body] |IZ], IZ, A/Alist, Z/Zlist) :- !,
  % Generate rename and bind with declaration
  %rnm_genname(Ident, RIdent),
  must(member(decl(Ident, RIdent), Alist), "rnm_stmt(template-func/bind-decl)"),       
  % Populate in alist
  rnm_populate(Ident:RIdent:temp, Alist, Zlist),
  must(rnm_parmlist(Parms, RParms, Zlist, Flist), "rnm_stmt(template-func/parms)"),
  must(rnm_stmtblock(Body, RBody/[scope|Flist], []/_), "rnm_stmt(template-func/body)"),
  A = [[template(TypeParms), [RIdent:T|RParms]:_ | RBody] |Z].


%%%
% rnm_parmlist(InParms, OutParms, InAlist, OutAlist)
%
% Rename and populate a list of parameters.
%
rnm_parmlist([], [], Alist, Alist).
rnm_parmlist([H:Ty|T], [RH:Ty|RT], Alist, Zlist) :-
  rnm_genname(H, RH),
  rnm_parmlist(T, RT, [H:RH:defn|Alist], Zlist).


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                                <expr>
%
rnm_stmt([E|IZ], IZ, A, Z) :-
  rnm_expr(E, A, Z), !.


%%%
% rnm_stmtblock(Stmts, Tape/InAlist, TapeTail/OutAlist)
%
% Primary routine to process a block of statements.
%
% Pre-inject forward-declarations for all definitions and templates before
% proceeding with the actual transformations.
%
rnm_stmtblock([], A, A).
rnm_stmtblock(Stmts, A/Alist, Z/Zlist) :-
  rnm_blockdecls(Stmts, Decls),
  append(Decls, Alist, Blist),
  rnm_stmtlist(Stmts, A/Blist, Z/Zlist).

%%%
% rnm_stmtlist(Stmts, Tape/InAlist, TapeTail/OutAlist)
%
% Transform a list of statements.
%
rnm_stmtlist([], A, A).
rnm_stmtlist(Stmts, A, Z) :-
  rnm_stmt(Stmts, R, A, B),
  rnm_stmtlist(R, B, Z).

%%%
% rnm_defnident(+Stmt, ?Ident)
%
% Extract identifiers from definitions and templates. Fails for all other
% statements.
%
rnm_defnident([template(_), [Ident:_ |_]:_ |_], Ident) :- !.
rnm_defnident([define, [Ident:_ |_] |_], Ident) :- !.
rnm_defnident([define, Ident:_ |_], Ident) :- !, must(atom(Ident), 'rnm_defnident(define-ident)').

rnm_blockdecls([], []).
rnm_blockdecls([H|T], Decls0) :-
  ( rnm_defnident(H, Ident) ->
    rnm_genname(Ident, RIdent),
    Decls0 = [decl(Ident, RIdent)|Decls1],
    rnm_blockdecls(T, Decls1)
  ; rnm_blockdecls(T, Decls0)
  ).

% ------------------------------------------------------------------------------
%                           INVALID INPUT HANDLERS
%
rnm_stmt(E, _, _, _) :- throw(transformation_error(rnm_stmt(E))).
rnm_expr(E, _, _) :- throw(transformation_error(rnm_expr(E))).


% ------------------------------------------------------------------------------
%                                 MISC
%
%%%
% rnm_scopemember(X, Alist)
%
% True if X is found in Alist before the first scope-cut.
%
rnm_scopemember(_, [scope|_]) :- !, fail.
rnm_scopemember(X, [H|T]) :-
  X = H;
  rnm_scopemember(X, T).
  

% Create overload group in alist unles already present
rnm_requireoverloadgroup(OvIdent, ROvIdent, Alist, Zlist) :-
  ( % - alias rename is already registered => nop
    rnm_scopemember(OvIdent:ROvIdent:ovrl, Alist) ->
    must(overloaded(ROvIdent), rnm_requireoverloadgroup('integrity-error')),
    Zlist = Alist
  ; % - otherwise => start the overload group
    rnm_genname(OvIdent, ROvIdent),
    ensure_asserted(overloaded(ROvIdent)),
    Zlist = [OvIdent:ROvIdent:ovrl|Alist]
  ).

rnm_addoverload(RAlias, RIdent) :-
  must(overloaded(RAlias), add_overload),
  ensure_asserted(overload(RAlias, RIdent)).

rnm_populate(Ident:RIdent:Kind, Alist, [Ident:RIdent:Kind|Alist]).


%-------------------------------------------------------------------------------
%                                  MAIN
%
rename(Input, Output) :-
  info("renaming identifiers                   ..."),
  (
    rnm_stmtblock(Input, Output/[], []/_) -> infonl("done");
    infonl("failure"), throw(transformation_error(rename('...')))
  ).


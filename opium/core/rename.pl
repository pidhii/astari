:- op(100, xfy, ':').


genname(Ident, RIdent) :-
  atom_concat(Ident, '$', Tmp),
  gensym(Tmp, RIdent).


% ------------------------------------------------------------------------------
%                           TRACING (FOR DEBUG)
%
%er(X, _/Alist, _) :-
  %write(">>> "), write(er(X)), nl,
  %write(" ai "), write(Alist), nl, fail.
%sr(X, R, _/Alist, _) :-
  %write(">>> "), write(sr(X)), nl,
  %write(" ai "), write(Alist), nl, fail.


% ------------------------------------------------------------------------------
%                             PRIMITIVES
%
rident(Ident:RIdent, Alist) :-
  once(member(Ident:RIdent, Alist); extern(Ident, RIdent, _)).


% Find rename of an identifier skipping overload groups (in case identifier
% happens to match with some overload group)
rident_noov(Ident:RIdent, Alist) :-
  (member(Ident:RIdent, Alist); extern(Ident, RIdent, _)),
  \+ overloaded(RIdent),
  !.


renamelis([], [], _).
renamelis([IH|IT], [RH|RT], Alist) :-
  rident(IH:RH, Alist),
  renamelis(IT, RT, Alist).


% Find renames excluding overloads (see rident_noov)
renamelis_noov([], [], _).
renamelis_noov([IH|IT], [RH|RT], Alist) :-
  rident_noov(IH:RH, Alist),
  renamelis_noov(IT, RT, Alist).


% ------------------------------------------------------------------------------
%                             EXPRESSIONS
%
% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                              <literal>
%
er(Literal:T, [Literal:T|Z]/Alist, Z/Alist) :- literal(Literal, _), !.

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                               <ident>
%
er(Ident:T, A/Alist, Z/Alist) :- atom(Ident), !,
  must(rident(Ident:RIdent:Kind, Alist), er(ident/unknown(Ident))),
  ( Kind == ovrl ->
    findall(I, overload(RIdent, I), Is),
    A = [overload(Is, _):T|Z]
  ; Kind == temp ->
    A = [instance(RIdent, _):T|Z]
  ; must(Kind == def, er(ident/invalid_kind(Ident,Kind))),
    A = [RIdent:T|Z]
  ).


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                 (eif <cond-expr> <then-expr> <else-expr>)
%
er([eif, Cond, Then, Else]:T, [[eif, RCond, RThen, RElse]:T|Z]/Alist, Z/Alist) :- !,
  must(er(Cond, [RCond]/Alist, []/Alist), "er(eif-then-else/cond)"),
  must(er(Then, [RThen]/Alist, []/Alist), "er(eif-then-else/then)"),
  must(er(Else, [RElse]/Alist, []/Alist), "er(eif-then-else/else)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                     (eif <cond-expr> <then-expr>)
%
er([eif, Cond, Then]:T, [[eif, RCond, RThen]:T|Z]/Alist, Z/Alist) :- !,
  must(er(Cond, [RCond]/Alist, []/Alist), "er(eif-then-else/cond)"),
  must(er(Then, [RThen]/Alist, []/Alist), "er(eif-then-else/then)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                          (<func-expr> <arg-expr> ...*)
%
er([Ident|Args]:T, [List:T|Z]/Alist, Z/Zlist) :- !,
  must(erlis([Ident|Args], List/Alist, []/Zlist), "er(fncall/?)").


erlis([], A, A).
erlis([H|T], A, Z) :-
  er(H, A, B),
  erlis(T, B, Z).


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
sr([[declare, Ident] |IZ], IZ, A/Alist, A/Zlist) :- !,
  must(member(decl(Ident, RIdent), Alist), sr('declare/no-such-ident'(Ident))),
  Zlist = [Ident:RIdent:def |Alist].

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         (declaret <ident>)
%
sr([[declaret, Ident] |IZ], IZ, A/Alist, A/Zlist) :- !,
  must(member(decl(Ident, RIdent), Alist), sr('declaret/no-such-ident'(Ident))),
  Zlist = [Ident:RIdent:temp |Alist].


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         (overload <ident> (<ident> ...*))
%
sr([[overload, Alias, Idents] |IZ], IZ, A/Alist, A/Zlist) :- !,
  renamelis_noov(Idents, RIdents, Alist),
  rensure_overloaded(Alias, RAlias, Alist, Zlist),
  maplist(rensure_overloads(RAlias), RIdents).

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                 (sif <cond-expr> <then-stmt> <else-stmt>)
%
sr([[sif, Cond, Then, Else]|Rem], Rem, [[sif, RCond, RThen, RElse]|Z]/Alist, Z/Alist) :- !,
  must(er(Cond, [RCond]/Alist, []/Alist), "er(sif-then-else/cond)"),
  must(sr([Then], [], [RThen]/Alist, []/Alist), "er(sif-then-else/then)"),
  must(sr([Else], [], [RElse]/Alist, []/Alist), "er(sif-then-else/else)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                 (sif <cond-expr> <then-expr>)
%
sr([[sif, Cond, Then]|Rem], Rem, [[sif, RCond, RThen]|Z]/Alist, Z/Alist) :- !,
  must(er(Cond, [RCond]/Alist, []/Alist), "er(sif-then/cond)"),
  must(sr([Then], [], [RThen]/Alist, []/Alist), "er(sif-then/then)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         (define <ident> <stmt> ...+)
%
sr(IA, IZ, A/Alist, Z/Zlist) :- 
  rattrlis(Attrs, IA, IB), IB = [[define, Ident:T |Body] |IZ], atom(Ident),
  !,
  % Generate rename and bind with declaration
  %genname(Ident, RIdent),
  must(member(decl(Ident, RIdent), Alist), "sr(define-ident/bind-decl)"),
  % Populate in alist
  %rpopulate(Attrs, Ident:RIdent:def, Alist, Zlist),
  must(srblk(Body, RBody/Zlist, []/_), "sr(define-ident/body)"),
  A = [[define, RIdent:T |RBody] | Z].

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         (define <sign> <stmt> ...+)
%
sr(IA, IZ, A/Alist, Z/Zlist) :-
  rattrlis(Attrs, IA, IB), IB = [[define, [Ident:T|Parms] |Body] |IZ],
  !,
  % Generate rename and bind with declaration
  %genname(Ident, RIdent),
  must(member(decl(Ident, RIdent), Alist), "sr(define-func/bind-decl)"),
  % Populate in alist
  rpopulate(Attrs, Ident:RIdent:def, Alist, Zlist),
  must(rparmlis(Parms, RParms, Zlist, Flist), "sr(define-func/parms)"),
  must(srblk(Body, RBody/[scope|Flist], []/_), "sr(define-func/body)"),
  A = [[define, [RIdent:T|RParms] | RBody] | Z].

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         (template <sign> <stmt> ...+)
%
sr(IA, IZ, A/Alist, Z/Zlist) :-
  rattrlis(Attrs, IA, IB), IB = [[template, [Ident:T|Parms]:_ | Body] |IZ],
  !,
  % Generate rename and bind with declaration
  %genname(Ident, RIdent),
  must(member(decl(Ident, RIdent), Alist), "sr(template-func/bind-decl)"),       
  % Populate in alist
  rpopulate(Attrs, Ident:RIdent:temp, Alist, Zlist),
  must(rparmlis(Parms, RParms, Zlist, Flist), "sr(template-func/parms)"),
  must(srblk(Body, RBody/[scope|Flist], []/_), "sr(template-func/body)"),
  A = [[template, [RIdent:T|RParms]:_ | RBody] |Z].


rparmlis([], [], Alist, Alist).
rparmlis([H:Ty|T], [RH:Ty|RT], Alist, Zlist) :-
  genname(H, RH),
  rparmlis(T, RT, [H:RH|Alist], Zlist).


scopemember(_, [scope|_]) :- !, fail.
scopemember(X, [H|T]) :-
  X = H;
  scopemember(X, T).
  

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                                <expr>
%
sr([E|IZ], IZ, A, Z) :-
  er(E, A, Z), !.


srblk([], A, A).
srblk(L, A/Alist, Z/Zlist) :-
  rgendecls(L, Decls),
  append(Decls, Alist, Blist),
  srlis(L, A/Blist, Z/Zlist).

srlis([], A, A).
srlis(L, A, Z) :-
  sr(L, R, A, B),
  srlis(R, B, Z).

%%%
% rscandef(+Statement, ?Ident)
%
% Extract identifiers used for definitions. Fails for statements that are not
% definitions.
%
rscandef([template, [Ident:_ |_]:_ |_], Ident) :- !.
rscandef([define, [Ident:_ |_] |_], Ident) :- !.
rscandef([define, Ident:_ |_], Ident) :- !, must(atom(Ident), 'rscandef(define-ident)').

rgendecls([], []).
rgendecls([H|T], Decls0) :-
  ( rscandef(H, Ident) ->
    genname(Ident, RIdent),
    Decls0 = [decl(Ident, RIdent)|Decls1],
    rgendecls(T, Decls1)
  ; rgendecls(T, Decls0)
  ).

% ------------------------------------------------------------------------------
%                           INVALID INPUT HANDLERS
%
sr(E, _, _, _) :- throw(transformation_error(sr(E))).
er(E, _, _) :- throw(transformation_error(er(E))).


% ------------------------------------------------------------------------------
%                                 MISC
%
% Create overload group in alist unles already present
rensure_overloaded(OvIdent, ROvIdent, Alist, Zlist) :-
  ( % - alias rename is already present
    scopemember(OvIdent:ROvIdent:ovrl, Alist) ->
    must(overloaded(ROvIdent), overloaded(OvIdent)),
    Zlist = Alist
  ; genname(OvIdent, ROvIdent),
    ensure_asserted(overloaded(ROvIdent)),
    Zlist = [OvIdent:ROvIdent:ovrl|Alist]
  ).

rensure_overloads(RAlias, RIdent) :-
  must(overloaded(RAlias), add_overload),
  ensure_asserted(overload(RAlias, RIdent)).

rpopulate(Attrs, Ident:RIdent:Kind, Alist, Zlist) :-
  ( member(overload(OvIdent), Attrs) ->
    rensure_overloaded(OvIdent, ROvIdent, Alist, Zlist),
    rensure_overloads(ROvIdent, RIdent)
  ; Zlist = [Ident:RIdent:Kind|Alist]
  ).


%-------------------------------------------------------------------------------
%                                  MAIN
%
rename(Input, Output) :-
  info("renaming identifiers                   ..."),
  (
    srblk(Input, Output/[], []/_) -> infonl("done");
    infonl("failure"), throw(transformation_error(rename('...')))
  ).


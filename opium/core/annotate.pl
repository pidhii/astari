:- op(100, xfx, ':').
:- op(100, xfx, '=>').


type(Args => Res) :- '$typelis'(Args), '$typelis'(Res), !.

resultof(OArgs => ORes, IArgs, IRes) :- !,
  '$decaylis'(IArgs, OArgs),
  '$castlis'(ORes, IRes).


'$typelis'(L, L) :- var(L), !.
'$typelis'([]).
'$typelis'([H|T]) :- type(H), '$typelis'(T).


'$castlis'(L, L) :- var(L), !.
'$castlis'([], []).
'$castlis'([IH|IT], [OH|OT]) :-
  cast(IH, OH),
  '$castlis'(IT, OT).


'$decaylis'(L, L) :- var(L), !.
'$decaylis'([], []).
'$decaylis'([IH|IT], [OH|OT]) :-
  decay(IH, OH),
  '$decaylis'(IT, OT).



varsof(X, Vars) :- varsof(X, Vars, []).
varsoflist(L, Vars) :- varsoflist(L, Vars, []).

varsof(X) --> { var(X) }, !, [X].
varsof(X) --> { atomic(X) }, !.
varsof(Term) --> { Term =.. [_|Args] }, varsoflist(Args).

varsoflist([]) --> !.
varsoflist([H|T]) --> varsof(H), varsoflist(T).

memberq(X, [H|T]) :-
  X == H;
  memberq(X, T).


% ------------------------------------------------------------------------------
%                           TRACING (FOR DEBUG)
%
%et(InputSeq, OTypes, A/_, Z/_) :- write(">>> "), write(et(InputSeq)), nl, fail.
%st(InputSeq, OTypes, A/_, Z/_) :- write(">>> "), write(st(InputSeq)), nl, fail.


% ------------------------------------------------------------------------------
%                             EXPRESSIONS
%
% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                              <literal>
%
et(Literal:T, [T]) --> { literal(Literal, T) }, !.

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                              <ident>
%
et(Ident:IdentType, [IdentType], A/Alist, A/Alist) :- atom(Ident), !,
  must(tident(Ident:IdentType, Alist), "et(ident/untyped)").

tident(Ident:T, Alist) :-
  member(Ident:T, Alist);
  extern(_, Ident, T).


tinst(X, _) :- var(X), !, throw("tinst(NONDET)").
tinst(Key / (EnvClos, TempParms), Key / (EnvClos, TempParmsCopy)) :- !,
  copy_term(TempParms, TempParmsCopy).
tinst(X, X).


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                              <overload>
%
et(overload(Variants, Ident):T, [T], A/Alist, Z/Alist) :- !,
  ( Variants = [] -> throw("et(overload/empty)")
  ; Variants = [Ident] ->
    must(tident(Ident:T, Alist), et(overload(Variants,Ident))),
    A = Z
  ; tmakeresolvecaluselis(Ident:T, Variants, CodeVariants, Alist),
    OrClause =.. [';'|CodeVariants],
    A = [OrClause|Z]
  ).

tmakeresolveclause(Ident:Type, Variant, Clause, Alist) :-
  tident(Variant:VariantType, Alist),
  A = [Ident = Variant, Type = VariantType],
  Clause =.. [','|A].

tmakeresolvecaluselis(Ident:Type, [], [],  _).
tmakeresolvecaluselis(Ident:Type, [H|T], [CH|CT], Alist) :-
  tmakeresolveclause(Ident:Type, H, CH, Alist),
  tmakeresolvecaluselis(Ident:Type, T, CT, Alist).

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                              <instance>
%
et(instance(Ident, Schema):T, [T], A/Alist, Z/Alist) :- !,
  %must(tident(Ident:Temp, Alist), et('instance-ident'(Ident))),
  A = [tinst(Ident/Schema, T) |Z],
  must(T = Ident/Schema, et('instance-ident'/'schema')).

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                 (eif <cond-expr> <then-expr> <else-expr>)
%
et([eif, Cond, Then, Else]:R, R) --> !,
  must(et(Cond, CondR), "et(eif-then-else/cond)"),
  tput(conditional(CondR)),
  must(et(Then, R), "et(eif-then-else/then)"),
  must(et(Else, R), "et(eif-then-else/else)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                     (eif <cond-expr> <then-expr>)
%
et([eif, Cond, Then]:R, R) --> !,
  must(et(Cond, CondR), "et(eif-then/cond)"),
  tput(conditional(CondR)),
  must(et(Then, R), "et(eif-then/then)"),
  { throw(transformation_error(unimplemented("et(eif-then)"))) }.

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                          (<fn-expr> <arg-expr> ...*)
%
et([Fn|Args]:R, R) --> !,
  must(et(Fn, [FnType|_]), "et(fncall/fn)"), % resolve function identifier
  must(etlis(Args, ArgTypelists), "et(fncall/args-1)"), % convert arguments to types
  { must(maplist(listhead, ArgTypelists, ArgTypes), "et(fncall/args-2)") }, % take only the head of each arg type
  tput(tabulatex(resultof(FnType, ArgTypes, R))). % defer function call

listhead([H|_], H).

% ------------------------------------------------------------------------------
%                              STATEMENTS
%
% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                 (sif <cond-expr> <then-expr> <else-expr>)
%
st([sif, Cond, Then, Else], []) --> !,
  must(et(Cond, CondR), "st(sif-then-else/cond)"),
  tput(conditional(CondR)),
  must(st(Then, _), "st(sif-then-else/then)"),
  must(st(Else, _), "st(sif-then-else/else)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                   (sif <cond-expr> <then-expr>)
%
st([sif, Cond, Then], []) --> !,
  must(et(Cond, CondR), "st(sif-then/cond)"),
  tput(conditional(CondR)),
  must(st(Then, _), "st(sif-then/then)").

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         (overload <ident> (<ident> ...+))
%
st([overload, Alias, Idents], []) --> !,
  must(ovrllis(Alias, Idents), "st(overload)").

ovrllis(_, []) --> !.
ovrllis(Alias, [H|T]) -->
  talist_add(Alias=H),
  ovrllis(Alias, T).

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         (define <ident> <stmt> ...+)
%
st([define, Ident:IdentType |Body], []) --> { atom(Ident) }, !,
  must(stblk(Body, [BodyTypeHead|_]), "st(define-ident/body)"),
  tput(materialize(BodyTypeHead, IdentType)),
  talist_add(Ident:IdentType).

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         (define <sign> <stmt> ...+)
%
st([define, [Ident:Res | ArgIdents] | Body], []) --> !,
  % populate self
  talist_add(Ident:(ArgTypes=>Res)),
  talist_get(Zlist),
  % populate arguments
  must(tparmlis(ArgIdents, ArgTypes), "st(define-func/parms)"),
  % generate typecheck for the function body
  must(stblk(Body, Res), "st(define-func/body)"),
  % hide parameters
  talist_set(Zlist).

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                         (template <sign> <stmt> ...+)
%
st([template, [Ident:Res |ArgIdents]:Schema | Body], []) --> !,
  % Current type environment
  talist_get(Alist),
  { once(member(typeenv(E), Alist); E = []) },
  % Populate self
  %talist_add(Ident:TemplateSign),
  % Save alist state to restore it after the changes meant for body
  talist_get(Zlist),
  {
    G = Args => Res,
    Schema = (E, G),
    TemplateSign = Ident / Schema
  },
  %
  % Clause head:
  { ClauseHead = resultof(TemplateSign, Args, Res) },
  %
  % Clause body:
  % - populate function paramters in alist
  must(tparmlis(ArgIdents, Args), "st(template-func/parms)"),
  % - update type environemnt
  talist_add(typeenv([Args|E])),
  % - generate typecheck for the function body
  talist_get(Flist),
  {
    ClauseBody0 = ['$decaylis'(IArgs, Args),
                   '$castlis'(Res, IRes)
                   |ClauseBody1],
    %ClauseBody0 = ClauseBody1,
    must(stblk(Body, Res, ClauseBody1/Flist, ClauseBody2/_), "st(template-func/body)"),
    % inject automatic registration of instantiation:
    SpecialDef = [define, [Ident|ArgIdents] |Body],
    ClauseBody2 = [ensure_asserted(instance(TemplateSign, SpecialDef))],
    % convert list into conjunction
    CB =.. [',' |ClauseBody0]
  },
  %
  % Write the clause:
  tput(asserta(ClauseHead :- CB)),
  %
  % hide parameters
  talist_set(Zlist).

tparmlis([], [], A, A).
tparmlis([IH:TH|IT], [TH|TT]) --> !, talist_add(IH:TH), tparmlis(IT, TT).
tparmlis([IH|IT], [TH|TT]) --> talist_add(IH:TH), tparmlis(IT, TT).

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                                <expr>
%
st(Input, Result, A, Z) :-
  et(Input, Result, A, Z), !.


% ------------------------------------------------------------------------------
%                           INVALID INPUT HANDLERS
%
st(InputSeq, _, _, _) :- throw(transformation_error(st(InputSeq))).
et(InputSeq, _, _, _) :- throw(transformation_error(et(InputSeq))).



% ------------------------------------------------------------------------------
%                                  UTILS
%
talist_add(X, A/Alist, A/Zlist) :- Zlist = [X|Alist].
talist_get(Alist, A/Alist, A/Alist).
talist_set(Alist, A/_, A/Alist).

tput(X, [X|Z]/Alist, Z/Alist).


etlis([], []) --> !.
etlis([H|T], [RH|RT]) --> et(H, RH), etlis(T, RT).


%tscanst([template(_), [Ident:_ |_] |_], [Ident:(Ident/_Schema)]).
%tscanblk([H|T], [RH|RT]) --> st(H, RH), stlis(T, RT).


stblk([], []) --> !.
stblk([St], R) --> !, st(St, R).
stblk([H|T], R) --> st(H, _), stblk(T, R).


%-------------------------------------------------------------------------------
%                                  MAIN
%
annotate(Input, Output) :-
  info("generating typer script                ..."),
  (
    stblk(Input, _, TypeCheck/[], []/_) -> infonl("done");
    infonl("failure"), throw(typecheck_error(generate_typecheck_script))
  ),
  when(veryverbose, (
    write("Typecheck script:"), nl,
    write("```"), nl,
    printlist(TypeCheck), write("."), nl,
    write("```"), nl
  )),

  info("running typer                          ..."),
  G =.. [',' | TypeCheck],
  findall((Input, Insts),
          (
            G,
            findall(instance(K, D), instance(K, D), Insts)
          ),
          Sols),
  (
    Sols = [] ->
      infonl("failure"),
      throw(typecheck_error(no_solutions));
    Sols = [(Output, Insts)] ->
      infonl("done"),
      assertall(Insts);
    % else -> 
      infonl("failure"),
      printsols(Sols),
      throw(typecheck_error(ambiguous))
  ).

assertall([]).
assertall([H|T]) :- asserta(H), assertall(T).


printsols(Sols) :-
  printsols(1, Sols).

printsols(_, []).
printsols(I, [(Annotated, _)|T]) :-
  write("Solution #"), write(I), nl,
  printlist(2, Annotated), nl,
  II is I + 1,
  printsols(II, T).

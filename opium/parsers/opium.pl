:- dynamic(op/3).

:- op(100, xfx, ':').
:- op(100, xfx, '=>').




cast(T, T).
decay(T, T).
materialize(X, X).
conditional([bool|_]).

literal(X, num) :- number(X).
literal(X, str) :- string(X).

type(bool).
type(num).
type(str).
type(list(T)) :- type(T).

delimiter('(').
delimiter(')').
delimiter('{').
delimiter('}').
delimiter(',').

token(extern).
token(as).
token('->').
token(infix).

keyword(X) :- delimiter(X), !.
keyword(X) :- token(X), !.

op(_, _, '+').
op(_, _, '==').


syntax('set!').
resultof('set!', [T, T], []) :- !.
extern('set!', 'set!', 'set!').


% ------------------------------------------------------------------------------
%                           TRACING (FOR DEBUG)
%
%ap(_, A, Z) :- write(">>> "), writeq(ap(A, Z)), nl, fail.
%ep(_, A, Z) :- write(">>> "), writeq(ep(A, Z)), nl, fail.
%xp(_, A, Z) :- write(">>> "), writeq(xp(A, Z)), nl, fail.
%sp(_, _, A, Z) :- write(">>> "), writeq(sp(A, Z)), nl, fail.
%tp(_, _, A, Z) :- write(">>> "), writeq(tp(A, Z)), nl, fail.


% ------------------------------------------------------------------------------
%                             PRIMITIVES
%
ip(Ident) --> [Ident], { atom(Ident), \+ keyword(Ident) }, !.

ip(Ident) --> [q(Ident)], { atom(Ident) }, !.

% <type> = <ident>
typ(Type) --> ip(Type), !.
% <type> = <typevar>
typ(Type) --> [q(Type)], { var(Type) }, !.
% <type> = '(' <type> ...* ')' '->' <type>
typ('=>'(ArgTypes, [RetType])) -->
  ['('], pcomalis(typ, ')', ArgTypes), ['->'], typ(RetType), !.
% <type> = '(' <type> ...* ')' '->' '(' <type> ...* ')'
typ('=>'(ArgTypes, RetTypes)) -->
  ['('], pcomalis(typ, ')', ArgTypes), ['->', '('], pcomalis(typ, ')', RetTypes), !.

% <typelist> = <type>
tylp([Type]) --> typ(Type), !.
% <typelist> = '(' <type> ...* ')'
tylp(Typelist) --> ['('], pcomalis(typ, ')', Typelist).

% <parm> = <ident> <type>
pp(Ident:Type) --> ip(Ident), typ(Type), !.
% <parm> = <ident>
pp(Ident) --> ip(Ident), !.

% <typed-parm> = <ident> <type>
typp(Ident:Type) --> ip(Ident), typ(Type), !.


% <body> = '=' <xexpr>
% <body> = '{' <stmt> ...* '}'
pbody([XExpr]) --> ['='], xp(XExpr), !.
pbody(Block) --> ['{'], splis(Block, [], '}').


% ------------------------------------------------------------------------------
%                                ATOMS
%
% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                              <literal>
%
ap(Literal) --> [Literal], {literal(Literal, _)}, !.

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                               <ident>
%
ap(Ident) --> ip(Ident), !.

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                           '(' <xexpr> ')'
%
ap(XExpr) --> ['('], xp(XExpr), [')'], !.

% ------------------------------------------------------------------------------
%                             EXPRESSIONS
%
% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                 'if' <xexpr> 'then' <xexpr> 'else' <xexpr>
%
ep([eif, Cond, Then, Else]) -->
  [if], xp(Cond), [then], xp(Then), [else], xp(Else), !.

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                     <atom> '(' <xexpr> [',' <xexpr>]...* ')'
%
ep(Call) -->
  ap(Atom), pcall(FirstAppl), !, pcalllis(MoreAppls),
  { makeappl([Atom|FirstAppl], MoreAppls, Call) }.

makeappl(Appl, [], Appl).
makeappl(Acc, [Args|T], Appl) :-
  makeappl([Acc|Args], T, Appl).
  

pcall(Args) --> ['('], pcomalis(xp, ')', Args).

pcalllis([H|T]) --> pcall(H), !, pcalllis(T).
pcalllis([]) --> !.

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                               <atom>
%
ep(X) --> ap(X), !.


% ------------------------------------------------------------------------------
%                            EXT-EXPRESSIONS
%
% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                      <expr> <operator> <xexpr>
%
xp([Op, Lhs, Rhs]) -->
  ep(Lhs), [Op], {op(_, _, Op)}, xp(Rhs), !.

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                               <expr>
%
xp(X) --> ep(X), !.


xplis([], [], _) :- !, throw(parse_error(unterminated_list)).
xplis([], [')'|Z], Z) :- !.
xplis([H|T], A, Z) :-
  xp(H, A, B),
  (
    B = [','|C] -> xplis(T, C, Z);
    B = [')'|Z] -> T = []
  ).


% ------------------------------------------------------------------------------
%                              STATEMENTS
%
% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                       'set' <ident> '=' <xexpr>
%
sp([['set!', Ident, XExpr]|Z], Z) -->
  [set], ip(Ident), ['='], xp(XExpr), !.

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                 'if' <xexpr> 'then' <xexpr> 'else' <xexpr>
%
sp([[sif, Cond, Then, Else]|Z], Z) -->
  [if], xp(Cond), [then], xp(Then), [else], xp(Else), !.

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                     'if' <xexpr> 'then' <xexpr>
%
sp([[sif, Cond, Then]|Z], Z) -->
  [if], xp(Cond), [then], xp(Then), !.

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                          <ident> '=' <xexpr>
%
sp([[define, Ident, XExpr]|Z], Z) -->
  ip(Ident), ['='], xp(XExpr), !.

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%            <ident> '(' [<parm> [ ',' <parm> ] ...*] ')' '->' <type> <body>
%
sp([[define, [Ident:Res|Args] |Body] |Z], Z) -->
  ip(Ident), ['('], pcomalis(typp, ')', Args), ['->'], tylp(Res), pbody(Body), !.

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%    'template' <ident> '(' [<ident> [ ',' <ident> ] ...*] ')' <body>
%
sp([[template, [Ident|Args] |Body] |Z], Z) -->
  [template], ip(Ident), ['('], pcomalis(pp, ')', Args), pbody(Body), !.


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                               <expr>
%
sp([X|Z], Z) --> xp(X), !.


splis(_, _, _, [], _) :- throw(parse_error(unterminated_list)).
splis(A, A, D) --> [D], !.
splis(A, Z, D) --> sp(A, B), splis(B, Z, D).


% ------------------------------------------------------------------------------
%                          TOPLEVEL-STATEMENTS
%
% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                  'extern' <ident> 'as' <ident> <type>
%
tp([[overload, Ident, [ExtIdent]] |Z], Z) -->
  [extern], ip(ExtIdent), [as], ip(Ident), typ(Type), !,
  { asserta(extern(ExtIdent, ExtIdent, Type)) }.

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                        'extern' <ident> <type>
%
tp([[overload, ShrdIdent, [ShrdIdent]] |Z], Z) -->
  [extern], ip(ShrdIdent), typ(Type), !,
  { asserta(extern(ShrdIdent, ShrdIdent, Type)) }.

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%               'infix' <ident> {left,right,nonassoc} <int>
%
tp(A, A) -->
  [infix], !, ip(Ident), [Fix], [Prec],
  {
    Fix == left     -> Assoc = xfy;
    Fix == right    -> Assoc = yfx;
    Fix == nonassoc -> Assoc = xfx
  },
  { integer(Prec) },
  { asserta(op(Prec, Assoc, Ident)) }.

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%                               <stmt>
%
tp(A, Z) --> sp(A, Z), !.

tplis(A, A, [], []) :- !.
tplis(A, Z) --> tp(A, B), tplis(B, Z).


% ------------------------------------------------------------------------------
%                         INVALID INPUT HANDLERS
%
ap(_, A, _) :- throw(parse_error(ap(A))).
ep(_, A, _) :- throw(parse_error(ep(A))).
xp(_, A, _) :- throw(parse_error(xp(A))).
sp(_, A, _) :- throw(parse_error(sp(A))).
tp(_, A, _) :- throw(parse_error(tp(A))).



% ------------------------------------------------------------------------------
%                               UTILS
%
pcomalis(_, _, [], [], _) :- throw(parse_error(unterminated_list)).
pcomalis(_, D, [], [D|Z], Z) :- !.
pcomalis(G, D, [H|T], A, Z) :-
  call(G, H, A, B),
  (
    B = [','|C] -> pcomalis(G, D, T, C, Z);
    B = [D|Z] -> T = []
  ).


% ------------------------------------------------------------------------------
%                                MAIN
%
parse(Input, Output) :-
  tokens(Input, Tokens),
  info("parsing                                ..."),
  (
    tplis(Output, [], Tokens, []) -> infonl("done");
    infonl("failure"), throw(parse_error)
  ).


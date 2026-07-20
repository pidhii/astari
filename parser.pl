
member(X, [X|_]).
member(X, [_|T]) :-
  member(X, T).


append([], L, L).
append([H|T], L, [H|TL]) :- append(T, L, TL).

rappend([H|T], L, [H|TL]) :- rappend(T, L, TL).
rappend([], L, L).


notempty([_|_]).


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                              Parser
%%
parses([], []).
parses(Goals, Input) :-
  parses_(Goals, Input).

parses_([GH|GT], Input) :-
  [GH|GT] = Input -> true;
  append(IL, IR, Input),
  notempty(IL),
  parses(GT, IR),
  parse(GH, IL).

parse(Goal, Input) :-
  tabulate(once(parse_(Goal, Input))).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                          Parser Grammars
%%
%%                            identity
%% X -> X
%%
parse_(X, [X]).

%%                           expr => ...
%% expr => expr op expr
parse_(expr(xfx(Lhs, Op, Rhs)), Input) :-
  append(L, [opsym(Op)|R], Input),
  parse(expr(Lhs), L),
  parse(expr(Rhs), R).

%% expr => sym
parse_(expr(A), [sym(A)]).

%% expr => val
parse_(expr(V), [val(V)]).

%% expr => var
parse_(expr(var(X)), [var(X)]).

%% expr => sym '(' expr ')'
parse_(expr(func(F, E)), [fun(F), '(' | Input]) :-
  append(I, [')'], Input),
  parse(expr(E), I).
%
%% expr => sym '(' ')'
parse_(expr(F), [fun(F), '(', ')']).

%% expr => '(' expr ')'
parse_(expr(inbrackets(E)), ['(' | Input]) :-
  append(I, [')'], Input),
  parse(expr(E), I).

%% expr => '[' ']'
parse_(expr([]), ['[', ']']).

%% expr => '[' expr ']'
parse_(expr(list(E)), ['[' | Input]) :-
  append(I, [']'], Input),
  parse(expr(E), I).

%% expr => '[' expr '|' expr ']'
parse_(expr(implist(E, Last)), ['[' | Input]) :-
  append(I, [']'], Input),
  parses([expr(E), '|', expr(Last)], I).

%% expr => op expr
parse_(expr(fx(Op, Arg)), [opsym(Op) | Input]) :-
  parse(expr(Arg), Input).


%%                           stmt => ...
%%
%% stmt => expr '.'
parse(stmt(S), In) :-
  append(Input, ['.'], In),
  parses([expr(S)], Input).



qtokens([], []).
qtokens([TH|TT], [QH|QT]) :-
  (
    var(TH)                                    -> QH = var(TH);
    op(TH, _, _)                               -> QH = opsym(TH);
    member(TH, ['(', ')', '[', '|', ']', '.']) -> QH = TH;
    atom(TH)                                   -> (
      TT = ['('|_] -> QH = fun(TH);
      QH = sym(TH)
    );
    atomic(TH)                                 -> QH = val(TH);
    TH = q(S)                                  -> (
      var(S) -> QH = var(S);
      TT = ['('|_] -> QH = fun(S);
      QH = sym(S)
    )
  ),
  qtokens(TT, QT).


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                          Operator Balancing
%%
right_assoc(xfx(_, OL, _), xfx(_, OR, _)) :- right_assoc_(OL, OR).
right_assoc(fx(OL, _), xfx(_, OR, _)) :- right_assoc_(OL, OR).
right_assoc(xfx(_, OL, _), fx(OR, _)).
left_assoc(xfx(_, OL, _), xfx(_, OR, _)) :- left_assoc_(OL, OR).
left_assoc(fx(OL, _), xfx(_, OR, _)) :- left_assoc_(OL, OR).
left_assoc(xfx(_, OL, _), fx(_, OR, _)) :- throw(stupid_questions).

right_assoc(X, Y) :- operator(X), operator(Y) -> fail; true.
left_assoc(X, Y) :- operator(X), operator(Y) -> fail; true.

operator(xfx(_, _, _)).
operator(fx(_, _)).

right_assoc_(Ol, Or) :-
  op(Ol, Pl, Al),
  op(Or, Pr, Ar),
  (
    Pl @< Pr -> true;
    Pl @> Pr -> fail;
    (Al \= Ar; Al = nonassoc) -> throw(syntax_error(operator_priority_clash(Ol, Or)));
    Al = right
  ).

left_assoc_(Ol, Or) :-
  op(Ol, Pl, Al),
  op(Or, Pr, Ar),
  (
    Pl @> Pr -> true;
    Pl @< Pr -> fail;
    (Al \= Ar; Al = nonassoc) -> throw(syntax_error(operator_priority_clash(Ol, Or)));
    Al = left
  ).

%% rotate_right(In, Out).
%%
%%   (A -> x) <- B  >>  A -> (x <- B)
%%
rotate_right(xfx(xfx(Al, Ao, X), Bo, Br), xfx(Al, Ao, xfx(X, Bo, Br))).
rotate_right(xfx(fx(Ao, X), Bo, Br), fx(Ao, xfx(X, Bo, Br))).

%% rotate_left(In, Out).
%%
%%   A -> (x <- B)  >>  (A -> x) <- B
%%
rotate_left(xfx(Al, Ao, xfx(X, Bo, Br)), xfx(xfx(Al, Ao, X), Bo, Br)).
rotate_left(fx(Ao, xfx(X, Bo, Br)), xfx(fx(Ao, X), Bo, Br)).

balance(I, O) :-
  breadthfirst,
  balance_(I, O).

balance_(X, X) :- atomic(X).
balance_(var(X), var(X)).
balance_(func(F, AIn), func(F, AOut)) :- balance_(AIn, AOut).
balance_(inbrackets(I), inbrackets(O)) :- balance_(I, O).
balance_(list(I), list(O)) :- balance_(I, O).
balance_(implist(IA, IB), implist(OA, OB)) :- balance_(IA, OA), balance_(IB, OB).
balance_(fx(Io, Ir), O) :-
  yield,
  balance_(Ir, Or),
  (
    right_assoc(fx(Io, Or), Or) -> O = fx(Io, Or);
    rotate_left(fx(Io, Or), Tmp), balance_(Tmp, O)
  ).

balance_(xfx(Il, Io, Ir), O) :-
  yield,
  balance_(Il, Ol),
  balance_(Ir, Or), 
  I = xfx(Ol, Io, Or),
  (
    \+ left_assoc(Ol, I) -> rotate_right(I, Tmp), balance_(Tmp, O);
    \+ right_assoc(I, Or) -> rotate_left(I, Tmp), balance_(Tmp, O);
    O = I
  ).


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                           Term Building
%%
flatten(X, X) :- atomic(X).

flatten(var(X), X).

flatten(func(F, A), Result) :-
  flatten(A, FlatA),
  (
    nonvar(FlatA), FlatA \= ',', FlatA =.. [',' | Args] -> Result =.. [F | Args];
    Result =.. [F, FlatA]
  ).

flatten(inbrackets(E), Result) :-
  flatten(E, FlatE),
  (
    nonvar(FlatE), FlatE \= ',', FlatE =.. [','|Args] -> Result =.. [','|Args];
    Result = FlatE
  ).

flatten(list(E), Result) :-
  flatten(E, FE),
  (
    nonvar(FE), FE \= ',', FE =.. [','|Result] -> true;
    Result = [FE]
  ).

flatten(implist(A, B), Result) :-
  flatten(A, FA),
  flatten(B, FB),
  (
    nonvar(FA), FA \= ',', FA =.. [','|FArgs] -> append(FArgs, FB, Result);
    Result = [FA|FB]
  ).

flatten(xfx(Lhs, Op, Rhs), Result) :-
  flatten(Lhs, L),
  flatten(Rhs, R),
  (
    Op = ',', nonvar(R), R \= ',', R =.. [','|T] -> Result =.. [',', L |T]    ;
    Op = ';', nonvar(L), L =.. [if, C, T, fail]  -> Result =   if(C, T, R)    ;
    Op = ';', nonvar(R), R \= ';', R =.. [';'|T] -> Result =.. [';', L |T]    ;
    Op = '->'                                    -> Result =   if(L, R, fail) ;
    Result =.. [Op, L, R]
  ).

flatten(fx(Op, Arg), Result) :-
  flatten(Arg, FArg),
  (
    nonvar(FArg), FArg \= ',', FArg =.. [',' | Args] -> Result =.. [Op | Args];
    Result =.. [Op, FArg]
  ).

  
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                           Operator Precedence
%%
op(':-', 0, nonassoc).
op(';', 10, right).
op('->', 50, nonassoc).
op(',', 100, right).

op('\+', 900, nonassoc).

op('=', 1000, nonassoc).
op('\=', 1000, nonassoc).

op('is', 1000, nonassoc).

op('=..', 1000, nonassoc).

op('==', 1500, nonassoc).
op('\==', 1500, nonassoc).
op('@>', 1500, nonassoc).
op('@<', 1500, nonassoc).
op('@>=', 1500, nonassoc).
op('@=<', 1500, nonassoc).

op('=:=', 1500, nonassoc).
op('=\=', 1500, nonassoc).
op('>', 1500, nonassoc).
op('<', 1500, nonassoc).
op('>=', 1500, nonassoc).
op('=<', 1500, nonassoc).

op('+', 2000, left).
op('-', 2000, left).

op('*', 2000, left).
op('/', 2000, left).
op('//', 2000, left).



parse_expr(TokensOrString, Term) :-
  (
    string(TokensOrString) -> tokens(TokensOrString, Tokens);
    Tokens = TokensOrString
  ),
  qtokens(Tokens, QTokens),
  once(parses([expr(Unbalanced)], QTokens)),
  balance(Unbalanced, Balanced),
  flatten(Balanced, Term).

%parse_stmt(Text, Term) :-
  %tokens(Text, Tokens),
  %qtokens(Tokens, QTokens),
  %once(parses([stmt(Unbalanced)], QTokens)),
  %balance(Unbalanced, Balanced),
  %flatten(Balanced, Term).

do_parse_one_stmt(Tokens, Term) :-
  qtokens(Tokens, QTokens),
  once(parses([stmt(Unbalanced)], QTokens)),
  balance(Unbalanced, Balanced),
  flatten(Balanced, Term).

parse_one_stmt(Tokens, Term, RemTokens) :-
  breadthfirst,
  once(append(L, ['.'|RemTokens], Tokens)),
  yield,
  append(L, ['.'], StmtToks),
  yield,
  do_parse_one_stmt(StmtToks, Term).


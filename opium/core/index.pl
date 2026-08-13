% Verbosity hooks (used by main)
:- dynamic(verbose/0).
:- dynamic(veryverbose/0).

% Type inference semantics
:- dynamic(cast/2). % subtyping
:- dynamic(decay/2). % function argument semantics
:- dynamic(materialize/2). % variable semantics
:- dynamic(conditional/1). % if-condition semantics
required_hook(cast/2).
required_hook(decay/2).
required_hook(materialize/2).
required_hook(conditional/1).

% Various API hooks
:- dynamic(extern/3).
:- dynamic(resultof/3).
:- dynamic(literal/2).
:- dynamic(type/1).

:- dynamic(specialization/2).
:- dynamic(instance/2).
:- dynamic(template/1).
:- dynamic(overload/2).

overloaded(RIdent) :-
  once(overload(RIdent, _)).


:- ensure_loaded("lists").
:- ensure_loaded("apply").

:- import_directory(".").
:- ensure_loaded("misc").
:- ensure_loaded("infest").
:- ensure_loaded("annotate").
:- ensure_loaded("rename").
:- ensure_loaded("scan").

:- op(100, xfx, ':').
:- op(100, xfx, '=>').


configured :-
  findall(
    _,
    ( required_hook(Name/Arity),
      length(Args, Arity),
      Clause =.. [Name|Args],
      must(clause(Clause, _), config_error(required_hook(Name/Arity)))
    ),
    _
  ).



%-------------------------------------------------------------------------------
%                                  MAIN
runall(Text, Parser, Emitter) :-
  configured,

  call(Parser, Text, Input),
  when(veryverbose, (
    write("Input:"), nl,
    printlist(2, Input),
    nl
  )),

  infest(Input, InfestedInput),
  when(veryverbose, (
    write("Infested Input:"), nl,
    printlist(2, InfestedInput),
    nl
  )),

  rename(InfestedInput, RenamedInput),
  when(veryverbose, (
    write("Renamed Input:"), nl,
    printlist(2, RenamedInput),
    nl
  )),

  annotate(RenamedInput, AnnotatedInput),
  when(veryverbose, (
    nl,
    write("Instantiations:"), nl,
    findall(K : S,  instance(K, S), Instantiations),
    printlist(2, Instantiations),
    nl,
    write("Annotations:"), nl,
    printlist(2, AnnotatedInput),
    nl
  )),

  scan(AnnotatedInput),
  when(veryverbose, (
    nl,
    write("Specializations:"), nl,
    findall(K : S,  specialization(K, S), Specializations),
    printlist(2, Specializations),
    nl
  )),

  call(Emitter, AnnotatedInput).


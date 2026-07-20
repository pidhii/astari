
put_char(S, Char) :-
  char_code(Char, Code),
  put_code(S, Code).

put_char(Code) :-
  current_output(S),
  put_char(S, Char). 

nl(S) :-
  put_code(S, 10).

nl :-
  current_output(S),
  nl(S).
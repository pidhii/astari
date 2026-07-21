
Builtins (ref https://www.deransart.fr/prolog/bips.html): 
<pre>
o <b>Control Constructs</b>
  - [x] true/0
  - [x] fail/0
  - [x] call/1
  - [ ] <s>!/0, cut</s>                    <i># use once/1 or if-then[-else]</i>
  - [x] `,`/2
  - [x] `;`/2
  - [x] if-then `->`/2              <i># strong cut</i>
  - [x] if-then-else (`->` + `;`)   <i># strong cut</i>
  - [x] catch/3 and throw/1
o <b>Term Unification</b>
  - [x] `=`/2
  - [ ] unify_with_occurs_check/2
  - [x] `\=`/2
o <b>Type Testing</b>
  - [x] var/1 
  - [x] atom/1
  - [x] integer/1 
  - [x] float/1 
  - [x] atomic/1
  - [x] compound/1 
  - [x] nonvar/1 
  - [x] number/1 
  - [x] string/1
o <b>Term Comparison</b>                      <i># <span style="color:yellow">order of terms besides numbers and strings is undetermined</span> (can differ between interpreter instances)</i>
  - [x] `@=<`/2
  - [x] `==`/2
  - [x] `\==`/2
  - [x] `@<`/2
  - [x] `@>`/2
  - [x] `@>=`/2
o <b>Term Creation and Decomposition</b>
  - [x] functor/3
  - [ ] arg/3
  - [x] `=..`/2
  - [ ] copy_term/2 
o <b>Arithmetic Evaluation</b>
  - [x] is/2
  - Evaluable Functors 
    - [x] `+`/2
    - [x] `-`/2
    - [x] `*`/2
    - [x] `/`/2
    - [x] `//`2
    - [ ] rem/2
    - [ ] mod/2
    - [x] `-`/1
    - [ ] abs/1
    - [ ] sign/1
    - [ ] float_integer_part/1
    - [ ] float_fractional_part/1
    - [ ] float/1
    - [ ] floor/1
    - [ ] truncate/1
    - [ ] round/1
    - [ ] ceiling/1
    - [ ] `**`/2
    - [ ] sin/1
    - [ ] cos/1
    - [ ] atan/1
    - [ ] exp/1
    - [ ] log/1
    - [ ] sqrt/1
    - [ ] `>>`/2
    - [ ] `<<`/2
    - [ ] `/\`/2
    - [ ] `\/`/2
    - [ ] `\`/1
o <b>Arithmetic Comparison</b>
  - [x] `=:=`/2
  - [x] `=\=`/2
  - [x] `<`/2
  - [x] `=<`/2
  - [x] `>`/2
  - [x] `>=`(@evaluable, @evaluable)
o <b>Clause Retrieval and Information</b>
  - [ ] clause/2 
  - [ ] current_predicate/1 
o <s><b>Clause Creation and Destruction</b></s>        <i># dirty + hits performance too hard</i>
  - [ ] <s>asserta/1</s>
  - [ ] <s>assertz/1</s> 
  - [ ] <s>retract/1</s>
  - [ ] <s>abolish/1</s>
o <b>All Solutions</b>
  - [ ] findall/3
  - [ ] bagof/3 
  - [ ] setof/3 
o <b>Input and Output</b>
  - [ ] current_input/1 
  - [x] current_output/1 
  - [ ] set_input/1 
  - [ ] set_output/1 
  - Opening a stream
    - [ ] open/3,4
  - Closing a stream
    - [ ] close/1,2
  - [ ] stream_property/2 
  - [ ] at_end_of_stream/0,1 
  - [ ] set_stream_position/2
o <b>Character Input Output</b>
  - [ ] get_char/1,2
  - [ ] get_code/1,2
  - [ ] peek_char/1,2
  - [ ] peek_code/1,2
  - [x] put_char/1,2
  - [x] put_code/1,2
  - [x] nl/0,1
  - [ ] flush_output/0,1
o <b>Reading from Binary streams</b>
  - [ ] get_byte/1,2
  - [ ] peek_byte/1,2
  - [ ] put_byte/1,2
o <b>Term Input and Output</b>
  - [ ] read_term/2,3
  - [ ] read/1,2
o <b>Writing terms</b>
  - [x] write_term/2,3
  - [x] write/1,2
  - [x] writeq/1,2
  - [x] write_canonical/1,2
o <b>Misc</b>
  - [ ] op/3
  - [ ] current_opt/3
  - [ ] char_conversion/2
  - [ ] current_char_conversion/2
o <b>Logic and Control</b>
  - [x] `\+`/1
  - [x] once/1 
  - [x] repeat/0 
o <b>Atom Processing</b>
  - [ ] atom_concat/3 
  - [ ] atom_length/2 
  - [ ] atom_chars/2
  - [ ] atom_codes/2 
  - [ ] sub_atom/5
  - [ ] char_code/2 
  - [ ] number_chars/2
  - [ ] number_codes/2 
o <b>Implementation defined hooks</b>
  - [x] halt/0,1
  - [ ] current_prolog_flag/2
  - [ ] set_prolog_flag/2
</pre>
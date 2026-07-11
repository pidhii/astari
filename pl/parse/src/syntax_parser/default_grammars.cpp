#include "syntax_parser.hpp"

#include "pl/coding/basic_decoder.hpp"
#include "pl/coding/basic_encoder.hpp"


void
load_default_grammar(dictionary &symdict, syntax_parser &stxparser)
{
  basic_encoder ec;
  basic_decoder dc;

  #define SYM(sym) symdict[#sym]
  #define TERM(sym, arity) ec.encode(term_header(SYM(sym), arity))

  // ','
  stxparser.add_grammar<binary_operator>(
      andseq, ',', obj, left,
      [](const object &lhs, const object &rhs) -> token {
        return token {andseq, lhs + rhs};
      }
  );
  stxparser.add_grammar<binary_operator>(
      obj, ',', obj, left,
      [](const object &lhs, const object &rhs) -> token {
        return token {andseq, lhs + rhs};
      }
  );
  stxparser.add_grammar<simple_grammar>(
      andseq, left,
      [&](const object_view &x) -> token {
        const size_t id = symdict[""];
        size_t arity = 0;
        for (auto it = x.begin(); it != x.end(); arity++)
          dc.decode_object(it);
        object result;
        result.push_back(ec.encode(term_header(id, arity)));
        result.append(x);
        return token {obj, result};
      }
  );

  // '->'
  stxparser.add_grammar<binary_operator>(
      obj, rarrow, obj, right,
      [&](const object &a, const object &b) -> token {
        return token {ifthen, a + b};
      }
  );

  // ';'
  stxparser.add_grammar<binary_operator>(
      obj, ';', orseq, right,
      [&](const object &lhs, const object &rhs) -> token {
        return token {orseq, lhs + rhs};
      }
  );
  stxparser.add_grammar<binary_operator>(
      ifthen, ';', orseq, right,
      [&](const object &ifthen, const object_view &orseq) -> token {
        object result;
        result += TERM(if, 3);
        result += ifthen;
        result += TERM(or, dc.count_objects(orseq.begin(), orseq.end()));
        result += orseq;
        return token {obj, result};
      }
  );
  stxparser.add_grammar<binary_operator>(
      obj, ';', obj, right,
      [&](const object &lhs, const object &rhs) -> token {
        return token {orseq, lhs + rhs};
      }
  );
  stxparser.add_grammar<binary_operator>(
      ifthen, ';', obj, right,
      [&](object_view ifthen, object_view els) -> token {
        object result;
        result += TERM(if, 3);
        result += ifthen;
        result += els;
        return token {obj, result};
      }
  );
  stxparser.add_grammar<simple_grammar>(
      orseq, left,
      [&](object_view x) -> token {
        object result;
        result += TERM(or, dc.count_objects(x.begin(), x.end()));
        result += x;
        return token {obj, result};
      }
  );
  stxparser.add_grammar<simple_grammar>(
      ifthen, left,
      [&](object_view ifthen) -> token {
        object result;
        result += TERM(if, 3);
        result += ifthen;
        result += TERM(fail, 0);
        return token {obj, result};
      }
  );

  // :-
  stxparser.add_grammar<subternary_operator>(
      obj, colon_minus, obj, '.', left,
      [&](object_view sign, object_view body) -> token {
        object result;
        result += sign;
        result += body;
        return token {predicate, result};
      }
  );
  stxparser.add_grammar<unary_sufix_operator>(
      obj, '.', left,
      [&](const object &sign) -> token {
        return token {statement, sign};
      }
  );
}
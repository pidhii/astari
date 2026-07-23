#include "object_file.hpp"

#include "pl/core/runtime.hpp"
#include "pl/misc/base64.h"
#include "pl/misc/display.hpp"



void
object_file::write_v0(std::ostream &out)
{
  // Locate and record all strings contained in the objects
  std::unordered_map<word_t, std::string_view> strings;
  for (const object &obj : objects)
  {
    for (word_t w : obj)
    {
      if (is_blob(w))
      {
        assert(blob_tag(w) == blob_tag::string);
        strings[w] = string(w);
      }
    }
  }

  // 1. Write symbols table
  out << symbols.m_names.size() << '\n';
  for (const auto &[name, id] : symbols.m_ids)
    out << name << ' ' << id << '\n';

  // 2. Write strings table
  out << strings.size() << '\n';
  for (const auto [w, s] : strings)
    out << w << ' ' << s << '\n';

  // 3. Write objects
  out << objects.size() << '\n';
  for (object &obj : objects)
  {
    normalize(obj, obj.data());
    const std::string_view objdata {reinterpret_cast<const char *>(obj.data()),
                                    obj.size() * sizeof(word_t)};
    out << base64_encode(objdata) << '\n';
    dump_object(symbols, obj, out, true, true, false); out << '\n';
  }
}

void
object_file::read_v0(std::istream &in, object_allocator &alloc)
{
  size_t nsym, nstr, nobj;
  std::unordered_map<word_t, word_t> strings;

  // 1. Read symbols table
  in >> nsym;
  size_t maxid = 0;
  while (nsym--)
  {
    std::string name;
    size_t id;
    in >> name >> id;
    auto it = symbols.m_ids.insert_or_assign(name, id);
    symbols.m_names[id] = it.first->first;
    maxid = std::max(maxid, id);
  }
  symbols.m_cnt = maxid + 1;

  // 2. Read strings table
  in >> nstr;
  while (nstr--)
  {
    word_t addr;
    std::string str;
    in >> addr >> str;
    const word_t s = alloc.make_string(str);
    strings[addr] = s;
  }

  // 3. Read objects
  in >> nobj;
  assert(in.peek() == '\n'); in.get();
  while (nobj--)
  {
    std::string b64obj, dump;
    std::getline(in, b64obj);
    std::getline(in, dump);
    const std::string objdata = base64_decode(b64obj);
    object obj {reinterpret_cast<const word_t *>(objdata.data()),
                objdata.size() / sizeof(word_t)};
    objects.emplace_back(std::move(obj));
  }

  // Inject allocated strings
  for (object &obj : objects)
  {
    for (word_t &w : obj)
    {
      if (is_blob(w))
        w = strings[w];
    }
  }
}
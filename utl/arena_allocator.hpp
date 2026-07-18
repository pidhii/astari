#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>



// #define MEMDEBUG

template <size_t BlockSize = 512 << 10, size_t BlockAlign = 8>
class arena_allocator {
  public:
  arena_allocator()
#ifdef MEMDEBUG
  { }
#else
  { std::tie(m_curblock_free, m_curblock_end) = _allocate_block(); }
#endif

  static constexpr size_t
  max_size() noexcept
  { return BlockSize; }

  void *
  allocate(size_t nbytes)
#ifdef MEMDEBUG
  {
    uint8_t *p = (uint8_t *)new uint64_t[nbytes];
    m_blocks.emplace_back(p);
    return p;
  }
#else
  {
    assert(nbytes <= BlockSize);
    void *const p = m_curblock_free;
    uint8_t *const newfree = m_curblock_free + nbytes;
    if (newfree <= m_curblock_end)
    {
      m_curblock_free = newfree;
      return p;
    }
    else
    {
      std::tie(m_curblock_free, m_curblock_end) = _allocate_block();
      return allocate(nbytes);
    }
  }
#endif

  void
  unallocate(size_t nbytes)
#ifdef MEMDEBUG
  { }
#else
  { m_curblock_free -= nbytes; }
#endif

  private:
#ifndef MEMDEBUG
  std::pair<uint8_t*, uint8_t*>
  _allocate_block()
  {
    using unit_type = std::aligned_storage<BlockAlign, BlockAlign>;
    unit_type *const block = new unit_type[BlockSize / sizeof(unit_type)];
    const std::unique_ptr<uint8_t[]> &newblock =
        m_blocks.emplace_back(reinterpret_cast<uint8_t *>(block));
    return {newblock.get(), newblock.get() + BlockSize};
  }

  private:
  uint8_t *m_curblock_free;
  uint8_t *m_curblock_end;
#endif
  std::vector<std::unique_ptr<uint8_t[]>> m_blocks;
};

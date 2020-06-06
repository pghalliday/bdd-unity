#include "Describe.hpp"
#include "AsyncIt.hpp"
#include "It.hpp"
#include "Pop.hpp"

namespace BddUnity {
  namespace Entry {

    Describe * Describe::create(Context::Interface & context, const char * thing, const int line, const f_describe describe, unsigned long timeout) {
      MemPool::Interface & memPool = context.getDescribeMemPool();
      void * address = memPool.alloc();
      if (address) {
        return new(address) Describe(context, thing, line, describe, timeout);
      }
      setStaticError(memPool.error, thing, line);
      return nullptr;
    }

    const Error * Describe::free() {
      const Error * ret = nullptr;
      const Error * e = nullptr;
      while (_entries) {
        e = _entries->free();
        if (e) ret = e;
        _entries = _entries->next;
      }
      e = _context.getDescribeMemPool().free(this);
      if (e) ret = e;
      return ret;
    }

    void Describe::describe(const char * thing, const f_describe describe, const long timeout) {
      Describe::describe(thing, 0, describe, timeout);
    }

    void Describe::describe(const char * thing, const int line, const f_describe describe, const long timeout) {
      Describe * entry = Describe::create(_context, thing, line, describe, timeout);
      if (entry) {
        _append(entry);
      } else {
        setError(Describe::staticError);
      }
    }

    void Describe::async(const char * thing, const AsyncIt::f_async it, const long timeout) {
      Describe::async(thing, 0, it, timeout);
    }

    void Describe::async(const char * thing, const int line, const AsyncIt::f_async it, const long timeout) {
      AsyncIt * entry = AsyncIt::create(_context, thing, line, it, timeout);
      if (entry) {
        _append(entry);
      } else {
        setError(AsyncIt::staticError);
      }
    }

    void Describe::it(const char * should, const It::f_testCallback it) {
      Describe::it(should, 0, it);
    }

    void Describe::it(const char * should, const int line, const It::f_testCallback it) {
      It * entry = It::create(_context, should, line, it);
      if (entry) {
        _append(entry);
      } else {
        setError(It::staticError);
      }
    }

    void Describe::run(Timeout & timeout, const f_done & done) {
      timeout.timeout = Timeout::NO_TIMEOUT;
      const Error * e = _context.getDepth().push(_thing, _timeout);
      if (e) {
        // failed to increase the depth
        setError(*e, _thing, _line);
        done(&error);
        return;
      }

      // collect the new entries
      _describe(*this);

      // check for error (eg: from MemPool allocation)
      if (hasError) {
        done(&error);
        return;
      }

      // append a pop to the entries
      Pop * pop = Pop::create(_context);
      if (!pop) {
        // I don't think this can happen
        // as the depth would be exceeded first
        setError(Pop::staticError, _thing, _line);
        done(&error);
        return;
      }
      _append(pop);

      // now insert the entries as next
      // and clear the local reference
      _entriesTail->next = next;
      next = _entries;
      _entries = nullptr;

      // and we're done
      done(nullptr);
      return;
    }

    Describe::Describe(
        Context::Interface & context,
        const char * thing,
        const int line,
        const f_describe describe,
        unsigned long timeout) :
      Interface(context),
      _thing(thing),
      _line(line),
      _describe(describe),
      _timeout(timeout) {}

    void Describe::_append(Interface * entry) {
      if (_entries) {
        _entriesTail->next = entry;
      } else {
        _entries = entry;
      }
      _entriesTail = entry;
    }

  }
}

/******************************************************************************
    Copyright © 2012-2015 Martin Karsten

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#ifndef _IntrusiveContainer_h_
#define _IntrusiveContainer_h_ 1

#include "generic/basics.h"
#include <assert.h>

#define GENASSERT0(expr)            { assert(expr); }
#define GENASSERT1(expr, msg)       { assert(expr); }
#define GENASSERTN(expr, args...)   { assert(expr); }
#define GENABORT0   exit
#define GENABORT1   exit



template<typename T,size_t ID=0> class IntrusiveStack {
public:
  class Link {
    friend class IntrusiveStack<T,ID>;
    T* next;
  public:
    constexpr Link() : next(nullptr) {}
    bool onStack() const { return next != nullptr; }
  } __packed;

private:
  T* head;

public:
  IntrusiveStack() : head(nullptr) {}
  bool empty() const { return head == nullptr; }

  T*              front()       { return head; }
  const T*        front() const { return head; }

  static T*       next(      T& elem) { return elem.Link::next; }
  static const T* next(const T& elem) { return elem.Link::next; }

  void push(T& first, T& last) {
    GENASSERT1(!last.onStack(), FmtHex(&first));
    last.Link::next = head;
    head = first;
  }

  void push(T& elem) {
    push(elem, elem);
  }

  T* pop() {
    GENASSERT1(!empty(), FmtHex(this));
    T* last = head;
    head = next(*last);
    last->Link::next = nullptr;
    return last;
  }

  T* pop(size_t& count) {
    GENASSERT1(!empty(), FmtHex(this));
    T* last = head;
    for (size_t i = 1; i < count; i += 1) {
      if slowpath(next(*last) == nullptr) count = i; // breaks loop and sets count
      else last = next(*last);
    }
    head = next(*last);
    last->Link::next = nullptr;
    return last;
  }

  void transferFrom(IntrusiveStack& es, size_t& count) {
    GENASSERT1(!es.empty(), FmtHex(&es));
    T* first = es.front();
    T* last = es.pop(count);
    push(*first, *last);
  }
} __packed;


template<typename T, size_t ID=0> class IntrusiveQueue {
public:
  class Link {
    friend class IntrusiveQueue<T,ID>;
    T* next;
  public:
    constexpr Link() : next(nullptr) {}
    bool onQueue() const { return next != nullptr; }
  } __packed;

private:
  T* head;
  T* tail;

public:
  IntrusiveQueue() : head(nullptr), tail(nullptr) {}
  bool empty() const {
    GENASSERT1((head == nullptr) == (tail == nullptr), FmtHex(this));
    return head == nullptr;
  }

  T*              front()       { return head; }
  const T*        front() const { return head; }
  T*              back()        { return tail; }
  const T*        back()  const { return tail; }

  static T*       next(      T& elem) { return elem.Link::next; }
  static const T* next(const T& elem) { return elem.Link::next; }

  void push(T& first, T& last) {
    GENASSERT1(!last.onQueue(), FmtHex(&first));
    if slowpath(!head) head = &first;
    else {
      GENASSERT1(tail != nullptr, FmtHex(this));
      tail->Link::next = &first;
    }
    tail = &last;
  }

  void push(T& elem) {
    push(elem, elem);
  }

  T* pop() {
    GENASSERT1(!empty(), FmtHex(this));
    T* last = head;
    head = next(*last);
    if slowpath(tail == last) tail = nullptr;
    last->Link::next = nullptr;
    return last;
  }

  T* pop(size_t& count) {
    GENASSERT1(!empty(), FmtHex(this));
    T* last = head;
    for (size_t i = 1; i < count; i += 1) {
      if slowpath(next(*last) == nullptr) count = i; // breaks loop and sets count
      else last = next(*last);
    }
    head = next(*last);
    if slowpath(tail == last) tail = nullptr;
    last->Link::next = nullptr;
    return last;
  }

  void transferFrom(IntrusiveQueue& eq, size_t& count) {
    GENASSERT1(!eq.empty(), FmtHex(&eq));
    T* first = eq.front();
    T* last = eq.pop(count);
    push(*first, *last);
  }
} __packed;

// NOTE WELL: this simple design (using anchor) only works, if Link is first in T
template<typename T, size_t ID=0> class IntrusiveList {
public:
  class Link {
    friend class IntrusiveList<T,ID>;
    Link* prev;
    Link* next;
  public:
    constexpr Link() : prev(nullptr), next(nullptr) {}
#if 0
    ~Link() {
      if fastpath(onList()) {
        prev->next = next;
        next->prev = prev;
      }
    }
#endif
    bool onList() const {
      GENASSERT1((prev == nullptr) == (next == nullptr), FmtHex(this));
      return next != nullptr;
    }
  } __packed;

private:
  Link anchor;

public:
  IntrusiveList() { anchor.next = anchor.prev = &anchor; }

  bool            empty() const { return &anchor == anchor.next; }
  Link*           fence()       { return &anchor; }
  const Link*     fence() const { return &anchor; }

  T*              front()       { return       (T*)anchor.next; }
  const T*        front() const { return (const T*)anchor.next; }
  T*              back()        { return       (T*)anchor.prev; }
  const T*        back()  const { return (const T*)anchor.prev; }

  static T*       next(      T& elem) { return       (T*)elem.Link::next; }
  static const T* next(const T& elem) { return (const T*)elem.Link::next; }
  static T*       prev(      T& elem) { return       (T*)elem.Link::prev; }
  static const T* prev(const T& elem) { return (const T*)elem.Link::prev; }

  static void insert_before(T& next, T& elem) {
    GENASSERT1(!elem.onList(), FmtHex(&elem));
    GENASSERT1(next.onList(), FmtHex(&prev));
    next.Link::prev->Link::next = &elem;
    elem.Link::prev = next.Link::prev;
    next.Link::prev = &elem;
    elem.Link::next = &next;
  }

  static void insert_after(T& prev, T& first, T&last) {
    GENASSERT1(first.Link::prev == nullptr, FmtHex(&first));
    GENASSERT1(last.Link::next == nullptr, FmtHex(&last));
    GENASSERT1(prev.onList(), FmtHex(&prev));
    prev.Link::next->Link::prev = &last;
    last.Link::next = prev.Link::next;
    prev.Link::next = &first;
    first.Link::prev = &prev;
  }

  static void insert_after(T& prev, T& elem) {
    insert_after(prev, elem, elem);
  }

  static T* remove(T& elem) {
    GENASSERT1(elem.onList(), FmtHex(&elem));
    elem.Link::prev->Link::next = elem.Link::next;
    elem.Link::next->Link::prev = elem.Link::prev;
    elem.Link::prev = nullptr;
    elem.Link::next = nullptr;
    return &elem;
  }

  T* remove(T& first, size_t& count) {
    GENASSERT1(first.onList(), FmtHex(&first));
    T* last = &first;
    for (size_t i = 1; i < count; i += 1) {
      if slowpath(next(*last) == fence()) count = i; // breaks loop and sets count
      else last = next(*last);
    }
    first.Link::prev->Link::next = last->Link::next;
    last->Link::next->Link::prev = first.Link::prev;
    first.Link::prev = nullptr;
    last->Link::next = nullptr;
    return last;
  }

  void push_front(T& elem)           { insert_before(*front(), elem); }
  void push_back(T& elem)            { insert_after (*back(),  elem); }
  void splice_back(T& first, T&last) { insert_after (*back(),  first, last); }

  T* pop_front() { GENASSERT1(!empty(), FmtHex(this)); return remove(*front()); }
  T* pop_back()  { GENASSERT1(!empty(), FmtHex(this)); return remove(*back()); }

  void transferFrom(IntrusiveList& el, size_t& count) {
    GENASSERT1(!el.empty(), FmtHex(&el));
    T* first = el.front();
    T* last = el.remove(*first, count);
    splice_back(*first, *last);
  }

  void transferAllFrom(IntrusiveList& el) {
    GENASSERT1(!el.empty(), FmtHex(&el));
    T* first = el.front();
    T* last = el.back();
    first->Link::prev = nullptr;
    last->Link::next = nullptr;
    el.anchor.next = el.anchor.prev = &el.anchor;
    splice_back(*first, *last);
  }
} __packed;

#endif /* _IntrusiveContainer_h_ */

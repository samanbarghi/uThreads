/*
 * EmbeddedList.h
 *
 *  Created on: Nov 10, 2014
 *      Author: Saman Barghi
 */

#ifndef EMBEDDEDLIST_H_
#define EMBEDDEDLIST_H_

#include "global.h"

// NOTE WELL: this design (anchor) only works, if Element is first in T
template<typename T, int ID=0> class EmbeddedList {
public:
  class Element {
    friend class EmbeddedList<T,ID>;
    Element* prev;
    Element* next;
  } __packed;

private:
  Element anchor;

public:
  EmbeddedList() { anchor.next = anchor.prev = &anchor; }

  bool            empty() const { return &anchor == anchor.next; }
  Element*        fence()       { return &anchor; }
  const Element*  fence() const { return &anchor; }

  T*              front()       { return       (T*)anchor.next; }
  const T*        front() const { return (const T*)anchor.next; }
  T*              back()        { return       (T*)anchor.prev; }
  const T*        back()  const { return (const T*)anchor.prev; }

  static T*       next(T* elem)       { return       (T*)elem->Element::next; }
  static const T* next(const T* elem) { return (const T*)elem->Element::next; }
  static T*       prev(T* elem)       { return       (T*)elem->Element::prev; }
  static const T* prev(const T* elem) { return (const T*)elem->Element::prev; }

  static void insert_before(T* elem, T* next) {
    next->Element::prev->Element::next = elem;
    elem->Element::prev = next->Element::prev;
    next->Element::prev = elem;
    elem->Element::next = next;
  }

  static void insert_after(T* elem, T* prev) {
    prev->Element::next->Element::prev = elem;
    elem->Element::next = prev->Element::next;
    prev->Element::next = elem;
    elem->Element::prev = prev;
  }

  static void remove(T* elem) {
    elem->Element::prev->Element::next = elem->Element::next;
    elem->Element::next->Element::prev = elem->Element::prev;
    elem->Element::next = nullptr;
    elem->Element::prev = nullptr;
  }

  void push_front(T* e) { insert_before(e, front()); }
  void push_back(T* e)  { insert_after (e, back()); }
  void pop_front() { remove(front()); }
  void pop_back()  { remove(back()); }
} __packed;

#endif /* EMBEDDEDLIST_H_ */

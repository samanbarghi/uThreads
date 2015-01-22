/*
 * Buffers.h
 *
 *  Created on: Jan 21, 2015
 *      Author: Saman Barghi
 */

#ifndef BUFFERS_H_
#define BUFFERS_H_
#include <array>
#include <cassert>
#include "global.h"

template<typename Element, size_t N>
class FixedArray : public std::array<Element,N> {
public:
  typedef Element ElementType;
  explicit FixedArray( size_t ) {}
};

template<typename Array>
class RingBuffer {
  size_t newest;
  size_t oldest;
  size_t count;
  Array array;
public:
  typedef typename Array::ElementType Element;
  explicit RingBuffer( size_t N = 0 )
    : newest(N-1), oldest(0), count(0), array(N) {}
  bool empty() const { return count == 0; }
  bool full() const { return count == array.max_size(); }
  size_t size() const { return count; }
  size_t max_size() const { return array.max_size(); }
  Element& front() {
    assert(count !=0);
    return array[oldest];
  }
  const Element& front() const {
    assert(count !=0);
    return array[oldest];
  }
  Element& back() {
    assert(count !=0);
    return array[newest];
  }
  const Element& back() const {
    assert(count !=0);
    return array[newest];
  }
  void push( const Element& x ) {
    assert(fastpath(count < array.max_size()));
    newest = (newest + 1) % array.max_size();
    array[newest] = x;
    count += 1;
  }
  void pop() {
    assert(count !=0);
    oldest = (oldest + 1) % array.max_size();
    count -= 1;
  }
};

template<typename Element, size_t N>
class FixedRingBuffer : public RingBuffer<FixedArray<Element,N>> {
public:
  explicit FixedRingBuffer(size_t) {}
};

#endif /* BUFFERS_H_ */

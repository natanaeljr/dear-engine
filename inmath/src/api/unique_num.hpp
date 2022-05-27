#pragma once

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UniqueNum

/// Utility type to make unique numbers (IDs) movable, when moved the value should be zero
template<typename T>
struct UniqueNum {
  T inner;
  UniqueNum() : inner(0) {}
  UniqueNum(T n) : inner(n) {}
  UniqueNum(UniqueNum&& o) : inner(o.inner) { o.inner = 0; }
  UniqueNum& operator=(UniqueNum&& o) { inner = o.inner; o.inner = 0; return *this; }
  UniqueNum(const UniqueNum&) = delete;
  UniqueNum& operator=(const UniqueNum&) = delete;
  operator T() const { return inner; }
};

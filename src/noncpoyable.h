#pragma once

// Disable the copy constructor of its derived class. The objects
// of classes inherit from noncopyable can not be copy.
class noncopyable {
 public:
  noncopyable(const noncopyable &) = delete;
  noncopyable *operator=(const noncopyable &) = delete;

 protected:
  noncopyable() = default;
  ~noncopyable() = default;
};
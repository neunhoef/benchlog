////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2021 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Kaveh Vahedipour
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGODB_REST_SERVER_METRICS_H
#define ARANGODB_REST_SERVER_METRICS_H 1

#include <atomic>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <string.h>
#include <vector>

#if defined ARANGODB_BITS
#include "Basics/debugging.h"

#include <velocypack/Builder.h>
#include <velocypack/Value.h>
#include <velocypack/velocypack-aliases.h>
#else
#include <cassert>
#define TRI_ASSERT(x) assert(x)
#endif

#include "counter.h"

class Metric {
 public:
  Metric(std::string const& name, std::string const& help, std::string const& labels);
  virtual ~Metric();
  std::string const& help() const;
  std::string const& name() const;
  std::string const& labels() const;
  virtual void toPrometheus(std::string& result) const = 0;
  void header(std::string& result) const;
 protected:
  std::string const _name;
  std::string const _help;
  std::string const _labels;
};

struct Metrics {
  using counter_type = gcl::counter::simplex<uint64_t, gcl::counter::atomicity::full>;
  using hist_type = gcl::counter::simplex_array<uint64_t, gcl::counter::atomicity::full>;
  using buffer_type = gcl::counter::buffer<uint64_t, gcl::counter::atomicity::full, gcl::counter::atomicity::full>;
};


/**
 * @brief Counter functionality
 */
class Counter : public Metric {
 public:
  Counter(uint64_t const& val, std::string const& name, std::string const& help,
          std::string const& labels = std::string());
  Counter(Counter const&) = delete;
  ~Counter();
  std::ostream& print (std::ostream&) const;
  Counter& operator++();
  Counter& operator++(int);
  Counter& operator+=(uint64_t const&);
  Counter& operator=(uint64_t const&);
  void count();
  void count(uint64_t);
  uint64_t load() const;
  void store(uint64_t const&);
  void push();
  virtual void toPrometheus(std::string&) const override;
 private:
  mutable Metrics::counter_type _c;
  mutable Metrics::buffer_type _b;
};


template<typename T> class Gauge : public Metric {
 public:
  Gauge() = delete;
  Gauge(T const& val, std::string const& name, std::string const& help,
        std::string const& labels = std::string())
    : Metric(name, help, labels), _g(val) { }

  Gauge(Gauge const&) = delete;
  ~Gauge() = default;

  std::ostream& print(std::ostream&) const;

  T fetch_add(T t, std::memory_order mo = std::memory_order_relaxed) noexcept {
    if constexpr(std::is_integral_v<T>) {
      return _g.fetch_add(t, mo);
    } else {
      T tmp(_g.load(std::memory_order_relaxed));
      do {
      } while (!_g.compare_exchange_weak(tmp, tmp + t, mo, std::memory_order_relaxed));
      return tmp;
    }
  }

  Gauge<T>& operator+=(T const& t) noexcept {
    fetch_add(t, std::memory_order_relaxed);
    return *this;
  }

  // prefix increment
  Gauge<T>& operator++() noexcept {
    fetch_add(1, std::memory_order_relaxed);
    return *this;
  }

  // postfix increment. as this would be inefficient, we simply forbid it
  Gauge<T>& operator++(int) = delete;

  T fetch_sub(T t, std::memory_order mo = std::memory_order_relaxed) noexcept {
    if constexpr(std::is_integral_v<T>) {
      return _g.fetch_sub(t, mo);
    } else {
      T tmp(_g.load(std::memory_order_relaxed));
      do {
      } while (!_g.compare_exchange_weak(tmp, tmp - t, mo, std::memory_order_relaxed));
      return tmp;
    }
  }

  Gauge<T>& operator-=(T const& t) noexcept {
    fetch_sub(t, std::memory_order_relaxed);
    return *this;
  }

  // prefix decrement
  Gauge<T>& operator--() noexcept {
    fetch_sub(1, std::memory_order_relaxed);
    return *this;
  }

  // postfix decrement. as this would be inefficient, we simply forbid it
  Gauge<T>& operator--(int) = delete;

  Gauge<T>& operator*=(T const& t) noexcept {
    T tmp(_g.load(std::memory_order_relaxed));
    do {
    } while (!_g.compare_exchange_weak(tmp, tmp * t));
    return *this;
  }

  Gauge<T>& operator/=(T const& t) noexcept {
    TRI_ASSERT(t != T(0));
    T tmp(_g.load(std::memory_order_relaxed));
    do {
    } while (!_g.compare_exchange_weak(tmp, tmp / t));
    return *this;
  }

  Gauge<T>& operator=(T const& t) noexcept {
    _g.store(t, std::memory_order_relaxed);
    return *this;
  }

  T load(std::memory_order mo = std::memory_order_relaxed) const noexcept { return _g.load(mo); }

  void toPrometheus(std::string& result) const override {
    result += "\n#TYPE " + name() + " gauge\n";
    result += "#HELP " + name() + " " + help() + "\n" + name();
    if (!labels().empty()) {
      result += "{" + labels() + "}";
    }
    result += " " + std::to_string(load()) + "\n";
  }
 private:
  std::atomic<T> _g;
};

std::ostream& operator<< (std::ostream&, Metrics::hist_type const&);

enum ScaleType { Fixed, Linear, Logarithmic, RoughLogarithmic };

template<typename T>
struct scale_t {
 public:

  using value_type = T;

  scale_t(T const& low, T const& high, size_t n) :
    _low(low), _high(high), _n(n) {
    TRI_ASSERT(n > 1);
    _delim.resize(n - 1);
  }
  virtual ~scale_t() = default;
  /**
   * @brief number of buckets
   */
  size_t n() const {
    return _n;
  }
  /**
   * @brief number of buckets
   */
  T low() const {
    return _low;
  }
  /**
   * @brief number of buckets
   */
  T high() const {
    return _high;
  }
  /**
   * @brief number of buckets
   */
  std::string const delim(size_t const& s) const {
    return (s < _n - 1) ? std::to_string(_delim.at(s)) : "+Inf";
  }
  /**
   * @brief number of buckets
   */
  std::vector<T> const& delims() const {
    return _delim;
  }

  /**
   * @brief dump to builder
   */
#if defined ARANGODB_BITS
  virtual void toVelocyPack(VPackBuilder& b) const {
    TRI_ASSERT(b.isOpenObject());
    b.add("lower-limit", VPackValue(_low));
    b.add("upper-limit", VPackValue(_high));
    b.add("value-type", VPackValue(typeid(T).name()));
    b.add(VPackValue("range"));
    VPackArrayBuilder abb(&b);
    for (auto const& i : _delim) {
      b.add(VPackValue(i));
    }
  }
#endif
  /**
   * @brief dump to
   */
  std::ostream& print(std::ostream& o) const {
#if defined ARANGODB_BITS
    VPackBuilder b;
    {
      VPackObjectBuilder bb(&b);
      this->toVelocyPack(b);
    }
    o << b.toJson();
#else
    o << "lowest value: " << _low << ", highest value: " << _high << ", type: " << typeid(T).name() << ", range: ";
#endif
    return o;
  }
 protected:
  T _low, _high;
  std::vector<T> _delim;
  size_t _n;
};

template<typename T>
std::ostream& operator<< (std::ostream& o, scale_t<T> const& s) {
  return s.print(o);
}

template <typename T>
struct fixed_scale_t : public scale_t<T> {
 public:
  using value_type = T;
  static constexpr ScaleType scale_type = Fixed;

  fixed_scale_t(T const& low, T const& high, std::initializer_list<T> const& list)
      : scale_t<T>(low, high, list.size() + 1) {
    this->_delim = list;
  }
  virtual ~fixed_scale_t() = default;
  /**
   * @brief index for val
   * @param val value
   * @return    index
   */
  size_t pos(T const& val) const {
    for (std::size_t i = 0; i < this->_delim.size(); ++i) {
      if (val <= this->_delim[i]) {
        return i;
      }
    }
    return this->_delim.size();
  }

#if defined ARANGODB_BITS
  virtual void toVelocyPack(VPackBuilder& b) const override {
    b.add("scale-type", VPackValue("fixed"));
    scale_t<T>::toVelocyPack(b);
  }
#endif

 private:
  T _base, _div;
};

template<typename T>
struct log_scale_t : public scale_t<T> {
 public:

  using value_type = T;
  static constexpr ScaleType scale_type = Logarithmic;

  log_scale_t(T const& base, T const& low, T const& high, size_t n) :
    scale_t<T>(low, high, n), _base(base) {
    TRI_ASSERT(base > T(0));
    _n = n;
    double nn = -1.0 * (n - 1);
    for (auto& i : this->_delim) {
      i = static_cast<T>(
        static_cast<double>(high - low) *
        std::pow(static_cast<double>(base), static_cast<double>(nn++)) + static_cast<double>(low));
    }
    _div = log(this->_delim.front() - low);
    TRI_ASSERT(_div > T(0));
    _lbase = log(static_cast<double>(_base));
  }
  virtual ~log_scale_t() = default;
  /**
   * @brief index for val
   * @param val value
   * @return    index
   */
  size_t pos(T const& val) const {
    if (val < this->_delim.front()) {
      return 0;
    } else if (val >= this->high()) {
      return _n;
    } else {
      return static_cast<size_t>(1+std::floor((log(val - this->_low)-this->_div)/this->_lbase));
    }
  }
#if defined ARANGODB_BITS
  /**
   * @brief Dump to builder
   * @param b Envelope
   */
  virtual void toVelocyPack(VPackBuilder& b) const override {
    b.add("scale-type", VPackValue("logarithmic"));
    b.add("base", VPackValue(_base));
    scale_t<T>::toVelocyPack(b);
  }
#endif
  /**
   * @brief Base
   * @return base
   */
  T base() const {
    return _base;
  }

 private:
  double _base, _lbase, _div, _n;
};

template<typename T>
inline int32_t log2rough(T x) {
  return log2rough((double) x);
}

template<>
inline int32_t log2rough<double>(double x) {
  uint64_t y;
  memcpy(&y, &x, 8);
  return ((int32_t)(y >> 52) & 0x7ff) - 1023;
}

template<>
inline int32_t log2rough<float>(float x) {
  uint32_t y;
  memcpy(&y, &x, 4);
  return (int32_t) ((y >> 23) & 0xff) - 127;
}

template<typename T>
struct logr_scale_t : public scale_t<T> {
 public:

  using value_type = T;
  static constexpr ScaleType scale_type = RoughLogarithmic;

  logr_scale_t(T const& base, T const& low, T const& high, size_t n) :
    scale_t<T>(low, high, n), _base(base) {
    // We want to have n buckets so we are looking for n-1 delimiters
    // between low and high. The first bucket will include everything below
    // low and everything up to the first delimiter. The last bucket will
    // include everything higher than the last delimiter and everything
    // above high.
    // First we compute _mul with this equation:
    //   low + _base ^ n / _mul = high
    // Thus: _mul = _base ^ n / (high - low)
    // Then the boundaries are:
    //   low + _base ^ i / _mul   for  i = 1, 2, ..., n-1
    // In the end, we compute the bucket with this formula:
    //   log2rough((val - low) * _mul) / _div
    // where _div is 1 for base 2 and 3 for base 8.
    // We only have to be careful for the boundaries.
    _mul = static_cast<T>(
              std::pow((double) base, (double) n) / (double) (high - low));
    for (size_t i = 0; i < n-1; ++i) {
      this->_delim[i] = low + std::pow((double) base, (double) i+1) / _mul;
    }
    _div = (base == 2) ? 1 : 3;
    _inFirst = (this->_low + this->_delim[0]) / 2;
  }
  virtual ~logr_scale_t() = default;
  /**
   * @brief index for val
   * @param val value
   * @return    index
   */

  size_t pos(T const& val) const {
    T tmp = (val < this->_inFirst) ? this->_inFirst : val;
    int32_t l = log2rough((tmp - this->_low) * _mul) / _div;
    size_t p = static_cast<size_t>(l);
    return (p < this->_n) ? p : this->_n - 1;
  }

#if defined ARANGODB_BITS
  /**
   * @brief Dump to builder
   * @param b Envelope
   */
  virtual void toVelocyPack(VPackBuilder& b) const override {
    b.add("scale-type", VPackValue("rough-logarithmic"));
    b.add("base", VPackValue(_base));
    scale_t<T>::toVelocyPack(b);
  }
#endif
  /**
   * @brief Base
   * @return base
   */
  T base() const {
    return _base;
  }

 private:
  T _base;
  T _mul;
  T _inFirst;
  int32_t _div;
};

template<typename T>
struct lin_scale_t : public scale_t<T> {
 public:

  using value_type = T;
  static constexpr ScaleType scale_type = Linear;

  lin_scale_t(T const& low, T const& high, size_t n) :
    scale_t<T>(low, high, n) {
    this->_delim.resize(n-1);
    _div = (high - low) / (T)n;
    if (_div <= 0) {
    }
    TRI_ASSERT(_div > 0);
    T le = low;
    for (auto& i : this->_delim) {
      le += _div;
      i = le;
    }
  }
  virtual ~lin_scale_t() = default;
  /**
   * @brief index for val
   * @param val value
   * @return    index
   */
  size_t pos(T const& val) const {
    return static_cast<size_t>(std::floor((val - this->_low)/ _div));
  }

#if defined ARANGODB_BITS
  virtual void toVelocyPack(VPackBuilder& b) const override {
    b.add("scale-type", VPackValue("linear"));
    scale_t<T>::toVelocyPack(b);
  }
#endif

 private:
  T _base, _div;
};


/**
 * @brief Histogram functionality
 */
template<typename Scale> class Histogram : public Metric {

 public:

  using value_type = typename Scale::value_type;

  Histogram() = delete;

  Histogram(Scale&& scale, std::string const& name, std::string const& help,
            std::string const& labels = std::string())
    : Metric(name, help, labels), _c(Metrics::hist_type(scale.n())), _scale(std::move(scale)),
      _lowr(std::numeric_limits<value_type>::max()),
      _highr(std::numeric_limits<value_type>::min()),
      _n(_scale.n() - 1) {}

  Histogram(Scale const& scale, std::string const& name, std::string const& help,
            std::string const& labels = std::string())
    : Metric(name, help, labels), _c(Metrics::hist_type(scale.n())), _scale(scale),
      _lowr(std::numeric_limits<value_type>::max()),
      _highr(std::numeric_limits<value_type>::min()),
      _n(_scale.n() - 1) {}

  ~Histogram() = default;

  void records(value_type const& val) {
    if (val < _lowr) {
      _lowr = val;
    } else if (val > _highr) {
      _highr = val;
    }
  }

  Scale const& scale() {
    return _scale;
  }

  size_t pos(value_type const& t) const {
    return _scale.pos(t);
  }

  void count(value_type const& t) {
    count(t, 1);
  }

  void count(value_type const& t, uint64_t n) {
    _c[_scale.pos(t)]+=n;
#ifdef USE_MAINTAINER_MODE
    records(t);
#endif
  }
  value_type const& low() const { return _scale.low(); }
  value_type const& high() const { return _scale.high(); }

  Metrics::hist_type::value_type& operator[](size_t n) {
    return _c[n];
  }

  std::vector<uint64_t> load() const {
    std::vector<uint64_t> v(size());
    for (size_t i = 0; i < size(); ++i) {
      v[i] = load(i);
    }
    return v;
  }

  uint64_t load(size_t i) const { return _c.load(i); };

  size_t size() const { return _c.size(); }

  virtual void toPrometheus(std::string& result) const override {
    result += "\n#TYPE " + name() + " histogram\n";
    result += "#HELP " + name() + " " + help() + "\n";
    std::string lbs = labels();
    auto const haveLabels = !lbs.empty();
    auto const separator = haveLabels && lbs.back() != ',';
    uint64_t sum(0);
    for (size_t i = 0; i < size(); ++i) {
      uint64_t n = load(i);
      sum += n;
      result += name() + "_bucket{";
      if (haveLabels) {
        result += lbs;
      }
      if (separator) {
        result += ",";
      }
      result += "le=\"" + _scale.delim(i) + "\"} " + std::to_string(n) + "\n";
    }
    result += name() + "_count";
    if (!labels().empty()) {
      result += "{" + labels() + "}";
    }
    result += " " + std::to_string(sum) + "\n";
  }

  std::ostream& print(std::ostream& o) const {
    o << name() << " scale: " <<  _scale << " extremes: [" << _lowr << ", " << _highr << "]";
    return o;
  }

 private:
  Metrics::hist_type _c;
  Scale _scale;
  value_type _lowr, _highr;
  size_t _n;

};

std::ostream& operator<< (std::ostream&, Metrics::counter_type const&);
template<typename T>
std::ostream& operator<<(std::ostream& o, Histogram<T> const& h) {
  return h.print(o);
}

#endif

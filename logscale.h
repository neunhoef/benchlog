#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>
#include <iostream>

enum ScaleType { Fixed, Linear, Logarithmic };

template<typename T>
struct scale_t {
 public:

  using value_type = T;

  scale_t(T const& low, T const& high, size_t n) :
    _low(low), _high(high), _n(n) {
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
    double nn = -1.0 * (n - 1);
    for (auto& i : this->_delim) {
      i = static_cast<T>(
        static_cast<double>(high - low) *
        std::pow(static_cast<double>(base), static_cast<double>(nn++)) + static_cast<double>(low));
    }
    _div = this->_delim.front() - low;
    _lbase = log(_base);
  }
  virtual ~log_scale_t() = default;
  /**
   * @brief index for val
   * @param val value
   * @return    index
   */
  size_t pos(T const& val) const {
    return static_cast<size_t>(1+std::floor(log((val - this->_low)/_div)/_lbase));
  }
  /**
   * @brief Base
   * @return base
   */
  T base() const {
    return _base;
  }

 private:
  T _base, _div;
  double _lbase;
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

  T _base, _div;
};


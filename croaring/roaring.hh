/* auto-generated on Tue Feb  7 06:00:34 EST 2017. Do not edit! */
#include "roaring.h"
/* begin file /code/roaring/CRoaring/cpp/roaring.hh */
/*
A C++ header for Roaring Bitmaps.
*/
#ifndef INCLUDE_ROARING_HH_
#define INCLUDE_ROARING_HH_

#include <stdarg.h>

#include <algorithm>
#include <new>
#include <stdexcept>

class RoaringSetBitForwardIterator;

class Roaring {
   public:
    /**
     * Create an empty bitmap
     */
    Roaring() : roaring(NULL) {
        roaring = roaring_bitmap_create();
        if (roaring == NULL) {
            throw std::runtime_error("failed memory alloc in constructor");
        }
    }

    /**
     * Construct a bitmap from a list of integer values.
     */
    Roaring(size_t n, const uint32_t *data) {
        roaring = roaring_bitmap_of_ptr(n, data);
        if (roaring == NULL) {
            throw std::runtime_error("failed memory alloc in constructor");
        }
    }
    /**
     * Copy constructor
     */
    Roaring(const Roaring &r) : roaring(NULL) {
        roaring = roaring_bitmap_copy(r.roaring);
        if (roaring == NULL) {
            throw std::runtime_error("failed memory alloc in constructor");
        }
    }

    /**
     * Construct a roaring object from the C struct.
     *
     * Passing a NULL point is unsafe.
     */
    Roaring(roaring_bitmap_t *s) : roaring(s) {}

    /**
     * Construct a bitmap from a list of integer values.
     */
    static Roaring bitmapOf(size_t n, ...) {
        Roaring ans;
        va_list vl;
        va_start(vl, n);
        for (size_t i = 0; i < n; i++) {
            ans.add(va_arg(vl, uint32_t));
        }
        va_end(vl);
        return ans;
    }


    /**
     * Add value x
     *
     */
    void add(uint32_t x) { roaring_bitmap_add(roaring, x); }

    /**
     * Add value n_args from pointer vals
     *
     */
    void addMany(size_t n_args, const uint32_t *vals) {
        roaring_bitmap_add_many(roaring, n_args, vals);
    }

    /**
     * Remove value x
     *
     */
    void remove(uint32_t x) { roaring_bitmap_remove(roaring, x); }


    /**
     * Return the largest value (if not empty)
     *
     */
    uint32_t maximum() const { return roaring_bitmap_maximum(roaring); }


    /**
    * Return the smallest value (if not empty)
    *
    */
    uint32_t minimum() const { return roaring_bitmap_minimum(roaring); }

    /**
     * Check if value x is present
     */
    bool contains(uint32_t x) const {
        return roaring_bitmap_contains(roaring, x);
    }

    /**
     * Destructor
     */
    ~Roaring() { roaring_bitmap_free(roaring); }

    /**
     * Copies the content of the provided bitmap, and
     * discard the current content.
     */
    Roaring &operator=(const Roaring &r) {
        roaring_bitmap_free(roaring);
        roaring = roaring_bitmap_copy(r.roaring);
        if (roaring == NULL) {
            throw std::runtime_error("failed memory alloc in assignement");
        }
        return *this;
    }

    /**
     * Compute the intersection between the current bitmap and the provided
     * bitmap,
     * writing the result in the current bitmap. The provided bitmap is not
     * modified.
     */
    Roaring &operator&=(const Roaring &r) {
        roaring_bitmap_and_inplace(roaring, r.roaring);
        return *this;
    }

    /**
     * Compute the difference between the current bitmap and the provided
     * bitmap,
     * writing the result in the current bitmap. The provided bitmap is not
     * modified.
     */
    Roaring &operator-=(const Roaring &r) {
        roaring_bitmap_andnot_inplace(roaring, r.roaring);
        return *this;
    }

    /**
     * Compute the union between the current bitmap and the provided bitmap,
     * writing the result in the current bitmap. The provided bitmap is not
     * modified.
     *
     * See also the fastunion function to aggregate many bitmaps more quickly.
     */
    Roaring &operator|=(const Roaring &r) {
        roaring_bitmap_or_inplace(roaring, r.roaring);
        return *this;
    }

    /**
     * Compute the symmetric union between the current bitmap and the provided
     * bitmap,
     * writing the result in the current bitmap. The provided bitmap is not
     * modified.
     */
    Roaring &operator^=(const Roaring &r) {
        roaring_bitmap_xor_inplace(roaring, r.roaring);
        return *this;
    }

    /**
     * Exchange the content of this bitmap with another.
     */
    void swap(Roaring &r) { std::swap(r.roaring, roaring); }

    /**
     * Get the cardinality of the bitmap (number of elements).
     */
    uint64_t cardinality() const {
        return roaring_bitmap_get_cardinality(roaring);
    }

    /**
    * Returns true if the bitmap is empty (cardinality is zero).
    */
    bool isEmpty() const { return roaring_bitmap_is_empty(roaring); }

    /**
    * Returns true if the bitmap is subset of the other.
    */
    bool isSubset(const Roaring &r) const { return roaring_bitmap_is_subset(roaring, r.roaring); }

    /**
    * Returns true if the bitmap is strict subset of the other.
    */
    bool isStrictSubset(const Roaring &r) const { return roaring_bitmap_is_strict_subset(roaring, r.roaring); }

    /**
     * Convert the bitmap to an array. Write the output to "ans",
     * caller is responsible to ensure that there is enough memory
     * allocated
     * (e.g., ans = new uint32[mybitmap.cardinality()];)
     */
    void toUint32Array(uint32_t *ans) const {
        roaring_bitmap_to_uint32_array(roaring, ans);
    }

    /**
     * Return true if the two bitmaps contain the same elements.
     */
    bool operator==(const Roaring &r) const {
        return roaring_bitmap_equals(roaring, r.roaring);
    }

    /**
     * compute the negation of the roaring bitmap within a specified interval.
     * areas outside the range are passed through unchanged.
     */
    void flip(uint64_t range_start, uint64_t range_end) {
        roaring_bitmap_flip_inplace(roaring, range_start, range_end);
    }

    /**
     *  Remove run-length encoding even when it is more space efficient
     *  return whether a change was applied
     */
    bool removeRunCompression() {
        return roaring_bitmap_remove_run_compression(roaring);
    }

    /** convert array and bitmap containers to run containers when it is more
     * efficient;
     * also convert from run containers when more space efficient.  Returns
     * true if the result has at least one run container.
     * Additional savings might be possible by calling shrinkToFit().
     */
    bool runOptimize() { return roaring_bitmap_run_optimize(roaring); }

    /**
     * If needed, reallocate memory to shrink the memory usage. Returns
     * the number of bytes saved.
    */
    size_t shrinkToFit() { return roaring_bitmap_shrink_to_fit(roaring); }

    /**
     * Iterate over the bitmap elements. The function iterator is called once
     * for
     *  all the values with ptr (can be NULL) as the second parameter of each
     * call.
     *
     *  roaring_iterator is simply a pointer to a function that returns void,
     *  and takes (uint32_t,void*) as inputs.
     */
    void iterate(roaring_iterator iterator, void *ptr) const {
        roaring_iterate(roaring, iterator, ptr);
    }

    /**
     * If the size of the roaring bitmap is strictly greater than rank, then
     * this function returns true and set element to the element of given rank.
     *   Otherwise, it returns false.
     */
    bool select(uint32_t rank, uint32_t *element) const {
        return roaring_bitmap_select(roaring, rank, element);
    }

    /**
    * Returns the number of integers that are smaller or equal to x.
    */
    uint64_t rank(uint32_t x) const {
        return roaring_bitmap_rank(roaring, x);
    }
    /**
     * write a bitmap to a char buffer. This is meant to be compatible with
     * the
     * Java and Go versions. Returns how many bytes were written which should be
     * getSizeInBytes().
     *
     * Setting the portable flag to false enable a custom format that
     * can save space compared to the portable format (e.g., for very
     * sparse bitmaps).
     */
    size_t write(char *buf, bool portable = true) const {
        if (portable)
            return roaring_bitmap_portable_serialize(roaring, buf);
        else
            return roaring_bitmap_serialize(roaring, buf);
    }

    /**
     * read a bitmap from a serialized version. This is meant to be compatible
     * with
     * the
     * Java and Go versions.
     *
     * Setting the portable flag to false enable a custom format that
     * can save space compared to the portable format (e.g., for very
     * sparse bitmaps).
     */
    static Roaring read(const char *buf, bool portable = true) {
        Roaring ans(NULL);
        if (portable)
            ans.roaring = roaring_bitmap_portable_deserialize(buf);
        else
            ans.roaring = roaring_bitmap_deserialize(buf);
        if (ans.roaring == NULL) {
            throw std::runtime_error("failed memory alloc while reading");
        }
        return ans;
    }

    /**
     * How many bytes are required to serialize this bitmap (meant to be
     * compatible
     * with Java and Go versions)
     *
     * Setting the portable flag to false enable a custom format that
     * can save space compared to the portable format (e.g., for very
     * sparse bitmaps).
     */
    size_t getSizeInBytes(bool portable = true) const {
        if (portable)
            return roaring_bitmap_portable_size_in_bytes(roaring);
        else
            return roaring_bitmap_size_in_bytes(roaring);
    }

    /**
     * Computes the intersection between two bitmaps and returns new bitmap.
     * The current bitmap and the provided bitmap are unchanged.
     */
    Roaring operator&(const Roaring &o) const {
        roaring_bitmap_t *r = roaring_bitmap_and(roaring, o.roaring);
        if (r == NULL) {
            throw std::runtime_error("failed materalization in and");
        }
        return Roaring(r);
    }


    /**
     * Computes the difference between two bitmaps and returns new bitmap.
     * The current bitmap and the provided bitmap are unchanged.
     */
    Roaring operator-(const Roaring &o) const {
        roaring_bitmap_t *r = roaring_bitmap_andnot(roaring, o.roaring);
        if (r == NULL) {
            throw std::runtime_error("failed materalization in andnot");
        }
        return Roaring(r);
    }

    /**
     * Computes the union between two bitmaps and returns new bitmap.
     * The current bitmap and the provided bitmap are unchanged.
     */
    Roaring operator|(const Roaring &o) const {
        roaring_bitmap_t *r = roaring_bitmap_or(roaring, o.roaring);
        if (r == NULL) {
            throw std::runtime_error("failed materalization in or");
        }
        return Roaring(r);
    }

    /**
     * Computes the symmetric union between two bitmaps and returns new bitmap.
     * The current bitmap and the provided bitmap are unchanged.
     */
    Roaring operator^(const Roaring &o) const {
        roaring_bitmap_t *r = roaring_bitmap_xor(roaring, o.roaring);
        if (r == NULL) {
            throw std::runtime_error("failed materalization in xor");
        }
        return Roaring(r);
    }

    /**
     * Whether or not we apply copy and write.
     */
    void setCopyOnWrite(bool val) { roaring->copy_on_write = val; }

    /**
     * Print the content of the bitmap
     */
    void printf() { roaring_bitmap_printf(roaring); }

    /**
     * Whether or not copy and write is active.
     */
    bool getCopyOnWrite() const { return roaring->copy_on_write; }

    /**
     * computes the logical or (union) between "n" bitmaps (referenced by a
     * pointer).
     */
    static Roaring fastunion(size_t n, const Roaring **inputs) {
        const roaring_bitmap_t **x =
            (const roaring_bitmap_t **)malloc(n * sizeof(roaring_bitmap_t *));
        if (x == NULL) {
            throw std::runtime_error("failed memory alloc in fastunion");
        }
        for (size_t k = 0; k < n; ++k) x[k] = inputs[k]->roaring;

        Roaring ans(NULL);
        ans.roaring = roaring_bitmap_or_many(n, x);
        if (ans.roaring == NULL) {
            throw std::runtime_error("failed memory alloc in fastunion");
        }
        free(x);
        return ans;
    }

    typedef  RoaringSetBitForwardIterator const_iterator;

    /**
    * Returns an iterator that can be used to access the position of the
    * set bits. The running time complexity of a full scan is proportional to the
    * number
    * of set bits: be aware that if you have long strings of 1s, this can be
    * very inefficient.
    *
    * It can be much faster to use the toArray method if you want to
    * retrieve the set bits.
    */
    const_iterator begin() const ;
    /*{
      return RoaringSetBitForwardIterator(*this);
    }*/

    /**
    * A bogus iterator that can be used together with begin()
    * for constructions such as for(auto i = b.begin();
    * i!=b.end(); ++i) {}
    */
    const_iterator end() const ; /*{
      return RoaringSetBitForwardIterator(*this, true);
    }*/

    roaring_bitmap_t *roaring;
};


/**
 * Used to go through the set bits. Not optimally fast, but convenient.
 */
class RoaringSetBitForwardIterator {
public:
  typedef std::forward_iterator_tag iterator_category;
  typedef uint32_t *pointer;
  typedef uint32_t &reference_type;
  typedef uint32_t value_type;
  typedef int32_t difference_type;
  typedef RoaringSetBitForwardIterator type_of_iterator;

  /**
   * Provides the location of the set bit.
   */
  value_type operator*() const {
    return i->current_value;
  }

  bool operator<(const type_of_iterator &o) {
    return i->current_value < *o;
  }

  bool operator<=(const type_of_iterator &o) {
    return i->current_value <= *o;
  }

  bool operator>(const type_of_iterator &o) {
    return i->current_value > *o;
  }

  bool operator>=(const type_of_iterator &o) {
    return i->current_value >= *o;
  }

  type_of_iterator &operator++() {// ++i, must returned inc. value
    roaring_advance_uint32_iterator(i);
    return *this;
  }

  type_of_iterator operator++(int) {// i++, must return orig. value
    RoaringSetBitForwardIterator orig(*this);
    roaring_advance_uint32_iterator(i);
    return orig;
  }

  bool operator==(const RoaringSetBitForwardIterator &o) {
    return i->current_value == *o;
  }

  bool operator!=(const RoaringSetBitForwardIterator &o) {
    return i->current_value != *o;
  }

  RoaringSetBitForwardIterator(const Roaring & parent, bool exhausted = false) : i(NULL) {
    if(exhausted) {
        i = (roaring_uint32_iterator_t *) malloc(sizeof(roaring_uint32_iterator_t));
        i->parent = parent.roaring;
        i->container_index = INT32_MAX;
        i->has_value = false;
        i->current_value = UINT32_MAX;
    } else {
      i = roaring_create_iterator(parent.roaring);
    }
  }

  virtual ~RoaringSetBitForwardIterator() {
    roaring_free_uint32_iterator(i);
    i = NULL;
  }

  RoaringSetBitForwardIterator(
      const RoaringSetBitForwardIterator &o)
      : i(NULL) {
    i = roaring_copy_uint32_iterator (o.i);
  }



  roaring_uint32_iterator_t *  i;
};


inline RoaringSetBitForwardIterator Roaring::begin() const {
      return RoaringSetBitForwardIterator(*this);
}

inline RoaringSetBitForwardIterator Roaring::end() const {
      return RoaringSetBitForwardIterator(*this, true);
}

#endif /* INCLUDE_ROARING_HH_ */
/* end file /code/roaring/CRoaring/cpp/roaring.hh */
/* begin file /code/roaring/CRoaring/cpp/roaring64map.hh */
/*
A C++ header for 64-bit Roaring Bitmaps, implemented by way of a map of many 32-bit Roaring Bitmaps.
*/
#ifndef INCLUDE_ROARING_64_MAP_HH_
#define INCLUDE_ROARING_64_MAP_HH_

#include <cstdarg>
#include <cstdio>
#include <algorithm>
#include <new>
#include <stdexcept>
#include <utility>
#include <map>
#include <numeric>
#include <limits>


class Roaring64MapSetBitForwardIterator;

class Roaring64Map{
   public:
    /**
     * Create an empty bitmap
     */
    Roaring64Map() = default;

    /**
     * Construct a bitmap from a list of 32-bit integer values.
     */
    Roaring64Map(size_t n, const uint32_t *data) {
        addMany(n, data);
    }

    /**
     * Construct a bitmap from a list of 64-bit integer values.
     */
    Roaring64Map(size_t n, const uint64_t *data) {
        addMany(n, data);
    }

    /**
     * Copy constructor
     */
    Roaring64Map(const Roaring64Map &r) : roarings(r.roarings) { }

    /**
     * Construct a 64-bit map from a 32-bit one
     */
    Roaring64Map(const Roaring &r) { emplaceOrInsert(0, r); }

    /**
     * Construct a roaring object from the C struct.
     *
     * Passing a NULL point is unsafe.
     */
    Roaring64Map(roaring_bitmap_t *s) { emplaceOrInsert(0, s); }

    /**
     * Construct a bitmap from a list of integer values.
     */
    static Roaring64Map bitmapOf(size_t n...) {
        Roaring64Map ans;
        va_list vl;
        va_start(vl, n);
        for (size_t i = 0; i < n; i++) {
            ans.add(va_arg(vl, uint64_t));
        }
        va_end(vl);
        return ans;
    }


    /**
     * Add value x
     *
     */
    void add(uint32_t x) {
        roarings[0].add(x);
        roarings[0].setCopyOnWrite(copyOnWrite);
    }
    void add(uint64_t x) {
        roarings[highBytes(x)].add(lowBytes(x));
        roarings[highBytes(x)].setCopyOnWrite(copyOnWrite);
    }

    /**
     * Add value n_args from pointer vals
     *
     */
    void addMany(size_t n_args, const uint32_t *vals) {
        for (size_t lcv = 0; lcv < n_args; lcv++) {
            roarings[0].add(vals[lcv]);
            roarings[0].setCopyOnWrite(copyOnWrite);
        }
    }
    void addMany(size_t n_args, const uint64_t *vals) {
        for (size_t lcv = 0; lcv < n_args; lcv++) {
            roarings[highBytes(vals[lcv])].add(lowBytes(vals[lcv]));
            roarings[highBytes(vals[lcv])].setCopyOnWrite(copyOnWrite);
        }
    }

    /**
     * Remove value x
     *
     */
    void remove(uint32_t x) { roarings[0].remove(x); }
    void remove(uint64_t x) {
        auto roaring_iter = roarings.find(highBytes(x));
        if (roaring_iter != roarings.cend())
            roaring_iter->second.remove(lowBytes(x));
    }

    /**
     * Return the largest value (if not empty)
     *
     */
    uint64_t maximum() const {
        for (auto roaring_iter = roarings.crbegin(); roaring_iter != roarings.crend(); ++roaring_iter) {
            if (!roaring_iter->second.isEmpty()) {
                return uniteBytes(roaring_iter->first, roaring_iter->second.maximum());
            }
        }
        return std::numeric_limits<uint64_t>::min();
    }


    /**
     * Return the smallest value (if not empty)
     *
     */
    uint64_t minimum() const {
        for (auto roaring_iter = roarings.cbegin(); roaring_iter != roarings.cend(); ++roaring_iter) {
            if (!roaring_iter->second.isEmpty()) {
                return uniteBytes(roaring_iter->first, roaring_iter->second.minimum());
            }
        }
        return std::numeric_limits<uint64_t>::max();
    }


    /**
     * Check if value x is present
     */
    bool contains(uint32_t x) const {
        return roarings.at(0).contains(x);
    }
    bool contains(uint64_t x) const {
        return roarings.at(highBytes(x)).contains(lowBytes(x));
    }

    /**
     * Destructor
     */
    ~Roaring64Map() = default;

    /**
     * Copies the content of the provided bitmap, and
     * discards the current content.
     */
    Roaring64Map &operator=(const Roaring64Map &r) {
        roarings = r.roarings;
        copyOnWrite = r.copyOnWrite;
        return *this;
    }

    /**
     * Compute the intersection between the current bitmap and the provided
     * bitmap,
     * writing the result in the current bitmap. The provided bitmap is not
     * modified.
     */
    Roaring64Map &operator&=(const Roaring64Map &r) {
        for (const auto& map_entry : r.roarings) {
            if (roarings.count(map_entry.first) == 1) {
                roarings[map_entry.first] &= map_entry.second;
                roarings[map_entry.first].setCopyOnWrite(copyOnWrite);
            }
        }
        return *this;
    }

    /**
     * Compute the difference between the current bitmap and the provided
     * bitmap,
     * writing the result in the current bitmap. The provided bitmap is not
     * modified.
     */
    Roaring64Map &operator-=(const Roaring64Map &r) {
        for (auto& map_entry : roarings) {
            if (r.roarings.count(map_entry.first) == 1)
                map_entry.second -= r.roarings.at(map_entry.first);
        }
        return *this;
    }

    /**
     * Compute the union between the current bitmap and the provided bitmap,
     * writing the result in the current bitmap. The provided bitmap is not
     * modified.
     *
     * See also the fastunion function to aggregate many bitmaps more quickly.
     */
    Roaring64Map &operator|=(const Roaring64Map &r) {
        for (const auto& map_entry : r.roarings) {
            if (roarings.count(map_entry.first) == 0) {
                roarings[map_entry.first] = map_entry.second;
                roarings[map_entry.first].setCopyOnWrite(copyOnWrite);
            }
            else
                roarings[map_entry.first] |= map_entry.second;
        }
        return *this;
    }

    /**
     * Compute the symmetric union between the current bitmap and the provided
     * bitmap,
     * writing the result in the current bitmap. The provided bitmap is not
     * modified.
     */
    Roaring64Map &operator^=(const Roaring64Map &r) {
        for (const auto& map_entry : r.roarings) {
            if (roarings.count(map_entry.first) == 0) {
                roarings[map_entry.first] = map_entry.second;
                roarings[map_entry.first].setCopyOnWrite(copyOnWrite);
            }
            else
                roarings[map_entry.first] ^= map_entry.second;
        }
        return *this;
    }

    /**
     * Exchange the content of this bitmap with another.
     */
    void swap(Roaring64Map &r) { roarings.swap(r.roarings); }

    /**
     * Get the cardinality of the bitmap (number of elements).
     * Throws std::length_error in the special case where the bitmap is full
     * (cardinality() == 2^64). Check isFull() before calling to avoid exception.
     */
    uint64_t cardinality() const {
        if (isFull()) {
            throw std::length_error("bitmap is full, cardinality is 2^64, "
                                    "unable to represent in a 64-bit integer");
        }
        return std::accumulate(roarings.cbegin(), roarings.cend(), 0,
            [](uint64_t previous, const std::pair<uint32_t, Roaring>& map_entry) {
                return previous + map_entry.second.cardinality();
            });
    }

    /**
    * Returns true if the bitmap is empty (cardinality is zero).
    */
    bool isEmpty() const {
        return std::all_of(roarings.cbegin(), roarings.cend(),
            [](const std::pair<uint32_t, Roaring>& map_entry) {
                return map_entry.second.isEmpty();
            });
    }

    /**
    * Returns true if the bitmap is full (cardinality is max uint64_t + 1).
    */
    bool isFull() const {
        // only bother to check if map is fully saturated
        return roarings.size() == ((size_t)std::numeric_limits<uint32_t>::max()) + 1 ?
            std::all_of(roarings.cbegin(), roarings.cend(),
            [](const std::pair<uint32_t, Roaring>& map_entry) {
                // roarings within map are saturated if cardinality is uint32_t max + 1
                return map_entry.second.cardinality() == ((uint64_t)std::numeric_limits<uint32_t>::max()) + 1;
            }) : false;
    }

    /**
    * Returns true if the bitmap is subset of the other.
    */
    bool isSubset(const Roaring64Map &r) const {
        for (const auto& map_entry : roarings) {
            auto roaring_iter = r.roarings.find(map_entry.first);
            if (roaring_iter == roarings.cend())
                return false;
            else
                if (!map_entry.second.isSubset(roaring_iter->second))
                    return false;
        }
        return true;
    }

    /**
    * Returns true if the bitmap is strict subset of the other.
    * Throws std::length_error in the special case where the bitmap is full
    * (cardinality() == 2^64). Check isFull() before calling to avoid exception.
    */
    bool isStrictSubset(const Roaring64Map &r) const { return isSubset(r) && cardinality() != r.cardinality(); }

    /**
     * Convert the bitmap to an array. Write the output to "ans",
     * caller is responsible to ensure that there is enough memory
     * allocated
     * (e.g., ans = new uint32[mybitmap.cardinality()];)
     */
    void toUint64Array(uint64_t *ans) const {
        std::accumulate(roarings.cbegin(), roarings.cend(), ans,
            [](uint64_t* previous, const std::pair<uint32_t, Roaring>& map_entry) {
                for (uint32_t low_bits : map_entry.second)
                    *previous++ = uniteBytes(map_entry.first, low_bits);
                return previous;
            });
    }

    /**
     * Return true if the two bitmaps contain the same elements.
     */
    bool operator==(const Roaring64Map &r) const {
        // we cannot use operator == on the map because either side may contain empty Roaring Bitmaps
        auto lhs_iter = roarings.cbegin();
        auto rhs_iter = r.roarings.cbegin();
        do {
            // if the left map has reached its end, ensure that the right map contains only empty Bitmaps
            if (lhs_iter == roarings.cend()) {
                while (rhs_iter != r.roarings.cend()) {
                    if (rhs_iter->second.isEmpty()) {
                        ++rhs_iter;
                        continue;
                    }
                    return false;
                }
                return true;
            }
            // if the left map has an empty bitmap, skip it
            if (lhs_iter->second.isEmpty()) {
                ++lhs_iter;
                continue;
            }

            do {
                // if the right map has reached its end, ensure that the right map contains only empty Bitmaps
                if (rhs_iter == r.roarings.cend()) {
                    while (lhs_iter != roarings.cend()) {
                        if (lhs_iter->second.isEmpty()) {
                            ++lhs_iter;
                            continue;
                        }
                        return false;
                    }
                    return true;
                }
                // if the right map has an empty bitmap, skip it
                if (rhs_iter->second.isEmpty()) {
                    ++rhs_iter;
                    continue;
                }
            } while (false);
        // if neither map has reached its end ensure elements are equal and move to the next element in both
        } while (lhs_iter++->second == rhs_iter++->second);
        return false;
    }

    /**
     * compute the negation of the roaring bitmap within a specified interval.
     * areas outside the range are passed through unchanged.
     */
    void flip(uint64_t range_start, uint64_t range_end) {
        uint32_t start_high = highBytes(range_start);
        uint32_t start_low = lowBytes(range_start);
        uint32_t end_high = highBytes(range_end);
        uint32_t end_low = lowBytes(range_end);

        if (start_high == end_high) {
            roarings[start_high].flip(start_low, end_low);
            return;
        }
        roarings[start_high].flip(start_low, std::numeric_limits<uint32_t>::max());
        roarings[start_high++].setCopyOnWrite(copyOnWrite);

        for ( ; start_high <= highBytes(range_end)-1; ++start_high) {
            roarings[start_high].flip(std::numeric_limits<uint32_t>::min(),
                                      std::numeric_limits<uint32_t>::max());
            roarings[start_high].setCopyOnWrite(copyOnWrite);
        }

        roarings[start_high].flip(std::numeric_limits<uint32_t>::min(), end_low);
        roarings[start_high].setCopyOnWrite(copyOnWrite);
    }

    /**
     *  Remove run-length encoding even when it is more space efficient
     *  return whether a change was applied
     */
    bool removeRunCompression() {
        return std::accumulate(roarings.begin(), roarings.end(), false,
            [](bool previous, std::pair<const uint32_t, Roaring>& map_entry) {
                return map_entry.second.removeRunCompression() && previous;
            });
    }

    /** convert array and bitmap containers to run containers when it is more
     * efficient;
     * also convert from run containers when more space efficient.  Returns
     * true if the result has at least one run container.
     * Additional savings might be possible by calling shrinkToFit().
     */
    bool runOptimize() {
        return std::accumulate(roarings.begin(), roarings.end(), false,
            [](bool previous, std::pair<const uint32_t, Roaring>& map_entry) {
                return map_entry.second.runOptimize() && previous;
            });
    }

    /**
     * If needed, reallocate memory to shrink the memory usage. Returns
     * the number of bytes saved.
    */
    size_t shrinkToFit() {
        size_t savedBytes = 0;
        auto iter = roarings.begin();
        while (iter != roarings.cend()) {
            if (iter->second.isEmpty()) {
                // empty Roarings are 84 bytes
                savedBytes += 88;
                roarings.erase(iter++);
            } else {
                savedBytes += iter->second.shrinkToFit();
                iter++;
            }
        }
        return savedBytes;
    }

    /**
     * Iterate over the bitmap elements. The function iterator is called once
     * for
     *  all the values with ptr (can be NULL) as the second parameter of each
     * call.
     *
     *  roaring_iterator64 is simply a pointer to a function that returns void,
     *  and takes (uint64_t,void*) as inputs.
     */
    void iterate(roaring_iterator64 iterator, void *ptr) const {
        std::for_each(roarings.begin(), roarings.cend(),
            [=](const std::pair<uint32_t, Roaring>& map_entry) {
                roaring_iterate64(map_entry.second.roaring, iterator, uint64_t(map_entry.first) << 32, ptr);
            });
    }

    /**
     * If the size of the roaring bitmap is strictly greater than rank, then
     this
       function returns true and set element to the element of given rank.
       Otherwise, it returns false.
     */
    bool select(uint64_t rank, uint32_t *element) const {
        for (const auto& map_entry : roarings) {
            uint64_t sub_cardinality = (uint64_t)map_entry.second.cardinality();
            if (rank < sub_cardinality) {
                return map_entry.second.select(rank, element);
            }
            rank -= sub_cardinality;
        }
        return false;
    }

    /**
    * Returns the number of integers that are smaller or equal to x.
    */
    uint64_t rank(uint64_t x) const {
        uint64_t result = 0;
        auto roaring_destination = roarings.find(highBytes(x));
        if (roaring_destination != roarings.cend()) {
            for (auto roaring_iter = roarings.cbegin(); roaring_iter != roaring_destination; ++roaring_iter) {
                result += roaring_iter->second.cardinality();
            }
            result += roaring_destination->second.rank(lowBytes(x));
            return result;
        }
        roaring_destination = roarings.lower_bound(highBytes(x));
        for (auto roaring_iter = roarings.cbegin(); roaring_iter != roaring_destination; ++roaring_iter) {
            result += roaring_iter->second.cardinality();
        }
        return result;
    }

    /**
     * write a bitmap to a char buffer. This is meant to be compatible with
     * the
     * Java and Go versions. Returns how many bytes were written which should be
     * getSizeInBytes().
     *
     * Setting the portable flag to false enable a custom format that
     * can save space compared to the portable format (e.g., for very
     * sparse bitmaps).
     */
    size_t write(char *buf, bool portable = true) const {
        const char* orig = buf;
        // push map size
        *((uint32_t*)buf) = roarings.size();
        buf += sizeof(uint32_t);
        std::for_each(roarings.cbegin(), roarings.cend(),
            [&buf, portable](const std::pair<uint32_t, Roaring>& map_entry) {
                // push map key
                *((uint32_t*)buf) = map_entry.first;
                buf += sizeof(uint32_t);
                // byte count in Roaring goes here
                // TODO: lower-level read API should return bytes read so we don't have to store this
                size_t* const writeBytesHere = (size_t*)buf;
                buf += sizeof(size_t);
                *writeBytesHere = map_entry.second.write(buf, portable);
                buf += *writeBytesHere;
            });
        return buf - orig;
    }

    /**
     * read a bitmap from a serialized version. This is meant to be compatible
     * with
     * the
     * Java and Go versions.
     *
     * Setting the portable flag to false enable a custom format that
     * can save space compared to the portable format (e.g., for very
     * sparse bitmaps).
     */
    static Roaring64Map read(const char *buf, bool portable = true) {
        Roaring64Map result;
        // get map size
        uint32_t map_size = *((uint32_t*)buf);
        buf += sizeof(uint32_t);
        for (uint32_t lcv = 0; lcv < map_size; lcv++) {
            // get map key
            uint32_t key = *((uint32_t*)buf);
            buf += sizeof(uint32_t);
            // read Bytes in Roaring
            size_t bytesInRoaring = *((size_t*)buf);
            buf += sizeof(size_t);
            // read Roaring
            result.emplaceOrInsert(key, Roaring::read(buf, portable));
            // forward buffer past the last Roaring Bitmap
            // TODO: lower-level read API should return bytes read so we don't have to store this
            buf += bytesInRoaring;
        }
        return result;
    }

    /**
     * How many bytes are required to serialize this bitmap (meant to be
     * compatible
     * with Java and Go versions)
     *
     * Setting the portable flag to false enable a custom format that
     * can save space compared to the portable format (e.g., for very
     * sparse bitmaps).
     */
    size_t getSizeInBytes(bool portable = true) const {
        // start with, respectively, map size and for each map entry, size of keys and Roaring serialized bytes
        return std::accumulate(roarings.cbegin(), roarings.cend(),
                               sizeof(uint32_t) + roarings.size() * (sizeof(uint32_t) + sizeof(size_t)),
            [=](uint64_t previous, const std::pair<uint32_t, Roaring>& map_entry) {
                // add in bytes used by each Roaring
                return previous + map_entry.second.getSizeInBytes(portable);
            });
    }

    /**
     * Computes the intersection between two bitmaps and returns new bitmap.
     * The current bitmap and the provided bitmap are unchanged.
     */
    Roaring64Map operator&(const Roaring64Map &o) const {
        return Roaring64Map(*this) &= o;
    }

    /**
     * Computes the difference between two bitmaps and returns new bitmap.
     * The current bitmap and the provided bitmap are unchanged.
     */
    Roaring64Map operator-(const Roaring64Map &o) const {
        return Roaring64Map(*this) -= o;
    }

    /**
     * Computes the union between two bitmaps and returns new bitmap.
     * The current bitmap and the provided bitmap are unchanged.
     */
    Roaring64Map operator|(const Roaring64Map &o) const {
        return Roaring64Map(*this) |= o;
    }

    /**
     * Computes the symmetric union between two bitmaps and returns new bitmap.
     * The current bitmap and the provided bitmap are unchanged.
     */
    Roaring64Map operator^(const Roaring64Map &o) const {
        return Roaring64Map(*this) ^= o;
    }



    /**
     * Whether or not we apply copy and write.
     */
    void setCopyOnWrite(bool val) {
        if (copyOnWrite == val)
            return;
        copyOnWrite = val;
        std::for_each(roarings.begin(), roarings.end(),
            [=](std::pair<const uint32_t, Roaring>& map_entry) {
                map_entry.second.setCopyOnWrite(val);
            });
    }

    /**
     * Print the content of the bitmap
     */
    void printf() {
        if (!isEmpty()) {
            auto map_iter = roarings.cbegin();
            while (map_iter->second.isEmpty())
                ++map_iter;
            struct iter_data {
                uint32_t high_bits;
                char first_char = '{';
            } outer_iter_data;
            outer_iter_data.high_bits = roarings.begin()->first;
            map_iter->second.iterate([](uint32_t low_bits, void* inner_iter_data)->bool
                { std::printf("%c%llu", ((iter_data*)inner_iter_data)->first_char,
                  (long long unsigned)uniteBytes(((iter_data*)inner_iter_data)->high_bits, low_bits));
                  if (((iter_data*)inner_iter_data)->first_char == '{')
                     ((iter_data*)inner_iter_data)->first_char = ',';
                  return true; }, (void*)&outer_iter_data);
            std::for_each(++map_iter, roarings.cend(),
                [](const std::pair<uint32_t, Roaring>& map_entry) {
                    map_entry.second.iterate([](uint32_t low_bits, void* high_bits)->bool
                        { std::printf(",%llu", (long long unsigned)uniteBytes(*(uint32_t*)high_bits, low_bits));
                          return true; }, (void*)&map_entry.first);
                });
        }
        else
            std::printf("{");
        std::printf("}\n");
    }

    /**
     * Whether or not copy and write is active.
     */
    bool getCopyOnWrite() const { return copyOnWrite; }

    /**
     * computes the logical or (union) between "n" bitmaps (referenced by a
     * pointer).
     */
    static Roaring64Map fastunion(size_t n, const Roaring64Map **inputs) {
        Roaring64Map ans;
        // not particularly fast
        for (size_t lcv = 0; lcv < n ; ++lcv) {
            ans |= *(inputs[lcv]);
        }
        return ans;
    }

    friend class Roaring64MapSetBitForwardIterator;
    typedef Roaring64MapSetBitForwardIterator const_iterator;

    /**
    * Returns an iterator that can be used to access the position of the
    * set bits. The running time complexity of a full scan is proportional to the
    * number
    * of set bits: be aware that if you have long strings of 1s, this can be
    * very inefficient.
    *
    * It can be much faster to use the toArray method if you want to
    * retrieve the set bits.
    */
    const_iterator begin() const;

    /**
    * A bogus iterator that can be used together with begin()
    * for constructions such as for(auto i = b.begin();
    * i!=b.end(); ++i) {}
    */
    const_iterator end() const;

private:
    std::map<uint32_t, Roaring> roarings;
    bool copyOnWrite = false;
    static uint32_t highBytes(const uint64_t in) { return uint32_t(in >> 32); }
    static uint32_t lowBytes(const uint64_t in) { return uint32_t(in); }
    static uint64_t uniteBytes(const uint32_t highBytes, const uint32_t lowBytes) {
        return (uint64_t(highBytes) << 32) | uint64_t(lowBytes);
    }
    // this is needed to tolerate gcc's C++11 libstdc++ lacking emplace
    // prior to version 4.8
    void emplaceOrInsert(const uint32_t key, const Roaring& value) {
#if defined(__GLIBCXX__) && __GLIBCXX__ < 20130322
    roarings.insert(std::make_pair(key, value));
#else
    roarings.emplace(std::make_pair(key, value));
#endif
    }
};


/**
 * Used to go through the set bits. Not optimally fast, but convenient.
 */
class Roaring64MapSetBitForwardIterator {
public:
    typedef std::forward_iterator_tag iterator_category;
    typedef uint64_t *pointer;
    typedef uint64_t &reference_type;
    typedef uint64_t value_type;
    typedef int64_t difference_type;
    typedef Roaring64MapSetBitForwardIterator type_of_iterator;

  /**
   * Provides the location of the set bit.
   */
    value_type operator*() const {
        return Roaring64Map::uniteBytes(map_iter->first, i->current_value);
    }

    bool operator<(const type_of_iterator &o) {
        if (map_iter == map_end) return false;
        if (o.map_iter == o.map_end) return true;
        return **this < *o;
    }

    bool operator<=(const type_of_iterator &o) {
        if (o.map_iter == o.map_end) return true;
        if (map_iter == map_end) return false;
        return **this <= *o;
    }

    bool operator>(const type_of_iterator &o) {
        if (o.map_iter == o.map_end) return false;
        if (map_iter == map_end) return true;
        return **this > *o;
    }

    bool operator>=(const type_of_iterator &o) {
        if (map_iter == map_end) return true;
        if (o.map_iter == o.map_end) return false;
        return **this >= *o;
    }

    type_of_iterator &operator++() {// ++i, must returned inc. value
        if (i->has_value == true)
            roaring_advance_uint32_iterator(i);
        while (!i->has_value) {
            map_iter++;
            if (map_iter == map_end)
                return *this;
            roaring_free_uint32_iterator(i);
            i = roaring_create_iterator(map_iter->second.roaring);
        }
        return *this;
    }

    type_of_iterator operator++(int) {// i++, must return orig. value
        Roaring64MapSetBitForwardIterator orig(*this);
        roaring_advance_uint32_iterator(i);
        while (!i->has_value) {
            map_iter++;
            if (map_iter == map_end)
                return orig;
            roaring_free_uint32_iterator(i);
            i = roaring_create_iterator(map_iter->second.roaring);
        }
        return orig;
    }

    bool operator==(const Roaring64MapSetBitForwardIterator &o) {
        if (map_iter == map_end && o.map_iter == o.map_end) return true;
        if (o.map_iter == o.map_end) return false;
        return **this == *o;
    }

    bool operator!=(const Roaring64MapSetBitForwardIterator &o) {
        if (map_iter == map_end && o.map_iter == o.map_end) return false;
        if (o.map_iter == o.map_end) return true;
        return **this != *o;
    }

    Roaring64MapSetBitForwardIterator(const Roaring64Map& parent, bool exhausted = false)
        : map_end(parent.roarings.cend())
    {
        if(exhausted) {
            map_iter = parent.roarings.cend();
            i = nullptr;
        } else {
            map_iter = parent.roarings.cbegin();
            i = roaring_create_iterator(map_iter->second.roaring);
            while (!i->has_value) {
                map_iter++;
                if (map_iter == map_end)
                    return;
                roaring_free_uint32_iterator(i);
                i = roaring_create_iterator(map_iter->second.roaring);
            }
        }
    }

    ~Roaring64MapSetBitForwardIterator() {
        roaring_free_uint32_iterator(i);
    }

    Roaring64MapSetBitForwardIterator(
        const Roaring64MapSetBitForwardIterator &o)
        : map_iter(o.map_iter), map_end(o.map_end) {
        i = roaring_copy_uint32_iterator (o.i);
    }

private:
    std::map<uint32_t, Roaring>::const_iterator map_iter;
    std::map<uint32_t, Roaring>::const_iterator map_end;
    roaring_uint32_iterator_t* i;
};


inline Roaring64MapSetBitForwardIterator Roaring64Map::begin() const {
      return Roaring64MapSetBitForwardIterator(*this);
}

inline Roaring64MapSetBitForwardIterator Roaring64Map::end() const {
      return Roaring64MapSetBitForwardIterator(*this, true);
}

#endif /* INCLUDE_ROARING_64_MAP_HH_ */
/* end file /code/roaring/CRoaring/cpp/roaring64map.hh */

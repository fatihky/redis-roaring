#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <roaring/roaring.h>

#include "test.h"


// arrays expected to both be sorted.
static int array_equals(uint32_t *a1, int32_t size1, uint32_t *a2,
                        int32_t size2) {
    if (size1 != size2) return 0;
    for (int i = 0; i < size1; ++i)
        if (a1[i] != a2[i]) {
            return 0;
        }
    return 1;
}

bool roaring_iterator_sumall(uint32_t value, void *param) {
    *(uint32_t *)param += value;
    return true;  // continue till the end
}

void can_add_to_copies(bool copy_on_write) {
    roaring_bitmap_t *bm1 = roaring_bitmap_create();
    bm1->copy_on_write = copy_on_write;
    roaring_bitmap_add(bm1, 3);
    roaring_bitmap_t *bm2 = roaring_bitmap_copy(bm1);
    assert(roaring_bitmap_get_cardinality(bm1) == 1);
    assert(roaring_bitmap_get_cardinality(bm2) == 1);
    roaring_bitmap_add(bm2, 4);
    roaring_bitmap_add(bm1, 5);
    assert(roaring_bitmap_get_cardinality(bm1) == 2);
    assert(roaring_bitmap_get_cardinality(bm2) == 2);
    roaring_bitmap_free(bm1);
    roaring_bitmap_free(bm2);
}


void test_stats() {
    // create a new empty bitmap
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    assert_non_null(r1);
    // then we can add values
    for (uint32_t i = 100; i < 1000; i++) {
        roaring_bitmap_add(r1, i);
    }
    for (uint32_t i = 1000; i < 100000; i += 10) {
        roaring_bitmap_add(r1, i);
    }
    roaring_bitmap_add(r1, 100000);

    roaring_statistics_t stats;
    roaring_bitmap_statistics(r1, &stats);
    assert_true(stats.cardinality == roaring_bitmap_get_cardinality(r1));
    assert_true(stats.min_value == 100);
    assert_true(stats.max_value == 100000);
    roaring_bitmap_free(r1);
}

// this should expose memory leaks (https://github.com/RoaringBitmap/CRoaring/pull/70)
void leaks_with_empty(bool copy_on_write) {
    roaring_bitmap_t *empty = roaring_bitmap_create();
    empty->copy_on_write = copy_on_write;
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    r1->copy_on_write = copy_on_write;
    for (uint32_t i = 100; i < 70000; i+=3) {
        roaring_bitmap_add(r1, i);
    }
    roaring_bitmap_t *ror = roaring_bitmap_or(r1,empty);
    roaring_bitmap_t *rxor = roaring_bitmap_xor(r1,empty);
    roaring_bitmap_t *rand = roaring_bitmap_and(r1,empty);
    roaring_bitmap_t *randnot = roaring_bitmap_andnot(r1,empty);
    roaring_bitmap_free(empty);
    assert_true(roaring_bitmap_equals(ror,r1));
    roaring_bitmap_free(ror);
    assert_true(roaring_bitmap_equals(rxor,r1));
    roaring_bitmap_free(rxor);
    assert_true(roaring_bitmap_equals(randnot,r1));
    roaring_bitmap_free(randnot);
    roaring_bitmap_free(r1);
    assert_true(roaring_bitmap_is_empty(rand));
    roaring_bitmap_free(rand);
}

void leaks_with_empty_true() {
  leaks_with_empty(true);
}

void leaks_with_empty_false() {
  leaks_with_empty(false);
}

void test_example(bool copy_on_write) {
    // create a new empty bitmap
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    r1->copy_on_write = copy_on_write;
    assert_non_null(r1);

    // then we can add values
    for (uint32_t i = 100; i < 1000; i++) {
        roaring_bitmap_add(r1, i);
    }

    // check whether a value is contained
    assert_true(roaring_bitmap_contains(r1, 500));

    // compute how many bits there are:
    uint32_t cardinality = roaring_bitmap_get_cardinality(r1);
    printf("Cardinality = %d \n", cardinality);

    // if your bitmaps have long runs, you can compress them by calling
    // run_optimize
    uint32_t size = roaring_bitmap_portable_size_in_bytes(r1);
    roaring_bitmap_run_optimize(r1);
    uint32_t compact_size = roaring_bitmap_portable_size_in_bytes(r1);

    printf("size before run optimize %d bytes, and after %d bytes\n", size,
           compact_size);

    // create a new bitmap with varargs
    roaring_bitmap_t *r2 = roaring_bitmap_of(5, 1, 2, 3, 5, 6);
    assert_non_null(r2);

    roaring_bitmap_printf(r2);

    // we can also create a bitmap from a pointer to 32-bit integers
    const uint32_t values[] = {2, 3, 4};
    roaring_bitmap_t *r3 = roaring_bitmap_of_ptr(3, values);
    r3->copy_on_write = copy_on_write;

    // we can also go in reverse and go from arrays to bitmaps
    uint64_t card1 = roaring_bitmap_get_cardinality(r1);
    uint32_t *arr1 = (uint32_t *)malloc(card1 * sizeof(uint32_t));
    assert(arr1 != NULL);
    roaring_bitmap_to_uint32_array(r1, arr1);

    roaring_bitmap_t *r1f = roaring_bitmap_of_ptr(card1, arr1);
    free(arr1);
    assert_non_null(r1f);

    // bitmaps shall be equal
    assert_true(roaring_bitmap_equals(r1, r1f));
    roaring_bitmap_free(r1f);

    // we can copy and compare bitmaps
    roaring_bitmap_t *z = roaring_bitmap_copy(r3);
    z->copy_on_write = copy_on_write;
    assert_true(roaring_bitmap_equals(r3, z));

    roaring_bitmap_free(z);

    // we can compute union two-by-two
    roaring_bitmap_t *r1_2_3 = roaring_bitmap_or(r1, r2);
    r1_2_3->copy_on_write = copy_on_write;
    roaring_bitmap_or_inplace(r1_2_3, r3);

    // we can compute a big union
    const roaring_bitmap_t *allmybitmaps[] = {r1, r2, r3};
    roaring_bitmap_t *bigunion = roaring_bitmap_or_many(3, allmybitmaps);
    assert_true(roaring_bitmap_equals(r1_2_3, bigunion));
    roaring_bitmap_t *bigunionheap =
        roaring_bitmap_or_many_heap(3, allmybitmaps);
    assert_true(roaring_bitmap_equals(r1_2_3, bigunionheap));
    roaring_bitmap_free(r1_2_3);
    roaring_bitmap_free(bigunion);
    roaring_bitmap_free(bigunionheap);

    // we can compute xor two-by-two
    roaring_bitmap_t *rx1_2_3 = roaring_bitmap_xor(r1, r2);
    rx1_2_3->copy_on_write = copy_on_write;
    roaring_bitmap_xor_inplace(rx1_2_3, r3);

    // we can compute a big xor
    const roaring_bitmap_t *allmybitmaps_x[] = {r1, r2, r3};
    roaring_bitmap_t *bigxor = roaring_bitmap_xor_many(3, allmybitmaps_x);
    assert_true(roaring_bitmap_equals(rx1_2_3, bigxor));

    roaring_bitmap_free(rx1_2_3);
    roaring_bitmap_free(bigxor);

    // we can compute intersection two-by-two
    roaring_bitmap_t *i1_2 = roaring_bitmap_and(r1, r2);
    roaring_bitmap_free(i1_2);

    // we can write a bitmap to a pointer and recover it later
    uint32_t expectedsize = roaring_bitmap_portable_size_in_bytes(r1);
    char *serializedbytes = malloc(expectedsize);
    size_t actualsize = roaring_bitmap_portable_serialize(r1, serializedbytes);
    assert_int_equal(actualsize, expectedsize);
    roaring_bitmap_t *t = roaring_bitmap_portable_deserialize(serializedbytes);
    assert_true(roaring_bitmap_equals(r1, t));
    roaring_bitmap_free(t);
    free(serializedbytes);

    // we can iterate over all values using custom functions
    uint32_t counter = 0;
    roaring_iterate(r1, roaring_iterator_sumall, &counter);

    /**
     * bool roaring_iterator_sumall(uint32_t value, void *param) {
     *        *(uint32_t *) param += value;
     *        return true; // continue till the end
     *  }
     *
     */

    // we can also create iterator structs
    counter = 0;
    roaring_uint32_iterator_t *  i = roaring_create_iterator(r1);
    while(i->has_value) {
      counter++;
      roaring_advance_uint32_iterator(i);
    }
    roaring_free_uint32_iterator(i);
    assert_true(roaring_bitmap_get_cardinality(r1) == counter);


    roaring_bitmap_free(r1);
    roaring_bitmap_free(r2);
    roaring_bitmap_free(r3);
}


void test_uint32_iterator() {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    for(uint32_t i = 0; i < 66000; i+=3) {
      roaring_bitmap_add(r1,i);
    }
    for(uint32_t i = 100000; i < 200000; i++) {
      roaring_bitmap_add(r1,i);
    }
    for(uint32_t i = 300000; i < 500000; i+=100) {
          roaring_bitmap_add(r1,i);
    }
    for(uint32_t i = 600000; i < 700000; i+=1) {
          roaring_bitmap_add(r1,i);
    }
    for(uint32_t i = 800000; i < 900000; i+=7) {
          roaring_bitmap_add(r1,i);
    }
    roaring_bitmap_run_optimize(r1);
    roaring_uint32_iterator_t *  iter = roaring_create_iterator(r1);
    for(uint32_t i = 0; i < 66000; i+=3) {
      assert_true(iter->has_value);
      assert_true(iter->current_value == i);
      roaring_advance_uint32_iterator(iter);
    }
    for(uint32_t i = 100000; i < 200000; i++) {
      assert_true(iter->has_value);
      assert_true(iter->current_value == i);
      roaring_advance_uint32_iterator(iter);
    }
    for(uint32_t i = 300000; i < 500000; i+=100) {
      assert_true(iter->has_value);
      assert_true(iter->current_value == i);
      roaring_advance_uint32_iterator(iter);
    }
    for(uint32_t i = 600000; i < 700000; i+=1) {
      assert_true(iter->has_value);
      assert_true(iter->current_value == i);
      roaring_advance_uint32_iterator(iter);
    }
    for(uint32_t i = 800000; i < 900000; i+=7) {
      assert_true(iter->has_value);
      assert_true(iter->current_value == i);
      roaring_advance_uint32_iterator(iter);
    }
    assert_false(iter->has_value);
    roaring_free_uint32_iterator(iter);
    roaring_bitmap_free(r1);
}


void test_example_true() { test_example(true); }

void test_example_false() { test_example(false); }

void can_remove_from_copies(bool copy_on_write) {
    roaring_bitmap_t *bm1 = roaring_bitmap_create();
    bm1->copy_on_write = copy_on_write;
    roaring_bitmap_add(bm1, 3);
    roaring_bitmap_t *bm2 = roaring_bitmap_copy(bm1);
    assert(roaring_bitmap_get_cardinality(bm1) == 1);
    assert(roaring_bitmap_get_cardinality(bm2) == 1);
    roaring_bitmap_add(bm2, 4);
    roaring_bitmap_add(bm1, 5);
    assert(roaring_bitmap_get_cardinality(bm1) == 2);
    assert(roaring_bitmap_get_cardinality(bm2) == 2);
    roaring_bitmap_remove(bm1, 5);
    assert(roaring_bitmap_get_cardinality(bm1) == 1);
    roaring_bitmap_remove(bm1, 4);
    assert(roaring_bitmap_get_cardinality(bm1) == 1);
    assert(roaring_bitmap_get_cardinality(bm2) == 2);
    roaring_bitmap_remove(bm2, 4);
    assert(roaring_bitmap_get_cardinality(bm2) == 1);
    roaring_bitmap_free(bm1);
    roaring_bitmap_free(bm2);
}

void test_basic_add() {
    roaring_bitmap_t *bm = roaring_bitmap_create();
    roaring_bitmap_add(bm, 0);
    roaring_bitmap_remove(bm, 0);
    roaring_bitmap_free(bm);
}

void test_addremove() {
    roaring_bitmap_t *bm = roaring_bitmap_create();
    for (uint32_t value = 33057; value < 147849; value += 8) {
        roaring_bitmap_add(bm, value);
    }
    for (uint32_t value = 33057; value < 147849; value += 8) {
        roaring_bitmap_remove(bm, value);
    }
    assert_true(roaring_bitmap_is_empty(bm));
    roaring_bitmap_free(bm);
}

void test_addremoverun() {
    roaring_bitmap_t *bm = roaring_bitmap_create();
    for (uint32_t value = 33057; value < 147849; value += 8) {
        roaring_bitmap_add(bm, value);
    }
    roaring_bitmap_run_optimize(bm);
    for (uint32_t value = 33057; value < 147849; value += 8) {
        roaring_bitmap_remove(bm, value);
    }
    assert_true(roaring_bitmap_is_empty(bm));
    roaring_bitmap_free(bm);
}

void test_remove_from_copies_true() { can_remove_from_copies(true); }

void test_remove_from_copies_false() { can_remove_from_copies(false); }

bool check_bitmap_from_range(uint32_t min, uint32_t max, uint32_t step) {
    roaring_bitmap_t *result = roaring_bitmap_from_range(min, max, step);
    assert_non_null(result);
    roaring_bitmap_t *expected = roaring_bitmap_create();
    assert_non_null(expected);
    for (uint32_t value = min; value < max; value += step) {
        roaring_bitmap_add(expected, value);
    }
    bool is_equal = roaring_bitmap_equals(expected, result);
    if (!is_equal) {
        fprintf(stderr, "[ERROR] check_bitmap_from_range(%u, %u, %u)\n",
                (unsigned)min, (unsigned)max, (unsigned)step);
    }
    roaring_bitmap_free(expected);
    roaring_bitmap_free(result);
    return is_equal;
}

void test_silly_range() {
    check_bitmap_from_range(0, 1, 1);
    check_bitmap_from_range(0, 2, 1);
    roaring_bitmap_t *bm1 = roaring_bitmap_from_range(0, 1, 1);
    roaring_bitmap_t *bm2 = roaring_bitmap_from_range(0, 2, 1);
    assert_false(roaring_bitmap_equals(bm1, bm2));
    roaring_bitmap_free(bm1);
    roaring_bitmap_free(bm2);
}

void test_range_and_serialize() {
    roaring_bitmap_t *old_bm = roaring_bitmap_from_range(65520, 131057, 16);
    size_t size = roaring_bitmap_portable_size_in_bytes(old_bm);
    char *buff = malloc(size);
    size_t actualsize = roaring_bitmap_portable_serialize(old_bm, buff);
    assert_int_equal(actualsize, size);
    roaring_bitmap_t *new_bm = roaring_bitmap_portable_deserialize(buff);
    assert_true(roaring_bitmap_equals(old_bm, new_bm));
    roaring_bitmap_free(old_bm);
    roaring_bitmap_free(new_bm);
    free(buff);
}

void test_bitmap_from_range() {
    assert_true(roaring_bitmap_from_range(1, 10, 0) ==
                NULL);                                        // undefined range
    assert_true(roaring_bitmap_from_range(5, 1, 3) == NULL);  // empty range
    for (uint32_t i = 16; i < 1 << 18; i *= 2) {
        uint32_t min = i - 10;
        for (uint32_t delta = 16; delta < 1 << 18; delta *= 2) {
            uint32_t max = i + delta;
            for (uint32_t step = 1; step <= 64;
                 step *= 2) {  // check powers of 2
                assert_true(check_bitmap_from_range(min, max, step));
            }
            for (uint32_t step = 1; step <= 81;
                 step *= 3) {  // check powers of 3
                assert_true(check_bitmap_from_range(min, max, step));
            }
            for (uint32_t step = 1; step <= 125;
                 step *= 5) {  // check powers of 5
                assert_true(check_bitmap_from_range(min, max, step));
            }
        }
    }
}

void test_printf() {
    roaring_bitmap_t *r1 =
        roaring_bitmap_of(8, 1, 2, 3, 100, 1000, 10000, 1000000, 20000000);
    assert_non_null(r1);
    roaring_bitmap_printf(r1);
    roaring_bitmap_free(r1);
    printf("\n");
}

void test_printf_withbitmap() {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    assert_non_null(r1);
    roaring_bitmap_printf(r1);
    /* Add some values to the bitmap */
    for (int i = 0, top_val = 4097; i < top_val; i++)
        roaring_bitmap_add(r1, 2 * i);
    roaring_bitmap_printf(r1);
    roaring_bitmap_free(r1);
    printf("\n");
}

void test_printf_withrun() {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    assert_non_null(r1);
    roaring_bitmap_printf(r1);
    /* Add some values to the bitmap */
    for (int i = 100, top_val = 200; i < top_val; i++)
        roaring_bitmap_add(r1, i);
    roaring_bitmap_run_optimize(r1);
    roaring_bitmap_printf(r1);  // does it crash?
    roaring_bitmap_free(r1);
    printf("\n");
}

bool dummy_iterator(uint32_t value, void *param) {
    (void)value;

    uint32_t *num = (uint32_t *)param;
    (*num)++;
    return true;
}

void test_iterate() {
    roaring_bitmap_t *r1 =
        roaring_bitmap_of(8, 1, 2, 3, 100, 1000, 10000, 1000000, 20000000);
    assert_non_null(r1);

    uint32_t num = 0;
    /* Add some values to the bitmap */
    for (int i = 0, top_val = 384000; i < top_val; i++)
        roaring_bitmap_add(r1, 3 * i);

    roaring_iterate(r1, dummy_iterator, (void *)&num);

    assert_int_equal(roaring_bitmap_get_cardinality(r1), num);
    roaring_bitmap_free(r1);
}

void test_iterate_empty() {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    assert_non_null(r1);
    uint32_t num = 0;

    roaring_iterate(r1, dummy_iterator, (void *)&num);

    assert_int_equal(roaring_bitmap_get_cardinality(r1), 0);
    assert_int_equal(roaring_bitmap_get_cardinality(r1), num);
    roaring_bitmap_free(r1);
}

void test_iterate_withbitmap() {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    assert_non_null(r1);
    /* Add some values to the bitmap */
    for (int i = 0, top_val = 4097; i < top_val; i++)
        roaring_bitmap_add(r1, 2 * i);
    uint32_t num = 0;

    roaring_iterate(r1, dummy_iterator, (void *)&num);

    assert_int_equal(roaring_bitmap_get_cardinality(r1), num);
    roaring_bitmap_free(r1);
}

void test_iterate_withrun() {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    assert_non_null(r1);
    /* Add some values to the bitmap */
    for (int i = 100, top_val = 200; i < top_val; i++)
        roaring_bitmap_add(r1, i);
    roaring_bitmap_run_optimize(r1);
    uint32_t num = 0;
    roaring_iterate(r1, dummy_iterator, (void *)&num);

    assert_int_equal(roaring_bitmap_get_cardinality(r1), num);
    roaring_bitmap_free(r1);
}

void test_remove_withrun() {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    assert_non_null(r1);
    /* Add some values to the bitmap */
    for (int i = 100, top_val = 20000; i < top_val; i++)
        roaring_bitmap_add(r1, i);
    assert_int_equal(roaring_bitmap_get_cardinality(r1), 20000 - 100);
    roaring_bitmap_remove(r1, 1000);
    assert_int_equal(roaring_bitmap_get_cardinality(r1), 20000 - 100 - 1);
    roaring_bitmap_run_optimize(r1);
    assert_int_equal(roaring_bitmap_get_cardinality(r1), 20000 - 100 - 1);
    roaring_bitmap_remove(r1, 2000);
    assert_int_equal(roaring_bitmap_get_cardinality(r1), 20000 - 100 - 1 - 1);
    roaring_bitmap_free(r1);
}

void test_portable_serialize() {
    roaring_bitmap_t *r1 =
        roaring_bitmap_of(8, 1, 2, 3, 100, 1000, 10000, 1000000, 20000000);
    assert_non_null(r1);

    uint32_t serialize_len;
    roaring_bitmap_t *r2;

    for (int i = 0, top_val = 384000; i < top_val; i++)
        roaring_bitmap_add(r1, 3 * i);

    uint32_t expectedsize = roaring_bitmap_portable_size_in_bytes(r1);
    char *serialized = malloc(expectedsize);
    serialize_len = roaring_bitmap_portable_serialize(r1, serialized);
    assert_int_equal(serialize_len, expectedsize);
    assert_int_equal(serialize_len, expectedsize);
    r2 = roaring_bitmap_portable_deserialize(serialized);
    assert_non_null(r2);

    uint64_t card1 = roaring_bitmap_get_cardinality(r1);
    uint32_t *arr1 = (uint32_t *)malloc(card1 * sizeof(uint32_t));
    roaring_bitmap_to_uint32_array(r1, arr1);

    uint64_t card2 = roaring_bitmap_get_cardinality(r2);
    uint32_t *arr2 = (uint32_t *)malloc(card2 * sizeof(uint32_t));
    roaring_bitmap_to_uint32_array(r2, arr2);

    assert_true(array_equals(arr1, card1, arr2, card2));
    assert_true(roaring_bitmap_equals(r1, r2));
    free(arr1);
    free(arr2);
    free(serialized);
    roaring_bitmap_free(r1);
    roaring_bitmap_free(r2);

    r1 = roaring_bitmap_of(6, 2946000, 2997491, 10478289, 10490227, 10502444,
                           19866827);
    expectedsize = roaring_bitmap_portable_size_in_bytes(r1);
    serialized = malloc(expectedsize);
    serialize_len = roaring_bitmap_portable_serialize(r1, serialized);
    assert_int_equal(serialize_len, expectedsize);
    assert_int_equal(serialize_len, expectedsize);

    r2 = roaring_bitmap_portable_deserialize(serialized);
    assert_non_null(r2);

    card1 = roaring_bitmap_get_cardinality(r1);
    arr1 = (uint32_t *)malloc(card1 * sizeof(uint32_t));
    roaring_bitmap_to_uint32_array(r1, arr1);

    card2 = roaring_bitmap_get_cardinality(r2);
    arr2 = (uint32_t *)malloc(card2 * sizeof(uint32_t));
    roaring_bitmap_to_uint32_array(r2, arr2);

    assert_true(array_equals(arr1, card1, arr2, card2));
    assert_true(roaring_bitmap_equals(r1, r2));
    free(arr1);
    free(arr2);
    free(serialized);
    roaring_bitmap_free(r1);
    roaring_bitmap_free(r2);

    r1 = roaring_bitmap_create();
    assert_non_null(r1);

    for (uint32_t k = 100; k < 100000; ++k) {
        roaring_bitmap_add(r1, k);
    }

    roaring_bitmap_run_optimize(r1);
    expectedsize = roaring_bitmap_portable_size_in_bytes(r1);
    serialized = malloc(expectedsize);
    serialize_len = roaring_bitmap_portable_serialize(r1, serialized);
    assert_int_equal(serialize_len, expectedsize);

    r2 = roaring_bitmap_portable_deserialize(serialized);
    assert_non_null(r2);

    card1 = roaring_bitmap_get_cardinality(r1);
    arr1 = (uint32_t *)malloc(card1 * sizeof(uint32_t));
    roaring_bitmap_to_uint32_array(r1, arr1);

    card2 = roaring_bitmap_get_cardinality(r2);
    arr2 = (uint32_t *)malloc(card2 * sizeof(uint32_t));
    roaring_bitmap_to_uint32_array(r2, arr2);

    assert(array_equals(arr1, card1, arr2, card2));
    assert(roaring_bitmap_equals(r1, r2));
    free(arr1);
    free(arr2);
    free(serialized);
    roaring_bitmap_free(r1);
    roaring_bitmap_free(r2);
}

void test_serialize() {
    roaring_bitmap_t *r1 =
        roaring_bitmap_of(8, 1, 2, 3, 100, 1000, 10000, 1000000, 20000000);
    assert_non_null(r1);

    uint32_t serialize_len;
    char *serialized;
    roaring_bitmap_t *r2;

    /* Add some values to the bitmap */
    for (int i = 0, top_val = 384000; i < top_val; i++)
        roaring_bitmap_add(r1, 3 * i);
    serialized = malloc(roaring_bitmap_size_in_bytes(r1));
    serialize_len = roaring_bitmap_serialize(r1, serialized);
    assert_int_equal(serialize_len, roaring_bitmap_size_in_bytes(r1));
    r2 = roaring_bitmap_deserialize(serialized);
    assert_non_null(r2);

    uint64_t card1 = roaring_bitmap_get_cardinality(r1);
    uint32_t *arr1 = (uint32_t *)malloc(card1 * sizeof(uint32_t));
    roaring_bitmap_to_uint32_array(r1, arr1);

    uint64_t card2 = roaring_bitmap_get_cardinality(r2);
    uint32_t *arr2 = (uint32_t *)malloc(card2 * sizeof(uint32_t));
    roaring_bitmap_to_uint32_array(r2, arr2);

    assert_true(array_equals(arr1, card1, arr2, card2));
    assert_true(roaring_bitmap_equals(r1, r2));
    free(arr1);
    free(arr2);
    free(serialized);
    roaring_bitmap_free(r1);
    roaring_bitmap_free(r2);

    run_container_t *run = run_container_create_given_capacity(1024);
    assert_non_null(run);
    for (int i = 0; i < 768; i++) run_container_add(run, 3 * i);

    serialize_len = run_container_serialization_len(run);
    char rbuf[serialize_len];
    assert_int_equal((int32_t)serialize_len,
                     run_container_serialize(run, rbuf));
    run_container_t *run1 = run_container_deserialize(rbuf, serialize_len);

    run_container_free(run);
    run_container_free(run1);

    r1 = roaring_bitmap_of(6, 2946000, 2997491, 10478289, 10490227, 10502444,
                           19866827);

    serialized = malloc(roaring_bitmap_size_in_bytes(r1));
    serialize_len = roaring_bitmap_serialize(r1, serialized);
    assert_int_equal(serialize_len, roaring_bitmap_size_in_bytes(r1));
    r2 = roaring_bitmap_deserialize(serialized);
    assert_non_null(r2);

    card1 = roaring_bitmap_get_cardinality(r1);
    arr1 = (uint32_t *)malloc(card1 * sizeof(uint32_t));
    assert_non_null(arr1);
    roaring_bitmap_to_uint32_array(r1, arr1);

    card2 = roaring_bitmap_get_cardinality(r2);
    arr2 = (uint32_t *)malloc(card2 * sizeof(uint32_t));
    assert_non_null(arr2);
    roaring_bitmap_to_uint32_array(r2, arr2);

    assert_true(array_equals(arr1, card1, arr2, card2));
    assert_true(roaring_bitmap_equals(r1, r2));
    free(arr1);
    free(arr2);
    free(serialized);
    roaring_bitmap_free(r1);
    roaring_bitmap_free(r2);

    r1 = roaring_bitmap_create();
    for (uint32_t k = 100; k < 100000; ++k) {
        roaring_bitmap_add(r1, k);
    }
    roaring_bitmap_run_optimize(r1);
    serialized = malloc(roaring_bitmap_size_in_bytes(r1));
    serialize_len = roaring_bitmap_serialize(r1, serialized);
    assert_int_equal(serialize_len, roaring_bitmap_size_in_bytes(r1));
    r2 = roaring_bitmap_deserialize(serialized);

    card1 = roaring_bitmap_get_cardinality(r1);
    arr1 = (uint32_t *)malloc(card1 * sizeof(uint32_t));
    assert_non_null(arr1);
    roaring_bitmap_to_uint32_array(r1, arr1);

    card2 = roaring_bitmap_get_cardinality(r2);
    arr2 = (uint32_t *)malloc(card2 * sizeof(uint32_t));
    assert_non_null(arr2);
    roaring_bitmap_to_uint32_array(r2, arr2);

    assert_true(array_equals(arr1, card1, arr2, card2));
    assert_true(roaring_bitmap_equals(r1, r2));
    free(arr1);
    free(arr2);
    free(serialized);
    roaring_bitmap_free(r1);
    roaring_bitmap_free(r2);

    /* ******* */
    roaring_bitmap_t *old_bm = roaring_bitmap_create();
    for (unsigned i = 0; i < 102; i++) roaring_bitmap_add(old_bm, i);
    char *buff = malloc(roaring_bitmap_size_in_bytes(old_bm));
    uint32_t size = roaring_bitmap_serialize(old_bm, buff);
    assert_int_equal(size, roaring_bitmap_size_in_bytes(old_bm));
    roaring_bitmap_t *new_bm = roaring_bitmap_deserialize(buff);
    free(buff);
    assert_true((unsigned int)roaring_bitmap_get_cardinality(old_bm) ==
                (unsigned int)roaring_bitmap_get_cardinality(new_bm));
    assert_true(roaring_bitmap_equals(old_bm, new_bm));
    roaring_bitmap_free(old_bm);
    roaring_bitmap_free(new_bm);
}

void test_add() {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    assert_non_null(r1);

    for (uint32_t i = 0; i < 10000; ++i) {
        assert_int_equal(roaring_bitmap_get_cardinality(r1), i);
        roaring_bitmap_add(r1, 200 * i);
        assert_int_equal(roaring_bitmap_get_cardinality(r1), i + 1);
    }

    roaring_bitmap_free(r1);
}

void test_contains() {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    assert_non_null(r1);

    for (uint32_t i = 0; i < 10000; ++i) {
        assert_int_equal(roaring_bitmap_get_cardinality(r1), i);
        roaring_bitmap_add(r1, 200 * i);
        assert_int_equal(roaring_bitmap_get_cardinality(r1), i + 1);
    }

    for (uint32_t i = 0; i < 200 * 10000; ++i) {
        assert_int_equal(roaring_bitmap_contains(r1, i), (i % 200 == 0));
    }

    roaring_bitmap_free(r1);
}

void test_intersection_array_x_array() {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    assert_non_null(r1);
    roaring_bitmap_t *r2 = roaring_bitmap_create();
    assert_non_null(r2);

    for (uint32_t i = 0; i < 100; ++i) {
        roaring_bitmap_add(r1, 2 * i);
        roaring_bitmap_add(r2, 3 * i);
        roaring_bitmap_add(r1, 5 * 65536 + 2 * i);
        roaring_bitmap_add(r2, 5 * 65536 + 3 * i);

        assert_int_equal(roaring_bitmap_get_cardinality(r2), 2 * (i + 1));
        assert_int_equal(roaring_bitmap_get_cardinality(r1), 2 * (i + 1));
    }

    roaring_bitmap_t *r1_and_r2 = roaring_bitmap_and(r1, r2);
    assert_non_null(r1_and_r2);
    assert_int_equal(roaring_bitmap_get_cardinality(r1_and_r2), 2 * 34);

    roaring_bitmap_free(r1_and_r2);
    roaring_bitmap_free(r2);
    roaring_bitmap_free(r1);
}

void test_intersection_array_x_array_inplace() {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    assert(r1);
    roaring_bitmap_t *r2 = roaring_bitmap_create();
    assert(r2);

    for (uint32_t i = 0; i < 100; ++i) {
        roaring_bitmap_add(r1, 2 * i);
        roaring_bitmap_add(r2, 3 * i);
        roaring_bitmap_add(r1, 5 * 65536 + 2 * i);
        roaring_bitmap_add(r2, 5 * 65536 + 3 * i);

        assert_int_equal(roaring_bitmap_get_cardinality(r2), 2 * (i + 1));
        assert_int_equal(roaring_bitmap_get_cardinality(r1), 2 * (i + 1));
    }

    roaring_bitmap_and_inplace(r1, r2);
    assert_int_equal(roaring_bitmap_get_cardinality(r1), 2 * 34);

    roaring_bitmap_free(r2);
    roaring_bitmap_free(r1);
}

void test_intersection_bitset_x_bitset() {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    assert(r1);
    roaring_bitmap_t *r2 = roaring_bitmap_create();
    assert(r2);

    for (uint32_t i = 0; i < 20000; ++i) {
        roaring_bitmap_add(r1, 2 * i);
        roaring_bitmap_add(r2, 3 * i);
        roaring_bitmap_add(r2, 3 * i + 1);
        roaring_bitmap_add(r1, 5 * 65536 + 2 * i);
        roaring_bitmap_add(r2, 5 * 65536 + 3 * i);
        roaring_bitmap_add(r2, 5 * 65536 + 3 * i + 1);

        assert_int_equal(roaring_bitmap_get_cardinality(r1), 2 * (i + 1));
        assert_int_equal(roaring_bitmap_get_cardinality(r2), 4 * (i + 1));
    }

    roaring_bitmap_t *r1_and_r2 = roaring_bitmap_and(r1, r2);
    assert_non_null(r1_and_r2);

    // NOT analytically determined but seems reasonable
    assert_int_equal(roaring_bitmap_get_cardinality(r1_and_r2), 26666);

    roaring_bitmap_free(r1_and_r2);
    roaring_bitmap_free(r2);
    roaring_bitmap_free(r1);
}

void test_intersection_bitset_x_bitset_inplace() {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    assert(r1);
    roaring_bitmap_t *r2 = roaring_bitmap_create();
    assert(r2);

    for (uint32_t i = 0; i < 20000; ++i) {
        roaring_bitmap_add(r1, 2 * i);
        roaring_bitmap_add(r2, 3 * i);
        roaring_bitmap_add(r2, 3 * i + 1);
        roaring_bitmap_add(r1, 5 * 65536 + 2 * i);
        roaring_bitmap_add(r2, 5 * 65536 + 3 * i);
        roaring_bitmap_add(r2, 5 * 65536 + 3 * i + 1);

        assert_int_equal(roaring_bitmap_get_cardinality(r1), 2 * (i + 1));
        assert_int_equal(roaring_bitmap_get_cardinality(r2), 4 * (i + 1));
    }

    roaring_bitmap_and_inplace(r1, r2);

    assert_int_equal(roaring_bitmap_get_cardinality(r1), 26666);
    roaring_bitmap_free(r2);
    roaring_bitmap_free(r1);
}

void test_union(bool copy_on_write) {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    r1->copy_on_write = copy_on_write;
    assert(r1);
    roaring_bitmap_t *r2 = roaring_bitmap_create();
    r2->copy_on_write = copy_on_write;
    assert(r2);

    for (uint32_t i = 0; i < 100; ++i) {
        roaring_bitmap_add(r1, 2 * i);
        roaring_bitmap_add(r2, 3 * i);
        assert_int_equal(roaring_bitmap_get_cardinality(r2), i + 1);
        assert_int_equal(roaring_bitmap_get_cardinality(r1), i + 1);
    }

    roaring_bitmap_t *r1_or_r2 = roaring_bitmap_or(r1, r2);
    r1_or_r2->copy_on_write = copy_on_write;
    assert_int_equal(roaring_bitmap_get_cardinality(r1_or_r2), 166);

    roaring_bitmap_free(r1_or_r2);
    roaring_bitmap_free(r2);
    roaring_bitmap_free(r1);
}

void test_union_true() { test_union(true); }

void test_union_false() { test_union(false); }

// density factor changes as one gets further into bitmap
static roaring_bitmap_t *gen_bitmap(double start_density,
                                    double density_gradient, int run_length,
                                    int blank_range_start, int blank_range_end,
                                    int universe_size) {
    srand(2345);
    roaring_bitmap_t *ans = roaring_bitmap_create();
    double d = start_density;

    for (int i = 0; i < universe_size; i += run_length) {
        d = start_density + i * density_gradient;
        double r = rand() / (double)RAND_MAX;
        if (r < d && !(i >= blank_range_start && i < blank_range_end))
            for (int j = 0; j < run_length; ++j) roaring_bitmap_add(ans, i + j);
    }
    roaring_bitmap_run_optimize(ans);
    return ans;
}

static roaring_bitmap_t *synthesized_xor(roaring_bitmap_t *r1,
                                         roaring_bitmap_t *r2) {
    unsigned universe_size = 0;
    roaring_statistics_t stats;
    roaring_bitmap_statistics(r1, &stats);
    universe_size = stats.max_value;
    roaring_bitmap_statistics(r2, &stats);
    if (stats.max_value > universe_size) universe_size = stats.max_value;

    roaring_bitmap_t *r1_or_r2 = roaring_bitmap_or(r1, r2);
    roaring_bitmap_t *r1_and_r2 = roaring_bitmap_and(r1, r2);
    roaring_bitmap_t *r1_nand_r2 =
        roaring_bitmap_flip(r1_and_r2, 0U, universe_size + 1U);
    roaring_bitmap_t *r1_xor_r2 = roaring_bitmap_and(r1_or_r2, r1_nand_r2);
    roaring_bitmap_free(r1_or_r2);
    roaring_bitmap_free(r1_and_r2);
    roaring_bitmap_free(r1_nand_r2);
    return r1_xor_r2;
}

static roaring_bitmap_t *synthesized_andnot(roaring_bitmap_t *r1,
                                            roaring_bitmap_t *r2) {
    unsigned universe_size = 0;
    roaring_statistics_t stats;
    roaring_bitmap_statistics(r1, &stats);
    universe_size = stats.max_value;
    roaring_bitmap_statistics(r2, &stats);
    if (stats.max_value > universe_size) universe_size = stats.max_value;

    roaring_bitmap_t *not_r2 = roaring_bitmap_flip(r2, 0U, universe_size + 1U);
    roaring_bitmap_t *r1_andnot_r2 = roaring_bitmap_and(r1, not_r2);
    roaring_bitmap_free(not_r2);
    return r1_andnot_r2;
}

// only for valid for universe < 10M, could adapt with roaring_bitmap_statistics
static void show_difference(roaring_bitmap_t *result,
                            roaring_bitmap_t *hopedfor) {
    int out_ctr = 0;
    for (int i = 0; i < 10000000; ++i) {
        if (roaring_bitmap_contains(result, i) &&
            !roaring_bitmap_contains(hopedfor, i)) {
            printf("result incorrectly has %d\n", i);
            ++out_ctr;
        }
        if (!roaring_bitmap_contains(result, i) &&
            roaring_bitmap_contains(hopedfor, i)) {
            printf("result incorrectly omits %d\n", i);
            ++out_ctr;
        }
        if (out_ctr > 20) {
            printf("20 errors seen, stopping comparison\n");
            break;
        }
    }
}

void test_xor(bool copy_on_write) {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    r1->copy_on_write = copy_on_write;
    roaring_bitmap_t *r2 = roaring_bitmap_create();
    r2->copy_on_write = copy_on_write;

    for (uint32_t i = 0; i < 300; ++i) {
        if (i % 2 == 0) roaring_bitmap_add(r1, i);
        if (i % 3 == 0) roaring_bitmap_add(r2, i);
    }

    roaring_bitmap_t *r1_xor_r2 = roaring_bitmap_xor(r1, r2);
    r1_xor_r2->copy_on_write = copy_on_write;

    int ansctr = 0;
    for (int i = 0; i < 300; ++i) {
        if (((i % 2 == 0) || (i % 3 == 0)) && (i % 6 != 0)) {
            ansctr++;
            if (!roaring_bitmap_contains(r1_xor_r2, i))
                printf("missing %d\n", i);
        } else if (roaring_bitmap_contains(r1_xor_r2, i))
            printf("surplus %d\n", i);
    }

    assert_int_equal(roaring_bitmap_get_cardinality(r1_xor_r2), ansctr);

    roaring_bitmap_free(r1_xor_r2);
    roaring_bitmap_free(r2);
    roaring_bitmap_free(r1);

    // some tougher tests on synthetic data

    roaring_bitmap_t *r[] = {
        // ascending density, last containers might be runs
        gen_bitmap(0.0, 1e-6, 1, 0, 0, 1000000),
        // descending density, first containers might be runs
        gen_bitmap(1.0, -1e-6, 1, 0, 0, 1000000),
        // uniformly rather sparse
        gen_bitmap(1e-5, 0.0, 1, 0, 0, 2000000),
        // uniformly rather sparse with runs
        gen_bitmap(1e-5, 0.0, 3, 0, 0, 2000000),
        // uniformly rather dense
        gen_bitmap(1e-1, 0.0, 1, 0, 0, 2000000),
        // ascending density but never too dense
        gen_bitmap(0.001, 1e-7, 1, 0, 0, 1000000),
        // ascending density but very sparse
        gen_bitmap(0.0, 1e-10, 1, 0, 0, 1000000),
        // descending with a gap
        gen_bitmap(0.5, -1e-6, 1, 600000, 800000, 1000000),
        //  gap elsewhere
        gen_bitmap(1, -1e-6, 1, 300000, 500000, 1000000),
        0  // sentinel
    };

    for (int i = 0; r[i]; ++i) {
        for (int j = i; r[j]; ++j) {
            roaring_bitmap_t *expected = synthesized_xor(r[i], r[j]);
            roaring_bitmap_t *result = roaring_bitmap_xor(r[i], r[j]);

            bool is_equal = roaring_bitmap_equals(expected, result);

            assert_true(is_equal);
            roaring_bitmap_free(expected);
            roaring_bitmap_free(result);
        }
    }
    for (int i = 0; r[i]; ++i) roaring_bitmap_free(r[i]);
}

void test_xor_true() { test_xor(true); }

void test_xor_false() { test_xor(false); }

void test_xor_inplace(bool copy_on_write) {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    r1->copy_on_write = copy_on_write;
    roaring_bitmap_t *r2 = roaring_bitmap_create();
    r2->copy_on_write = copy_on_write;

    for (uint32_t i = 0; i < 300; ++i) {
        if (i % 2 == 0) roaring_bitmap_add(r1, i);
        if (i % 3 == 0) roaring_bitmap_add(r2, i);
    }
    roaring_bitmap_xor_inplace(r1, r2);
    assert_int_equal(roaring_bitmap_get_cardinality(r1), 166 - 16);

    roaring_bitmap_free(r2);
    roaring_bitmap_free(r1);

    // some tougher tests on synthetic data

    roaring_bitmap_t *r[] = {
        // ascending density, last containers might be runs
        gen_bitmap(0.0, 1e-6, 1, 0, 0, 1000000),
        // descending density, first containers might be runs
        gen_bitmap(1.0, -1e-6, 1, 0, 0, 1000000),
        // uniformly rather sparse
        gen_bitmap(1e-5, 0.0, 1, 0, 0, 2000000),
        // uniformly rather sparse with runs
        gen_bitmap(1e-5, 0.0, 3, 0, 0, 2000000),
        // uniformly rather dense
        gen_bitmap(1e-1, 0.0, 1, 0, 0, 2000000),
        // ascending density but never too dense
        gen_bitmap(0.001, 1e-7, 1, 0, 0, 1000000),
        // ascending density but very sparse
        gen_bitmap(0.0, 1e-10, 1, 0, 0, 1000000),
        // descending with a gap
        gen_bitmap(0.5, -1e-6, 1, 600000, 800000, 1000000),
        //  gap elsewhere
        gen_bitmap(1, -1e-6, 1, 300000, 500000, 1000000),
        0  // sentinel
    };

    for (int i = 0; r[i]; ++i) {
        for (int j = i + 1; r[j]; ++j) {
            roaring_bitmap_t *expected = synthesized_xor(r[i], r[j]);
            roaring_bitmap_t *copy = roaring_bitmap_copy(r[i]);
            copy->copy_on_write = copy_on_write;

            roaring_bitmap_xor_inplace(copy, r[j]);

            bool is_equal = roaring_bitmap_equals(expected, copy);
            if (!is_equal) {
                printf("problem with i=%d j=%d\n", i, j);
                printf("copy's cardinality  is %d and expected's is %d\n",
                       (int)roaring_bitmap_get_cardinality(copy),
                       (int)roaring_bitmap_get_cardinality(expected));
                show_difference(copy, expected);
            }

            assert_true(is_equal);
            roaring_bitmap_free(expected);
            roaring_bitmap_free(copy);
        }
    }
    for (int i = 0; r[i]; ++i) roaring_bitmap_free(r[i]);
}

void test_xor_inplace_true() { test_xor_inplace(true); }

void test_xor_inplace_false() { test_xor_inplace(false); }

void test_xor_lazy(bool copy_on_write) {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    r1->copy_on_write = copy_on_write;
    roaring_bitmap_t *r2 = roaring_bitmap_create();
    r2->copy_on_write = copy_on_write;

    for (uint32_t i = 0; i < 300; ++i) {
        if (i % 2 == 0) roaring_bitmap_add(r1, i);
        if (i % 3 == 0) roaring_bitmap_add(r2, i);
    }

    roaring_bitmap_t *r1_xor_r2 = roaring_bitmap_lazy_xor(r1, r2);
    roaring_bitmap_repair_after_lazy(r1_xor_r2);

    r1_xor_r2->copy_on_write = copy_on_write;

    int ansctr = 0;
    for (int i = 0; i < 300; ++i) {
        if (((i % 2 == 0) || (i % 3 == 0)) && (i % 6 != 0)) {
            ansctr++;
            if (!roaring_bitmap_contains(r1_xor_r2, i))
                printf("missing %d\n", i);
        } else if (roaring_bitmap_contains(r1_xor_r2, i))
            printf("surplus %d\n", i);
    }

    assert_int_equal(roaring_bitmap_get_cardinality(r1_xor_r2), ansctr);

    roaring_bitmap_free(r1_xor_r2);
    roaring_bitmap_free(r2);
    roaring_bitmap_free(r1);

    // some tougher tests on synthetic data
    roaring_bitmap_t *r[] = {
        // ascending density, last containers might be runs
        gen_bitmap(0.0, 1e-6, 1, 0, 0, 1000000),
        // descending density, first containers might be runs
        gen_bitmap(1.0, -1e-6, 1, 0, 0, 1000000),
        // uniformly rather sparse
        gen_bitmap(1e-5, 0.0, 1, 0, 0, 2000000),
        // uniformly rather sparse with runs
        gen_bitmap(1e-5, 0.0, 3, 0, 0, 2000000),
        // uniformly rather dense
        gen_bitmap(1e-1, 0.0, 1, 0, 0, 2000000),
        // ascending density but never too dense
        gen_bitmap(0.001, 1e-7, 1, 0, 0, 1000000),
        // ascending density but very sparse
        gen_bitmap(0.0, 1e-10, 1, 0, 0, 1000000),
        // descending with a gap
        gen_bitmap(0.5, -1e-6, 1, 600000, 800000, 1000000),
        //  gap elsewhere
        gen_bitmap(1, -1e-6, 1, 300000, 500000, 1000000),
        0  // sentinel
    };

    for (int i = 0; r[i]; ++i) {
        for (int j = i; r[j]; ++j) {
            roaring_bitmap_t *expected = synthesized_xor(r[i], r[j]);

            roaring_bitmap_t *result = roaring_bitmap_lazy_xor(r[i], r[j]);
            roaring_bitmap_repair_after_lazy(result);

            bool is_equal = roaring_bitmap_equals(expected, result);
            if (!is_equal) {
                printf("problem with i=%d j=%d\n", i, j);
                printf("result's cardinality  is %d and expected's is %d\n",
                       (int)roaring_bitmap_get_cardinality(result),
                       (int)roaring_bitmap_get_cardinality(expected));
                show_difference(result, expected);
            }

            assert_true(is_equal);
            roaring_bitmap_free(expected);
            roaring_bitmap_free(result);
        }
    }
    for (int i = 0; r[i]; ++i) roaring_bitmap_free(r[i]);
}

void test_xor_lazy_true() { test_xor_lazy(true); }

void test_xor_lazy_false() { test_xor_lazy(false); }

void test_xor_lazy_inplace(bool copy_on_write) {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    r1->copy_on_write = copy_on_write;
    roaring_bitmap_t *r2 = roaring_bitmap_create();
    r2->copy_on_write = copy_on_write;

    for (uint32_t i = 0; i < 300; ++i) {
        if (i % 2 == 0) roaring_bitmap_add(r1, i);
        if (i % 3 == 0) roaring_bitmap_add(r2, i);
    }

    roaring_bitmap_t *r1_xor_r2 = roaring_bitmap_copy(r1);
    r1_xor_r2->copy_on_write = copy_on_write;

    roaring_bitmap_lazy_xor_inplace(r1_xor_r2, r2);
    roaring_bitmap_repair_after_lazy(r1_xor_r2);

    int ansctr = 0;
    for (int i = 0; i < 300; ++i) {
        if (((i % 2 == 0) || (i % 3 == 0)) && (i % 6 != 0)) {
            ansctr++;
            if (!roaring_bitmap_contains(r1_xor_r2, i))
                printf("missing %d\n", i);
        } else if (roaring_bitmap_contains(r1_xor_r2, i))
            printf("surplus %d\n", i);
    }

    assert_int_equal(roaring_bitmap_get_cardinality(r1_xor_r2), ansctr);

    roaring_bitmap_free(r1_xor_r2);
    roaring_bitmap_free(r2);
    roaring_bitmap_free(r1);

    // some tougher tests on synthetic data
    roaring_bitmap_t *r[] = {
        // ascending density, last containers might be runs
        gen_bitmap(0.0, 1e-6, 1, 0, 0, 1000000),
        // descending density, first containers might be runs
        gen_bitmap(1.0, -1e-6, 1, 0, 0, 1000000),
        // uniformly rather sparse
        gen_bitmap(1e-5, 0.0, 1, 0, 0, 2000000),
        // uniformly rather sparse with runs
        gen_bitmap(1e-5, 0.0, 3, 0, 0, 2000000),
        // uniformly rather dense
        gen_bitmap(1e-1, 0.0, 1, 0, 0, 2000000),
        // ascending density but never too dense
        gen_bitmap(0.001, 1e-7, 1, 0, 0, 1000000),
        // ascending density but very sparse
        gen_bitmap(0.0, 1e-10, 1, 0, 0, 1000000),
        // descending with a gap
        gen_bitmap(0.5, -1e-6, 1, 600000, 800000, 1000000),
        //  gap elsewhere
        gen_bitmap(1, -1e-6, 1, 300000, 500000, 1000000),
        0  // sentinel
    };

    for (int i = 0; r[i]; ++i) {
        for (int j = i; r[j]; ++j) {
            roaring_bitmap_t *expected = synthesized_xor(r[i], r[j]);

            roaring_bitmap_t *result = roaring_bitmap_copy(r[i]);
            roaring_bitmap_lazy_xor_inplace(result, r[j]);
            roaring_bitmap_repair_after_lazy(result);

            bool is_equal = roaring_bitmap_equals(expected, result);
            if (!is_equal) {
                printf("problem with i=%d j=%d\n", i, j);
                printf("result's cardinality  is %d and expected's is %d\n",
                       (int)roaring_bitmap_get_cardinality(result),
                       (int)roaring_bitmap_get_cardinality(expected));
                show_difference(result, expected);
            }

            assert_true(is_equal);
            roaring_bitmap_free(expected);
            roaring_bitmap_free(result);
        }
    }
    for (int i = 0; r[i]; ++i) roaring_bitmap_free(r[i]);
}

void test_xor_lazy_inplace_true() { test_xor_lazy_inplace(true); }

void test_xor_lazy_inplace_false() { test_xor_lazy_inplace(false); }

static roaring_bitmap_t *roaring_from_sentinel_array(int *data,
                                                     bool copy_on_write) {
    roaring_bitmap_t *ans = roaring_bitmap_create();
    ans->copy_on_write = copy_on_write;

    for (; *data != -1; ++data) {
        roaring_bitmap_add(ans, *data);
    }
    return ans;
}

void test_andnot(bool copy_on_write) {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    r1->copy_on_write = copy_on_write;
    roaring_bitmap_t *r2 = roaring_bitmap_create();
    r2->copy_on_write = copy_on_write;

    int data1[] = {1,
                   2,
                   65536 * 2 + 1,
                   65536 * 2 + 2,
                   65536 * 3 + 1,
                   65536 * 3 + 2,
                   65536 * 10 + 1,
                   65536 * 10 + 2,
                   65536 * 16 + 1,
                   65536 * 16 + 2,
                   65536 * 20 + 1,
                   65536 * 21 + 1,
                   -1};
    roaring_bitmap_t *rb1 = roaring_from_sentinel_array(data1, copy_on_write);
    int data2[] = {2,
                   3,
                   65536 * 10 + 2,
                   65536 * 10 + 3,
                   65536 * 12 + 2,
                   65536 * 12 + 3,
                   65536 * 14 + 2,
                   65536 * 14 + 3,
                   65536 * 16 + 2,
                   65536 * 16 + 3,
                   -1};
    roaring_bitmap_t *rb2 = roaring_from_sentinel_array(data2, copy_on_write);

    int data3[] = {2,
                   3,
                   65536 * 10 + 1,
                   65536 * 10 + 2,
                   65536 * 12 + 2,
                   65536 * 12 + 3,
                   65536 * 14 + 2,
                   65536 * 14 + 3,
                   65536 * 16 + 2,
                   65536 * 16 + 3,
                   -1};
    roaring_bitmap_t *rb3 = roaring_from_sentinel_array(data3, copy_on_write);
    int d1_minus_d2[] = {1,
                         65536 * 2 + 1,
                         65536 * 2 + 2,
                         65536 * 3 + 1,
                         65536 * 3 + 2,
                         65536 * 10 + 1,
                         65536 * 16 + 1,
                         65536 * 20 + 1,
                         65536 * 21 + 1,
                         -1};
    roaring_bitmap_t *rb1_minus_rb2 =
        roaring_from_sentinel_array(d1_minus_d2, copy_on_write);

    int d1_minus_d3[] = {1, 65536 * 2 + 1, 65536 * 2 + 2, 65536 * 3 + 1,
                         65536 * 3 + 2,
                         // 65536*10+1,
                         65536 * 16 + 1, 65536 * 20 + 1, 65536 * 21 + 1, -1};
    roaring_bitmap_t *rb1_minus_rb3 =
        roaring_from_sentinel_array(d1_minus_d3, copy_on_write);

    int d2_minus_d1[] = {3,
                         65536 * 10 + 3,
                         65536 * 12 + 2,
                         65536 * 12 + 3,
                         65536 * 14 + 2,
                         65536 * 14 + 3,
                         65536 * 16 + 3,
                         -1};

    roaring_bitmap_t *rb2_minus_rb1 =
        roaring_from_sentinel_array(d2_minus_d1, copy_on_write);

    int d3_minus_d1[] = {3,
                         // 65536*10+3,
                         65536 * 12 + 2, 65536 * 12 + 3, 65536 * 14 + 2,
                         65536 * 14 + 3, 65536 * 16 + 3, -1};
    roaring_bitmap_t *rb3_minus_rb1 =
        roaring_from_sentinel_array(d3_minus_d1, copy_on_write);

    int d3_minus_d2[] = {65536 * 10 + 1, -1};
    roaring_bitmap_t *rb3_minus_rb2 =
        roaring_from_sentinel_array(d3_minus_d2, copy_on_write);

    roaring_bitmap_t *temp = roaring_bitmap_andnot(rb1, rb2);
    assert_true(roaring_bitmap_equals(rb1_minus_rb2, temp));
    roaring_bitmap_free(temp);

    temp = roaring_bitmap_andnot(rb1, rb3);
    assert_true(roaring_bitmap_equals(rb1_minus_rb3, temp));
    roaring_bitmap_free(temp);

    temp = roaring_bitmap_andnot(rb2, rb1);
    assert_true(roaring_bitmap_equals(rb2_minus_rb1, temp));
    roaring_bitmap_free(temp);

    temp = roaring_bitmap_andnot(rb3, rb1);
    assert_true(roaring_bitmap_equals(rb3_minus_rb1, temp));
    roaring_bitmap_free(temp);

    temp = roaring_bitmap_andnot(rb3, rb2);
    assert_true(roaring_bitmap_equals(rb3_minus_rb2, temp));
    roaring_bitmap_free(temp);

    roaring_bitmap_t *large_run_bitmap =
        roaring_bitmap_from_range(2, 11 * 65536 + 27, 1);
    temp = roaring_bitmap_andnot(rb1, large_run_bitmap);

    int d1_minus_largerun[] = {
        1, 65536 * 16 + 1, 65536 * 16 + 2, 65536 * 20 + 1, 65536 * 21 + 1, -1};
    roaring_bitmap_t *rb1_minus_largerun =
        roaring_from_sentinel_array(d1_minus_largerun, copy_on_write);
    assert_true(roaring_bitmap_equals(rb1_minus_largerun, temp));
    roaring_bitmap_free(temp);

    roaring_bitmap_free(rb1);
    roaring_bitmap_free(rb2);
    roaring_bitmap_free(rb3);
    roaring_bitmap_free(rb1_minus_rb2);
    roaring_bitmap_free(rb1_minus_rb3);
    roaring_bitmap_free(rb2_minus_rb1);
    roaring_bitmap_free(rb3_minus_rb1);
    roaring_bitmap_free(rb3_minus_rb2);
    roaring_bitmap_free(rb1_minus_largerun);
    roaring_bitmap_free(large_run_bitmap);

    for (uint32_t i = 0; i < 300; ++i) {
        if (i % 2 == 0) roaring_bitmap_add(r1, i);
        if (i % 3 == 0) roaring_bitmap_add(r2, i);
    }

    roaring_bitmap_t *r1_andnot_r2 = roaring_bitmap_andnot(r1, r2);
    r1_andnot_r2->copy_on_write = copy_on_write;

    int ansctr = 0;
    for (int i = 0; i < 300; ++i) {
        if ((i % 2 == 0) && (i % 3 != 0)) {
            ansctr++;
            if (!roaring_bitmap_contains(r1_andnot_r2, i))
                printf("missing %d\n", i);
        } else if (roaring_bitmap_contains(r1_andnot_r2, i))
            printf("surplus %d\n", i);
    }

    assert_int_equal(roaring_bitmap_get_cardinality(r1_andnot_r2), ansctr);
    roaring_bitmap_free(r1_andnot_r2);
    roaring_bitmap_free(r2);
    roaring_bitmap_free(r1);

    // some tougher tests on synthetic data

    roaring_bitmap_t *r[] = {
        // ascending density, last containers might be runs
        gen_bitmap(0.0, 1e-6, 1, 0, 0, 1000000),
        // descending density, first containers might be runs
        gen_bitmap(1.0, -1e-6, 1, 0, 0, 1000000),
        // uniformly rather sparse
        gen_bitmap(1e-5, 0.0, 1, 0, 0, 2000000),
        // uniformly rather sparse with runs
        gen_bitmap(1e-5, 0.0, 3, 0, 0, 2000000),
        // uniformly rather dense
        gen_bitmap(1e-1, 0.0, 1, 0, 0, 2000000),
        // ascending density but never too dense
        gen_bitmap(0.001, 1e-7, 1, 0, 0, 1000000),
        // ascending density but very sparse
        gen_bitmap(0.0, 1e-10, 1, 0, 0, 1000000),
        // descending with a gap
        gen_bitmap(0.5, -1e-6, 1, 600000, 800000, 1000000),
        //  gap elsewhere
        gen_bitmap(1, -1e-6, 1, 300000, 500000, 1000000),
        0  // sentinel
    };

    for (int i = 0; r[i]; ++i) {
        for (int j = i; r[j]; ++j) {
            roaring_bitmap_t *expected = synthesized_andnot(r[i], r[j]);
            roaring_bitmap_t *result = roaring_bitmap_andnot(r[i], r[j]);

            bool is_equal = roaring_bitmap_equals(expected, result);

            assert_true(is_equal);
            roaring_bitmap_free(expected);
            roaring_bitmap_free(result);
        }
    }
    for (int i = 0; r[i]; ++i) roaring_bitmap_free(r[i]);
}

void test_andnot_true() { test_andnot(true); }

void test_andnot_false() { test_andnot(false); }

void test_andnot_inplace(bool copy_on_write) {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    r1->copy_on_write = copy_on_write;
    roaring_bitmap_t *r2 = roaring_bitmap_create();
    r2->copy_on_write = copy_on_write;

    int data1[] = {1,
                   2,
                   65536 * 2 + 1,
                   65536 * 2 + 2,
                   65536 * 3 + 1,
                   65536 * 3 + 2,
                   65536 * 10 + 1,
                   65536 * 10 + 2,
                   65536 * 16 + 1,
                   65536 * 16 + 2,
                   65536 * 20 + 1,
                   65536 * 21 + 1,
                   -1};
    roaring_bitmap_t *rb1 = roaring_from_sentinel_array(data1, copy_on_write);
    int data2[] = {2,
                   3,
                   65536 * 10 + 2,
                   65536 * 10 + 3,
                   65536 * 12 + 2,
                   65536 * 12 + 3,
                   65536 * 14 + 2,
                   65536 * 14 + 3,
                   65536 * 16 + 2,
                   65536 * 16 + 3,
                   -1};
    roaring_bitmap_t *rb2 = roaring_from_sentinel_array(data2, copy_on_write);

    int data3[] = {2,
                   3,
                   65536 * 10 + 1,
                   65536 * 10 + 2,
                   65536 * 12 + 2,
                   65536 * 12 + 3,
                   65536 * 14 + 2,
                   65536 * 14 + 3,
                   65536 * 16 + 2,
                   65536 * 16 + 3,
                   -1};
    roaring_bitmap_t *rb3 = roaring_from_sentinel_array(data3, copy_on_write);
    int d1_minus_d2[] = {1,
                         65536 * 2 + 1,
                         65536 * 2 + 2,
                         65536 * 3 + 1,
                         65536 * 3 + 2,
                         65536 * 10 + 1,
                         65536 * 16 + 1,
                         65536 * 20 + 1,
                         65536 * 21 + 1,
                         -1};
    roaring_bitmap_t *rb1_minus_rb2 =
        roaring_from_sentinel_array(d1_minus_d2, copy_on_write);

    int d1_minus_d3[] = {1, 65536 * 2 + 1, 65536 * 2 + 2, 65536 * 3 + 1,
                         65536 * 3 + 2,
                         // 65536*10+1,
                         65536 * 16 + 1, 65536 * 20 + 1, 65536 * 21 + 1, -1};
    roaring_bitmap_t *rb1_minus_rb3 =
        roaring_from_sentinel_array(d1_minus_d3, copy_on_write);

    int d2_minus_d1[] = {3,
                         65536 * 10 + 3,
                         65536 * 12 + 2,
                         65536 * 12 + 3,
                         65536 * 14 + 2,
                         65536 * 14 + 3,
                         65536 * 16 + 3,
                         -1};

    roaring_bitmap_t *rb2_minus_rb1 =
        roaring_from_sentinel_array(d2_minus_d1, copy_on_write);

    int d3_minus_d1[] = {3,
                         // 65536*10+3,
                         65536 * 12 + 2, 65536 * 12 + 3, 65536 * 14 + 2,
                         65536 * 14 + 3, 65536 * 16 + 3, -1};
    roaring_bitmap_t *rb3_minus_rb1 =
        roaring_from_sentinel_array(d3_minus_d1, copy_on_write);

    int d3_minus_d2[] = {65536 * 10 + 1, -1};
    roaring_bitmap_t *rb3_minus_rb2 =
        roaring_from_sentinel_array(d3_minus_d2, copy_on_write);

    roaring_bitmap_t *cpy = roaring_bitmap_copy(rb1);
    roaring_bitmap_andnot_inplace(cpy, rb2);
    assert_true(roaring_bitmap_equals(rb1_minus_rb2, cpy));
    roaring_bitmap_free(cpy);

    cpy = roaring_bitmap_copy(rb1);
    roaring_bitmap_andnot_inplace(cpy, rb3);
    assert_true(roaring_bitmap_equals(rb1_minus_rb3, cpy));
    roaring_bitmap_free(cpy);

    cpy = roaring_bitmap_copy(rb2);
    roaring_bitmap_andnot_inplace(cpy, rb1);
    assert_true(roaring_bitmap_equals(rb2_minus_rb1, cpy));
    roaring_bitmap_free(cpy);

    cpy = roaring_bitmap_copy(rb3);
    roaring_bitmap_andnot_inplace(cpy, rb1);
    assert_true(roaring_bitmap_equals(rb3_minus_rb1, cpy));
    roaring_bitmap_free(cpy);

    cpy = roaring_bitmap_copy(rb3);
    roaring_bitmap_andnot_inplace(cpy, rb2);
    assert_true(roaring_bitmap_equals(rb3_minus_rb2, cpy));
    roaring_bitmap_free(cpy);

    roaring_bitmap_t *large_run_bitmap =
        roaring_bitmap_from_range(2, 11 * 65536 + 27, 1);

    cpy = roaring_bitmap_copy(rb1);
    roaring_bitmap_andnot_inplace(cpy, large_run_bitmap);

    int d1_minus_largerun[] = {
        1, 65536 * 16 + 1, 65536 * 16 + 2, 65536 * 20 + 1, 65536 * 21 + 1, -1};
    roaring_bitmap_t *rb1_minus_largerun =
        roaring_from_sentinel_array(d1_minus_largerun, copy_on_write);
    assert_true(roaring_bitmap_equals(rb1_minus_largerun, cpy));
    roaring_bitmap_free(cpy);

    roaring_bitmap_free(rb1);
    roaring_bitmap_free(rb2);
    roaring_bitmap_free(rb3);
    roaring_bitmap_free(rb1_minus_rb2);
    roaring_bitmap_free(rb1_minus_rb3);
    roaring_bitmap_free(rb2_minus_rb1);
    roaring_bitmap_free(rb3_minus_rb1);
    roaring_bitmap_free(rb3_minus_rb2);
    roaring_bitmap_free(rb1_minus_largerun);
    roaring_bitmap_free(large_run_bitmap);

    int diff_cardinality = 0;
    for (uint32_t i = 0; i < 300; ++i) {
        if (i % 2 == 0) roaring_bitmap_add(r1, i);
        if (i % 3 == 0) roaring_bitmap_add(r2, i);
        if ((i % 2 == 0) && (i % 3 != 0)) ++diff_cardinality;
    }
    roaring_bitmap_andnot_inplace(r1, r2);
    assert_int_equal(roaring_bitmap_get_cardinality(r1), diff_cardinality);

    roaring_bitmap_free(r2);
    roaring_bitmap_free(r1);

    // some tougher tests on synthetic data

    roaring_bitmap_t *r[] = {
        // ascending density, last containers might be runs
        gen_bitmap(0.0, 1e-6, 1, 0, 0, 1000000),
        // descending density, first containers might be runs
        gen_bitmap(1.0, -1e-6, 1, 0, 0, 1000000),
        // uniformly rather sparse
        gen_bitmap(1e-5, 0.0, 1, 0, 0, 2000000),
        // uniformly rather sparse with runs
        gen_bitmap(1e-5, 0.0, 3, 0, 0, 2000000),
        // uniformly rather dense
        gen_bitmap(1e-1, 0.0, 1, 0, 0, 2000000),
        // ascending density but never too dense
        gen_bitmap(0.001, 1e-7, 1, 0, 0, 1000000),
        // ascending density but very sparse
        gen_bitmap(0.0, 1e-10, 1, 0, 0, 1000000),
        // descending with a gap
        gen_bitmap(0.5, -1e-6, 1, 600000, 800000, 1000000),
        //  gap elsewhere
        gen_bitmap(1, -1e-6, 1, 300000, 500000, 1000000),
        0  // sentinel
    };

    for (int i = 0; r[i]; ++i) {
        for (int j = i + 1; r[j]; ++j) {
            roaring_bitmap_t *expected = synthesized_andnot(r[i], r[j]);
            roaring_bitmap_t *copy = roaring_bitmap_copy(r[i]);
            copy->copy_on_write = copy_on_write;

            roaring_bitmap_andnot_inplace(copy, r[j]);

            bool is_equal = roaring_bitmap_equals(expected, copy);
            if (!is_equal) {
                printf("problem with i=%d j=%d\n", i, j);
                printf("copy's cardinality  is %d and expected's is %d\n",
                       (int)roaring_bitmap_get_cardinality(copy),
                       (int)roaring_bitmap_get_cardinality(expected));
                show_difference(copy, expected);
            }

            assert_true(is_equal);
            roaring_bitmap_free(expected);
            roaring_bitmap_free(copy);
        }
    }
    for (int i = 0; r[i]; ++i) roaring_bitmap_free(r[i]);
}

void test_andnot_inplace_true() { test_andnot_inplace(true); }

void test_andnot_inplace_false() { test_xor_inplace(false); }

static roaring_bitmap_t *make_roaring_from_array(uint32_t *a, int len) {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    for (int i = 0; i < len; ++i) roaring_bitmap_add(r1, a[i]);
    return r1;
}

void test_conversion_to_int_array() {
    int ans_ctr = 0;
    uint32_t *ans = calloc(100000, sizeof(int32_t));

    // a dense bitmap container  (best done with runs)
    for (uint32_t i = 0; i < 50000; ++i) {
        if (i != 30000) {  // making 2 runs
            ans[ans_ctr++] = i;
        }
    }

    // a sparse one
    for (uint32_t i = 70000; i < 130000; i += 17) {
        ans[ans_ctr++] = i;
    }

    // a dense one but not good for runs

    for (uint32_t i = 65536 * 3; i < 65536 * 4; i++) {
        if (i % 3 != 0) {
            ans[ans_ctr++] = i;
        }
    }

    roaring_bitmap_t *r1 = make_roaring_from_array(ans, ans_ctr);

    uint64_t card = roaring_bitmap_get_cardinality(r1);
    uint32_t *arr = (uint32_t *)malloc(card * sizeof(uint32_t));
    roaring_bitmap_to_uint32_array(r1, arr);

    assert_true(array_equals(arr, card, ans, ans_ctr));
    roaring_bitmap_free(r1);
    free(arr);
    free(ans);
}

void test_conversion_to_int_array_with_runoptimize() {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    int ans_ctr = 0;
    uint32_t *ans = calloc(100000, sizeof(int32_t));

    // a dense bitmap container  (best done with runs)
    for (uint32_t i = 0; i < 50000; ++i) {
        if (i != 30000) {  // making 2 runs
            ans[ans_ctr++] = i;
        }
    }

    // a sparse one
    for (uint32_t i = 70000; i < 130000; i += 17) {
        ans[ans_ctr++] = i;
    }

    // a dense one but not good for runs

    for (uint32_t i = 65536 * 3; i < 65536 * 4; i++) {
        if (i % 3 != 0) {
            ans[ans_ctr++] = i;
        }
    }
    roaring_bitmap_free(r1);

    r1 = make_roaring_from_array(ans, ans_ctr);
    assert_true(roaring_bitmap_run_optimize(r1));

    uint64_t card = roaring_bitmap_get_cardinality(r1);
    uint32_t *arr = (uint32_t *)malloc(card * sizeof(uint32_t));
    roaring_bitmap_to_uint32_array(r1, arr);

    assert_true(array_equals(arr, card, ans, ans_ctr));
    roaring_bitmap_free(r1);
    free(arr);
    free(ans);
}

void test_array_to_run() {
    int ans_ctr = 0;
    uint32_t *ans = calloc(100000, sizeof(int32_t));

    // array container  (best done with runs)
    for (uint32_t i = 0; i < 500; ++i) {
        if (i != 300) {  // making 2 runs
            ans[ans_ctr++] = 65536 + i;
        }
    }

    roaring_bitmap_t *r1 = make_roaring_from_array(ans, ans_ctr);
    assert_true(roaring_bitmap_run_optimize(r1));

    uint64_t card = roaring_bitmap_get_cardinality(r1);
    uint32_t *arr = (uint32_t *)malloc(card * sizeof(uint32_t));
    roaring_bitmap_to_uint32_array(r1, arr);

    assert_true(array_equals(arr, card, ans, ans_ctr));
    roaring_bitmap_free(r1);
    free(arr);
    free(ans);
}

void test_array_to_self() {
    int ans_ctr = 0;

    uint32_t *ans = calloc(100000, sizeof(int32_t));

    // array container  (best not done with runs)
    for (uint32_t i = 0; i < 500; i += 2) {
        if (i != 300) {  // making 2 runs
            ans[ans_ctr++] = 65536 + i;
        }
    }

    roaring_bitmap_t *r1 = make_roaring_from_array(ans, ans_ctr);
    assert_false(roaring_bitmap_run_optimize(r1));

    uint64_t card = roaring_bitmap_get_cardinality(r1);
    uint32_t *arr = (uint32_t *)malloc(card * sizeof(uint32_t));
    roaring_bitmap_to_uint32_array(r1, arr);

    assert_true(array_equals(arr, card, ans, ans_ctr));
    roaring_bitmap_free(r1);
    free(arr);
    free(ans);
}

void test_bitset_to_self() {
    int ans_ctr = 0;
    uint32_t *ans = calloc(100000, sizeof(int32_t));

    // bitset container  (best not done with runs)
    for (uint32_t i = 0; i < 50000; i += 2) {
        if (i != 300) {  // making 2 runs
            ans[ans_ctr++] = 65536 + i;
        }
    }

    roaring_bitmap_t *r1 = make_roaring_from_array(ans, ans_ctr);
    assert_false(roaring_bitmap_run_optimize(r1));

    uint64_t card = roaring_bitmap_get_cardinality(r1);
    uint32_t *arr = (uint32_t *)malloc(card * sizeof(uint32_t));
    roaring_bitmap_to_uint32_array(r1, arr);

    assert_true(array_equals(arr, card, ans, ans_ctr));
    roaring_bitmap_free(r1);
    free(arr);
    free(ans);
}

void test_bitset_to_run() {
    int ans_ctr = 0;
    uint32_t *ans = calloc(100000, sizeof(int32_t));

    // bitset container  (best done with runs)
    for (uint32_t i = 0; i < 50000; i++) {
        if (i != 300) {  // making 2 runs
            ans[ans_ctr++] = 65536 + i;
        }
    }

    roaring_bitmap_t *r1 = make_roaring_from_array(ans, ans_ctr);
    assert(roaring_bitmap_run_optimize(r1));

    uint64_t card = roaring_bitmap_get_cardinality(r1);
    uint32_t *arr = (uint32_t *)malloc(card * sizeof(uint32_t));
    roaring_bitmap_to_uint32_array(r1, arr);

    assert_true(array_equals(arr, card, ans, ans_ctr));
    roaring_bitmap_free(r1);
    free(arr);
    free(ans);
}

// not sure how to get containers that are runcontainers but not efficient

void test_run_to_self() {
    int ans_ctr = 0;
    uint32_t *ans = calloc(100000, sizeof(int32_t));

    // bitset container  (best done with runs)
    for (uint32_t i = 0; i < 50000; i++) {
        if (i != 300) {  // making 2 runs
            ans[ans_ctr++] = 65536 + i;
        }
    }

    roaring_bitmap_t *r1 = make_roaring_from_array(ans, ans_ctr);
    bool b = roaring_bitmap_run_optimize(r1);  // will make a run container
    b = roaring_bitmap_run_optimize(r1);       // we hope it will keep it
    assert_true(b);  // still true there is a runcontainer

    uint64_t card = roaring_bitmap_get_cardinality(r1);
    uint32_t *arr = (uint32_t *)malloc(card * sizeof(uint32_t));
    roaring_bitmap_to_uint32_array(r1, arr);

    assert_true(array_equals(arr, card, ans, ans_ctr));
    roaring_bitmap_free(r1);
    free(arr);
    free(ans);
}


void test_remove_run_to_bitset() {
    int ans_ctr = 0;
    uint32_t *ans = calloc(100000, sizeof(int32_t));

    // bitset container  (best done with runs)
    for (uint32_t i = 0; i < 50000; i++) {
        if (i != 300) {  // making 2 runs
            ans[ans_ctr++] = 65536 + i;
        }
    }

    roaring_bitmap_t *r1 = make_roaring_from_array(ans, ans_ctr);
    assert_true(roaring_bitmap_run_optimize(r1));  // will make a run container
    assert_true(roaring_bitmap_remove_run_compression(r1));  // removal done
    assert_true(
        roaring_bitmap_run_optimize(r1));  // there is again a run container

    uint64_t card = roaring_bitmap_get_cardinality(r1);
    uint32_t *arr = (uint32_t *)malloc(card * sizeof(uint32_t));
    roaring_bitmap_to_uint32_array(r1, arr);

    assert_true(array_equals(arr, card, ans, ans_ctr));
    roaring_bitmap_free(r1);
    free(arr);
    free(ans);
}

void test_remove_run_to_array() {
    int ans_ctr = 0;
    uint32_t *ans = calloc(100000, sizeof(int32_t));

    // array  (best done with runs)
    for (uint32_t i = 0; i < 500; i++) {
        if (i != 300) {  // making 2 runs
            ans[ans_ctr++] = 65536 + i;
        }
    }

    roaring_bitmap_t *r1 = make_roaring_from_array(ans, ans_ctr);
    assert_true(roaring_bitmap_run_optimize(r1));  // will make a run container
    assert_true(roaring_bitmap_remove_run_compression(r1));  // removal done
    assert_true(
        roaring_bitmap_run_optimize(r1));  // there is again a run container

    uint64_t card = roaring_bitmap_get_cardinality(r1);
    uint32_t *arr = (uint32_t *)malloc(card * sizeof(uint32_t));
    roaring_bitmap_to_uint32_array(r1, arr);

    assert_true(array_equals(arr, card, ans, ans_ctr));
    roaring_bitmap_free(r1);
    free(arr);
    free(ans);
}

// array in, array out
void test_negation_array0() {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    assert_non_null(r1);

    roaring_bitmap_t *notted_r1 = roaring_bitmap_flip(r1, 200U, 500U);
    assert_non_null(notted_r1);
    assert_int_equal(300, roaring_bitmap_get_cardinality(notted_r1));

    roaring_bitmap_free(notted_r1);
    roaring_bitmap_free(r1);
}

// array in, array out
void test_negation_array1() {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    assert_non_null(r1);

    roaring_bitmap_add(r1, 1);
    roaring_bitmap_add(r1, 2);
    // roaring_bitmap_add(r1,3);
    roaring_bitmap_add(r1, 4);
    roaring_bitmap_add(r1, 5);
    roaring_bitmap_t *notted_r1 = roaring_bitmap_flip(r1, 2U, 5U);
    assert_non_null(notted_r1);
    assert_int_equal(3, roaring_bitmap_get_cardinality(notted_r1));

    roaring_bitmap_free(notted_r1);
    roaring_bitmap_free(r1);
}

// arrays to bitmaps and runs
void test_negation_array2() {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    assert_non_null(r1);

    for (uint32_t i = 0; i < 100; ++i) {
        roaring_bitmap_add(r1, 2 * i);
        roaring_bitmap_add(r1, 5 * 65536 + 2 * i);
    }

    assert_int_equal(roaring_bitmap_get_cardinality(r1), 200);

    // get the first batch of ones but not the second
    roaring_bitmap_t *notted_r1 = roaring_bitmap_flip(r1, 0U, 100000U);
    assert_non_null(notted_r1);

    // lose 100 for key 0, but gain 100 for key 5
    assert_int_equal(100000, roaring_bitmap_get_cardinality(notted_r1));
    roaring_bitmap_free(notted_r1);

    // flip all ones and beyond
    notted_r1 = roaring_bitmap_flip(r1, 0U, 1000000U);
    assert_non_null(notted_r1);
    assert_int_equal(1000000 - 200, roaring_bitmap_get_cardinality(notted_r1));
    roaring_bitmap_free(notted_r1);

    // Flip some bits in the middle
    notted_r1 = roaring_bitmap_flip(r1, 100000U, 200000U);
    assert_non_null(notted_r1);
    assert_int_equal(100000 + 200, roaring_bitmap_get_cardinality(notted_r1));
    roaring_bitmap_free(notted_r1);

    // flip almost all of the bits, end at an even boundary
    notted_r1 = roaring_bitmap_flip(r1, 1U, 65536 * 6);
    assert_non_null(notted_r1);
    assert_int_equal(65536 * 6 - 200 + 1,
                     roaring_bitmap_get_cardinality(notted_r1));
    roaring_bitmap_free(notted_r1);

    // flip first bunch of the bits, end at an even boundary
    notted_r1 = roaring_bitmap_flip(r1, 1U, 65536 * 5);
    assert_non_null(notted_r1);
    assert_int_equal(65536 * 5 - 100 + 1 + 100,
                     roaring_bitmap_get_cardinality(notted_r1));
    roaring_bitmap_free(notted_r1);

    roaring_bitmap_free(r1);
}

// bitmaps to bitmaps and runs
void test_negation_bitset1() {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    assert_non_null(r1);

    for (uint32_t i = 0; i < 25000; ++i) {
        roaring_bitmap_add(r1, 2 * i);
        roaring_bitmap_add(r1, 5 * 65536 + 2 * i);
    }

    assert_int_equal(roaring_bitmap_get_cardinality(r1), 50000);

    // get the first batch of ones but not the second
    roaring_bitmap_t *notted_r1 = roaring_bitmap_flip(r1, 0U, 100000U);
    assert_non_null(notted_r1);

    // lose 25000 for key 0, but gain 25000 for key 5
    assert_int_equal(100000, roaring_bitmap_get_cardinality(notted_r1));
    roaring_bitmap_free(notted_r1);

    // flip all ones and beyond
    notted_r1 = roaring_bitmap_flip(r1, 0U, 1000000U);
    assert_non_null(notted_r1);
    assert_int_equal(1000000 - 50000,
                     roaring_bitmap_get_cardinality(notted_r1));
    roaring_bitmap_free(notted_r1);

    // Flip some bits in the middle
    notted_r1 = roaring_bitmap_flip(r1, 100000U, 200000U);
    assert_non_null(notted_r1);
    assert_int_equal(100000 + 50000, roaring_bitmap_get_cardinality(notted_r1));
    roaring_bitmap_free(notted_r1);

    // flip almost all of the bits, end at an even boundary
    notted_r1 = roaring_bitmap_flip(r1, 1U, 65536 * 6);
    assert_non_null(notted_r1);
    assert_int_equal(65536 * 6 - 50000 + 1,
                     roaring_bitmap_get_cardinality(notted_r1));
    roaring_bitmap_free(notted_r1);

    // flip first bunch of the bits, end at an even boundary
    notted_r1 = roaring_bitmap_flip(r1, 1U, 65536 * 5);
    assert_non_null(notted_r1);
    assert_int_equal(65536 * 5 - 25000 + 1 + 25000,
                     roaring_bitmap_get_cardinality(notted_r1));
    roaring_bitmap_free(notted_r1);

    roaring_bitmap_free(r1);
}

void test_negation_helper(bool runopt, uint32_t gap) {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    assert_non_null(r1);

    for (uint32_t i = 0; i < 65536; ++i) {
        if (i % 147 < gap) continue;
        roaring_bitmap_add(r1, i);
        roaring_bitmap_add(r1, 5 * 65536 + i);
    }
    if (runopt) {
        bool hasrun = roaring_bitmap_run_optimize(r1);
        assert_true(hasrun);
    }

    int orig_card = roaring_bitmap_get_cardinality(r1);

    // get the first batch of ones but not the second
    roaring_bitmap_t *notted_r1 = roaring_bitmap_flip(r1, 0U, 100000U);
    assert_non_null(notted_r1);

    // lose some for key 0, but gain same num for key 5
    assert_int_equal(100000, roaring_bitmap_get_cardinality(notted_r1));
    roaring_bitmap_free(notted_r1);

    // flip all ones and beyond
    notted_r1 = roaring_bitmap_flip(r1, 0U, 1000000U);
    assert_non_null(notted_r1);
    assert_int_equal(1000000 - orig_card,
                     roaring_bitmap_get_cardinality(notted_r1));
    roaring_bitmap_free(notted_r1);

    // Flip some bits in the middle
    notted_r1 = roaring_bitmap_flip(r1, 100000U, 200000U);
    assert_non_null(notted_r1);
    assert_int_equal(100000 + orig_card,
                     roaring_bitmap_get_cardinality(notted_r1));
    roaring_bitmap_free(notted_r1);

    // flip almost all of the bits, end at an even boundary
    notted_r1 = roaring_bitmap_flip(r1, 1U, 65536 * 6);
    assert_non_null(notted_r1);
    assert_int_equal((65536 * 6 - 1) - orig_card,
                     roaring_bitmap_get_cardinality(notted_r1));
    roaring_bitmap_free(notted_r1);

    // flip first bunch of the bits, end at an even boundary
    notted_r1 = roaring_bitmap_flip(r1, 1U, 65536 * 5);
    assert_non_null(notted_r1);
    assert_int_equal(65536 * 5 - 1 - (orig_card / 2) + (orig_card / 2),
                     roaring_bitmap_get_cardinality(notted_r1));
    roaring_bitmap_free(notted_r1);

    roaring_bitmap_free(r1);
}

// bitmaps to arrays and runs
void test_negation_bitset2() { test_negation_helper(false, 2); }

// runs to arrays
void test_negation_run1() { test_negation_helper(true, 1); }

// runs to runs
void test_negation_run2() { test_negation_helper(true, 30); }

/* Now, same thing except inplace.  At this level, cannot really know if
 * inplace
 * done */

// array in, array out
void test_inplace_negation_array0() {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    assert_non_null(r1);

    roaring_bitmap_flip_inplace(r1, 200U, 500U);
    assert_non_null(r1);
    assert_int_equal(300, roaring_bitmap_get_cardinality(r1));

    roaring_bitmap_free(r1);
}

// array in, array out
void test_inplace_negation_array1() {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    assert_non_null(r1);

    roaring_bitmap_add(r1, 1);
    roaring_bitmap_add(r1, 2);

    roaring_bitmap_add(r1, 4);
    roaring_bitmap_add(r1, 5);
    roaring_bitmap_flip_inplace(r1, 2U, 5U);
    assert_non_null(r1);
    assert_int_equal(3, roaring_bitmap_get_cardinality(r1));

    roaring_bitmap_free(r1);
}

// arrays to bitmaps and runs
void test_inplace_negation_array2() {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    assert_non_null(r1);

    for (uint32_t i = 0; i < 100; ++i) {
        roaring_bitmap_add(r1, 2 * i);
        roaring_bitmap_add(r1, 5 * 65536 + 2 * i);
    }
    roaring_bitmap_t *r1_orig = roaring_bitmap_copy(r1);

    assert_int_equal(roaring_bitmap_get_cardinality(r1), 200);

    // get the first batch of ones but not the second
    roaring_bitmap_flip_inplace(r1, 0U, 100000U);
    assert_non_null(r1);

    // lose 100 for key 0, but gain 100 for key 5
    assert_int_equal(100000, roaring_bitmap_get_cardinality(r1));
    roaring_bitmap_free(r1);
    r1 = roaring_bitmap_copy(r1_orig);

    // flip all ones and beyond
    roaring_bitmap_flip_inplace(r1, 0U, 1000000U);
    assert_non_null(r1);
    assert_int_equal(1000000 - 200, roaring_bitmap_get_cardinality(r1));
    roaring_bitmap_free(r1);
    r1 = roaring_bitmap_copy(r1_orig);

    // Flip some bits in the middle
    roaring_bitmap_flip_inplace(r1, 100000U, 200000U);
    assert_non_null(r1);
    assert_int_equal(100000 + 200, roaring_bitmap_get_cardinality(r1));
    roaring_bitmap_free(r1);
    r1 = roaring_bitmap_copy(r1_orig);

    // flip almost all of the bits, end at an even boundary
    roaring_bitmap_flip_inplace(r1, 1U, 65536 * 6);
    assert_non_null(r1);
    assert_int_equal(65536 * 6 - 200 + 1, roaring_bitmap_get_cardinality(r1));
    roaring_bitmap_free(r1);
    r1 = roaring_bitmap_copy(r1_orig);

    // flip first bunch of the bits, end at an even boundary
    roaring_bitmap_flip_inplace(r1, 1U, 65536 * 5);
    assert_non_null(r1);
    assert_int_equal(65536 * 5 - 100 + 1 + 100,
                     roaring_bitmap_get_cardinality(r1));
    /* */
    roaring_bitmap_free(r1_orig);
    roaring_bitmap_free(r1);
}

// bitmaps to bitmaps and runs
void test_inplace_negation_bitset1() {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    assert_non_null(r1);

    for (uint32_t i = 0; i < 25000; ++i) {
        roaring_bitmap_add(r1, 2 * i);
        roaring_bitmap_add(r1, 5 * 65536 + 2 * i);
    }

    roaring_bitmap_t *r1_orig = roaring_bitmap_copy(r1);

    assert_int_equal(roaring_bitmap_get_cardinality(r1), 50000);

    // get the first batch of ones but not the second
    roaring_bitmap_flip_inplace(r1, 0U, 100000U);
    assert_non_null(r1);

    // lose 25000 for key 0, but gain 25000 for key 5
    assert_int_equal(100000, roaring_bitmap_get_cardinality(r1));
    roaring_bitmap_free(r1);
    r1 = roaring_bitmap_copy(r1_orig);

    // flip all ones and beyond
    roaring_bitmap_flip_inplace(r1, 0U, 1000000U);
    assert_non_null(r1);
    assert_int_equal(1000000 - 50000, roaring_bitmap_get_cardinality(r1));
    roaring_bitmap_free(r1);
    r1 = roaring_bitmap_copy(r1_orig);

    // Flip some bits in the middle
    roaring_bitmap_flip_inplace(r1, 100000U, 200000U);
    assert_non_null(r1);
    assert_int_equal(100000 + 50000, roaring_bitmap_get_cardinality(r1));
    roaring_bitmap_free(r1);
    r1 = roaring_bitmap_copy(r1_orig);

    // flip almost all of the bits, end at an even boundary
    roaring_bitmap_flip_inplace(r1, 1U, 65536 * 6);
    assert_non_null(r1);
    assert_int_equal(65536 * 6 - 50000 + 1, roaring_bitmap_get_cardinality(r1));
    roaring_bitmap_free(r1);
    r1 = roaring_bitmap_copy(r1_orig);

    // flip first bunch of the bits, end at an even boundary
    roaring_bitmap_flip_inplace(r1, 1U, 65536 * 5);
    assert_non_null(r1);
    assert_int_equal(65536 * 5 - 25000 + 1 + 25000,
                     roaring_bitmap_get_cardinality(r1));
    roaring_bitmap_free(r1);

    roaring_bitmap_free(r1_orig);
}

void test_inplace_negation_helper(bool runopt, uint32_t gap) {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    assert_non_null(r1);

    for (uint32_t i = 0; i < 65536; ++i) {
        if (i % 147 < gap) continue;
        roaring_bitmap_add(r1, i);
        roaring_bitmap_add(r1, 5 * 65536 + i);
    }
    if (runopt) {
        bool hasrun = roaring_bitmap_run_optimize(r1);
        assert_true(hasrun);
    }

    int orig_card = roaring_bitmap_get_cardinality(r1);
    roaring_bitmap_t *r1_orig = roaring_bitmap_copy(r1);

    // get the first batch of ones but not the second
    roaring_bitmap_flip_inplace(r1, 0U, 100000U);
    assert_non_null(r1);

    // lose some for key 0, but gain same num for key 5
    assert_int_equal(100000, roaring_bitmap_get_cardinality(r1));
    roaring_bitmap_free(r1);

    // flip all ones and beyond
    r1 = roaring_bitmap_copy(r1_orig);
    roaring_bitmap_flip_inplace(r1, 0U, 1000000U);
    assert_non_null(r1);
    assert_int_equal(1000000 - orig_card, roaring_bitmap_get_cardinality(r1));
    roaring_bitmap_free(r1);

    // Flip some bits in the middle
    r1 = roaring_bitmap_copy(r1_orig);
    roaring_bitmap_flip_inplace(r1, 100000U, 200000U);
    assert_non_null(r1);
    assert_int_equal(100000 + orig_card, roaring_bitmap_get_cardinality(r1));
    roaring_bitmap_free(r1);

    // flip almost all of the bits, end at an even boundary
    r1 = roaring_bitmap_copy(r1_orig);
    roaring_bitmap_flip_inplace(r1, 1U, 65536 * 6);
    assert_non_null(r1);
    assert_int_equal((65536 * 6 - 1) - orig_card,
                     roaring_bitmap_get_cardinality(r1));
    roaring_bitmap_free(r1);

    // flip first bunch of the bits, end at an even boundary
    r1 = roaring_bitmap_copy(r1_orig);
    roaring_bitmap_flip_inplace(r1, 1U, 65536 * 5);
    assert_non_null(r1);
    assert_int_equal(65536 * 5 - 1 - (orig_card / 2) + (orig_card / 2),
                     roaring_bitmap_get_cardinality(r1));
    roaring_bitmap_free(r1);

    roaring_bitmap_free(r1_orig);
}

// bitmaps to arrays and runs
void test_inplace_negation_bitset2() { test_inplace_negation_helper(false, 2); }

// runs to arrays
void test_inplace_negation_run1() { test_inplace_negation_helper(true, 1); }

// runs to runs
void test_inplace_negation_run2() { test_inplace_negation_helper(true, 30); }

// runs to bitmaps is hard to do.
// TODO it

void test_rand_flips() {
    srand(1234);
    const int min_runs = 1;
    const int flip_trials = 5;  // these are expensive tests
    const int range = 2000000;
    char *input = malloc(range);
    char *output = malloc(range);

    for (int card = 2; card < 1000000; card *= 8) {
        printf("test_rand_flips with attempted card %d", card);

        roaring_bitmap_t *r = roaring_bitmap_create();
        memset(input, 0, range);
        for (int i = 0; i < card; ++i) {
            float f1 = rand() / (float)RAND_MAX;
            float f2 = rand() / (float)RAND_MAX;
            float f3 = rand() / (float)RAND_MAX;
            int pos = (int)(f1 * f2 * f3 *
                            range);  // denser at the start, sparser at end
            roaring_bitmap_add(r, pos);
            input[pos] = 1;
        }
        for (int i = 0; i < min_runs; ++i) {
            int startpos = rand() % (range / 2);
            for (int j = startpos; j < startpos + 65536 * 2; ++j)
                if (j % 147 < 100) {
                    roaring_bitmap_add(r, j);
                    input[j] = 1;
                }
        }
        roaring_bitmap_run_optimize(r);
        printf(" and actual card = %d\n",
               (int)roaring_bitmap_get_cardinality(r));

        for (int i = 0; i < flip_trials; ++i) {
            int start = rand() % (range - 1);
            int len = rand() % (range - start);
            roaring_bitmap_t *ans = roaring_bitmap_flip(r, start, start + len);
            memcpy(output, input, range);
            for (int j = start; j < start + len; ++j) output[j] = 1 - input[j];

            // verify answer
            for (int j = 0; j < range; ++j) {
                assert_true(((bool)output[j]) ==
                            roaring_bitmap_contains(ans, j));
            }

            roaring_bitmap_free(ans);
        }
        roaring_bitmap_free(r);
    }
    free(output);
    free(input);
}

// randomized flipping test - inplace version
void test_inplace_rand_flips() {
    srand(1234);
    const int min_runs = 1;
    const int flip_trials = 5;  // these are expensive tests
    const int range = 2000000;
    char *input = malloc(range);
    char *output = malloc(range);

    for (int card = 2; card < 1000000; card *= 8) {
        printf("test_inplace_rand_flips with attempted card %d", card);

        roaring_bitmap_t *r = roaring_bitmap_create();
        memset(input, 0, range);
        for (int i = 0; i < card; ++i) {
            float f1 = rand() / (float)RAND_MAX;
            float f2 = rand() / (float)RAND_MAX;
            float f3 = rand() / (float)RAND_MAX;
            int pos = (int)(f1 * f2 * f3 *
                            range);  // denser at the start, sparser at end
            roaring_bitmap_add(r, pos);
            input[pos] = 1;
        }
        for (int i = 0; i < min_runs; ++i) {
            int startpos = rand() % (range / 2);
            for (int j = startpos; j < startpos + 65536 * 2; ++j)
                if (j % 147 < 100) {
                    roaring_bitmap_add(r, j);
                    input[j] = 1;
                }
        }
        roaring_bitmap_run_optimize(r);
        printf(" and actual card = %d\n",
               (int)roaring_bitmap_get_cardinality(r));

        roaring_bitmap_t *r_orig = roaring_bitmap_copy(r);

        for (int i = 0; i < flip_trials; ++i) {
            int start = rand() % (range - 1);
            int len = rand() % (range - start);

            roaring_bitmap_flip_inplace(r, start, start + len);
            memcpy(output, input, range);
            for (int j = start; j < start + len; ++j) output[j] = 1 - input[j];

            // verify answer
            for (int j = 0; j < range; ++j) {
                assert_true(((bool)output[j]) == roaring_bitmap_contains(r, j));
            }

            roaring_bitmap_free(r);
            r = roaring_bitmap_copy(r_orig);
        }
        roaring_bitmap_free(r_orig);
        roaring_bitmap_free(r);
    }
    free(output);
    free(input);
}

void test_flip_array_container_removal() {
    roaring_bitmap_t *bm = roaring_bitmap_create();
    for (unsigned val = 0; val < 100; val++) {
        roaring_bitmap_add(bm, val);
    }
    roaring_bitmap_flip_inplace(bm, 0, 100);
    roaring_bitmap_free(bm);
}

void test_flip_bitset_container_removal() {
    roaring_bitmap_t *bm = roaring_bitmap_create();
    for (unsigned val = 0; val < 10000; val++) {
        roaring_bitmap_add(bm, val);
    }
    roaring_bitmap_flip_inplace(bm, 0, 10000);
    roaring_bitmap_free(bm);
}

void test_flip_run_container_removal() {
    roaring_bitmap_t *bm = roaring_bitmap_from_range(0, 10000, 1);
    roaring_bitmap_flip_inplace(bm, 0, 10000);
    roaring_bitmap_free(bm);
}

void test_flip_run_container_removal2() {
    roaring_bitmap_t *bm = roaring_bitmap_from_range(0, 66002, 1);
    roaring_bitmap_flip_inplace(bm, 0, 987653576);
    roaring_bitmap_free(bm);
}

// randomized test for rank query
void select_test() {
    srand(1234);
    const int min_runs = 1;
    const uint32_t range = 2000000;
    char *input = malloc(range);

    for (int card = 2; card < 1000000; card *= 8) {
        printf("select_test with attempted card %d", card);

        roaring_bitmap_t *r = roaring_bitmap_create();
        memset(input, 0, range);
        for (int i = 0; i < card; ++i) {
            float f1 = rand() / (float)RAND_MAX;
            float f2 = rand() / (float)RAND_MAX;
            float f3 = rand() / (float)RAND_MAX;
            int pos = (int)(f1 * f2 * f3 *
                            range);  // denser at the start, sparser at end
            roaring_bitmap_add(r, pos);
            input[pos] = 1;
        }
        for (int i = 0; i < min_runs; ++i) {
            int startpos = rand() % (range / 2);
            for (int j = startpos; j < startpos + 65536 * 2; ++j)
                if (j % 147 < 100) {
                    roaring_bitmap_add(r, j);
                    input[j] = 1;
                }
        }
        roaring_bitmap_run_optimize(r);
        uint32_t true_card = roaring_bitmap_get_cardinality(r);
        printf(" and actual card = %u\n", (unsigned)true_card);

        r->copy_on_write = 1;
        roaring_bitmap_t *r_copy = roaring_bitmap_copy(r);

        void *bitmaps[] = {r, r_copy};
        for (unsigned i_bm = 0; i_bm < 2; i_bm++) {
            uint32_t rank = 0;
            uint32_t element;
            for (uint32_t i = 0; i < true_card; i++) {
                if (input[i]) {
                    assert_true(
                        roaring_bitmap_select(bitmaps[i_bm], rank, &element));
                    assert_int_equal(i, element);
                    rank++;
                }
            }
            for (uint32_t n = 0; n < 10; n++) {
                assert_false(roaring_bitmap_select(bitmaps[i_bm], true_card + n,
                                                   &element));
            }
        }

        roaring_bitmap_free(r);
        roaring_bitmap_free(r_copy);
    }
    free(input);
}


void test_maximum_minimum() {
    for(uint32_t mymin = 123; mymin < 1000000; mymin *=2) {
      // just arrays
      roaring_bitmap_t *r = roaring_bitmap_create();
      uint32_t x = mymin;
      for (; x < 1000 + mymin; x+= 100) {
          roaring_bitmap_add(r, x);
      }
      assert_true(roaring_bitmap_minimum(r) == mymin);
      assert_true(roaring_bitmap_maximum(r) == x - 100);
      // now bitmap
      x = mymin;
      for (; x < 64000 + mymin; x+= 2) {
          roaring_bitmap_add(r, x);
      }
      assert_true(roaring_bitmap_minimum(r) == mymin);
      assert_true(roaring_bitmap_maximum(r) == x - 2);
      // now run
      x = mymin;
      for (; x < 64000 + mymin; x++) {
          roaring_bitmap_add(r, x);
      }
      roaring_bitmap_run_optimize(r);
      assert_true(roaring_bitmap_minimum(r) == mymin);
      assert_true(roaring_bitmap_maximum(r) == x - 1);
      roaring_bitmap_free(r);
  }
}


static uint64_t rank(uint32_t * arr, size_t length, uint32_t x) {
  uint64_t sum = 0;
  for(size_t i = 0; i < length; ++i) {
    if(arr[i] > x) break;
    sum++;
  }
  return sum;
}


void test_rank() {
    for(uint32_t mymin = 123; mymin < 1000000; mymin *=2) {
      // just arrays
      roaring_bitmap_t *r = roaring_bitmap_create();
      uint32_t x = mymin;
      for (; x < 1000 + mymin; x+= 100) {
          roaring_bitmap_add(r, x);
      }
      uint64_t card = roaring_bitmap_get_cardinality(r);
      uint32_t * ans = malloc(card * sizeof(uint32_t));
      roaring_bitmap_to_uint32_array(r,ans);
      for(uint32_t z = 0; z < 1000 + mymin + 10; z+= 10) {
        uint64_t truerank = rank(ans,card,z);
        uint64_t computedrank = roaring_bitmap_rank(r,z);
        if(truerank != computedrank) printf("%d != %d \n",(int)truerank, (int)computedrank);
        assert_true(truerank == computedrank);
      }
      free(ans);
      // now bitmap
      x = mymin;
      for (; x < 64000 + mymin; x+= 2) {
          roaring_bitmap_add(r, x);
      }
      card = roaring_bitmap_get_cardinality(r);
      ans = malloc(card * sizeof(uint32_t));
      roaring_bitmap_to_uint32_array(r,ans);
      for(uint32_t z = 0; z < 64000 + mymin + 10; z+= 10) {
        uint64_t truerank = rank(ans,card,z);
        uint64_t computedrank = roaring_bitmap_rank(r,z);
        if(truerank != computedrank) printf("%d != %d \n",(int)truerank, (int)computedrank);
        assert_true(truerank == computedrank);
      }
      free(ans);
      // now run
      x = mymin;
      for (; x < 64000 + mymin; x++) {
          roaring_bitmap_add(r, x);
      }
      roaring_bitmap_run_optimize(r);
      card = roaring_bitmap_get_cardinality(r);
      ans = malloc(card * sizeof(uint32_t));
      roaring_bitmap_to_uint32_array(r,ans);
      for(uint32_t z = 0; z < 64000 + mymin + 10; z+= 10) {
        uint64_t truerank = rank(ans,card,z);
        uint64_t computedrank = roaring_bitmap_rank(r,z);
        if(truerank != computedrank) printf("%d != %d \n",(int)truerank, (int)computedrank);
        assert_true(truerank == computedrank);
      }
      free(ans);

      roaring_bitmap_free(r);
  }
}

// Return a random value which does not belong to the roaring bitmap.
// Value will be lower than upper_bound.
uint32_t choose_missing_value(roaring_bitmap_t *rb, uint32_t upper_bound) {
    do {
        uint32_t value = rand()%upper_bound;
        if(!roaring_bitmap_contains(rb, value))
            return value;
    } while(true);
}

void test_subset() {
    uint32_t value;
    roaring_bitmap_t *rb1 = roaring_bitmap_create();
    roaring_bitmap_t *rb2 = roaring_bitmap_create();
    assert_true(roaring_bitmap_is_subset(rb1, rb2));
    assert_false(roaring_bitmap_is_strict_subset(rb1, rb2));
    // Sparse values
    for(int i = 0 ; i < 1000 ; i++) {
        roaring_bitmap_add(rb2, choose_missing_value(rb2, UINT32_C(1)<<31));
    }
    assert_true(roaring_bitmap_is_subset(rb1, rb2));
    assert_true(roaring_bitmap_is_strict_subset(rb1, rb2));
    roaring_bitmap_or_inplace(rb1, rb2);
    assert_true(roaring_bitmap_is_subset(rb1, rb2));
    assert_false(roaring_bitmap_is_strict_subset(rb1, rb2));
    value = choose_missing_value(rb1, UINT32_C(1)<<31);
    roaring_bitmap_add(rb1, value);
    roaring_bitmap_add(rb2, choose_missing_value(rb1, UINT32_C(1)<<31));
    assert_false(roaring_bitmap_is_subset(rb1, rb2));
    assert_false(roaring_bitmap_is_strict_subset(rb1, rb2));
    roaring_bitmap_add(rb2, value);
    assert_true(roaring_bitmap_is_subset(rb1, rb2));
    assert_true(roaring_bitmap_is_strict_subset(rb1, rb2));
    // Dense values
    for(int i = 0 ; i < 50000 ; i++) {
        value = choose_missing_value(rb2, 1<<17);
        roaring_bitmap_add(rb1, value);
        roaring_bitmap_add(rb2, value);
    }
    assert_true(roaring_bitmap_is_subset(rb1, rb2));
    assert_true(roaring_bitmap_is_strict_subset(rb1, rb2));
    value = choose_missing_value(rb2, 1<<16);
    roaring_bitmap_add(rb1, value);
    roaring_bitmap_add(rb2, choose_missing_value(rb1, 1<<16));
    assert_false(roaring_bitmap_is_subset(rb1, rb2));
    assert_false(roaring_bitmap_is_strict_subset(rb1, rb2));
    roaring_bitmap_add(rb2, value);
    assert_true(roaring_bitmap_is_subset(rb1, rb2));
    assert_true(roaring_bitmap_is_strict_subset(rb1, rb2));
    roaring_bitmap_free(rb1);
    roaring_bitmap_free(rb2);
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_rank),
        cmocka_unit_test(test_maximum_minimum),
        cmocka_unit_test(test_stats),
        cmocka_unit_test(test_addremove),
        cmocka_unit_test(test_addremoverun),
        cmocka_unit_test(test_basic_add),
        cmocka_unit_test(test_remove_withrun),
        cmocka_unit_test(test_remove_from_copies_true),
        cmocka_unit_test(test_remove_from_copies_false),
        cmocka_unit_test(test_range_and_serialize),
        cmocka_unit_test(test_silly_range),
        cmocka_unit_test(test_example_true),
        cmocka_unit_test(test_example_false),
        cmocka_unit_test(test_uint32_iterator),
        cmocka_unit_test(leaks_with_empty_true),
        cmocka_unit_test(leaks_with_empty_false),
        cmocka_unit_test(test_bitmap_from_range),
        cmocka_unit_test(test_printf),
        cmocka_unit_test(test_printf_withbitmap),
        cmocka_unit_test(test_printf_withrun),
        cmocka_unit_test(test_iterate),
        cmocka_unit_test(test_iterate_empty),
        cmocka_unit_test(test_iterate_withbitmap),
        cmocka_unit_test(test_iterate_withrun),
        cmocka_unit_test(test_serialize),
        cmocka_unit_test(test_portable_serialize),
        cmocka_unit_test(test_add),
        cmocka_unit_test(test_contains),
        cmocka_unit_test(test_intersection_array_x_array),
        cmocka_unit_test(test_intersection_array_x_array_inplace),
        cmocka_unit_test(test_intersection_bitset_x_bitset),
        cmocka_unit_test(test_intersection_bitset_x_bitset_inplace),
        cmocka_unit_test(test_union_true),
        cmocka_unit_test(test_union_false),
        cmocka_unit_test(test_xor_false),
        cmocka_unit_test(test_xor_inplace_false),
        cmocka_unit_test(test_xor_lazy_false),
        cmocka_unit_test(test_xor_lazy_inplace_false),
        cmocka_unit_test(test_xor_true),
        cmocka_unit_test(test_xor_inplace_true),
        cmocka_unit_test(test_xor_lazy_true),
        cmocka_unit_test(test_xor_lazy_inplace_true),
        cmocka_unit_test(test_andnot_false),
        cmocka_unit_test(test_andnot_inplace_false),
        cmocka_unit_test(test_andnot_true),
        cmocka_unit_test(test_andnot_inplace_true),
        cmocka_unit_test(test_conversion_to_int_array),
        cmocka_unit_test(test_array_to_run),
        cmocka_unit_test(test_array_to_self),
        cmocka_unit_test(test_bitset_to_self),
        cmocka_unit_test(test_conversion_to_int_array_with_runoptimize),
        cmocka_unit_test(test_run_to_self),
        cmocka_unit_test(test_remove_run_to_bitset),
        cmocka_unit_test(test_remove_run_to_array),
        cmocka_unit_test(test_negation_array0),
        cmocka_unit_test(test_negation_array1),
        cmocka_unit_test(test_negation_array2),
        cmocka_unit_test(test_negation_bitset1),
        cmocka_unit_test(test_negation_bitset2),
        cmocka_unit_test(test_negation_run1),
        cmocka_unit_test(test_negation_run2),
        cmocka_unit_test(test_rand_flips),
        cmocka_unit_test(test_inplace_negation_array0),
        cmocka_unit_test(test_inplace_negation_array1),
        cmocka_unit_test(test_inplace_negation_array2),
        cmocka_unit_test(test_inplace_negation_bitset1),
        cmocka_unit_test(test_inplace_negation_bitset2),
        cmocka_unit_test(test_inplace_negation_run1),
        cmocka_unit_test(test_inplace_negation_run2),
        cmocka_unit_test(test_inplace_rand_flips),
        cmocka_unit_test(test_flip_array_container_removal),
        cmocka_unit_test(test_flip_bitset_container_removal),
        cmocka_unit_test(test_flip_run_container_removal),
        cmocka_unit_test(test_flip_run_container_removal2),
        cmocka_unit_test(select_test),
        cmocka_unit_test(test_subset),
        // cmocka_unit_test(test_run_to_bitset),
        // cmocka_unit_test(test_run_to_array),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}

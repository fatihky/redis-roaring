/**
* The purpose of this test is to check that we can call CRoaring from C++
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <roaring/roaring.h>
#include "roaring.hh"
#include "roaring64map.hh"
extern "C" {
#include "test.h"
}

bool roaring_iterator_sumall(uint32_t value, void *param) {
    *(uint32_t *)param += value;
    return true;  // we always process all values
}

bool roaring_iterator_sumall64(uint64_t value, void *param) {
    *(uint64_t *)param += value;
    return true;  // we always process all values
}

void test_example(bool copy_on_write) {
    // create a new empty bitmap
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    r1->copy_on_write = copy_on_write;
    assert_ptr_not_equal(r1, NULL);

    // then we can add values
    for (uint32_t i = 100; i < 1000; i++) {
        roaring_bitmap_add(r1, i);
    }
    // check whether a value is contained
    assert_true(roaring_bitmap_contains(r1, 500));

    // compute how many bits there are:
    uint32_t cardinality = roaring_bitmap_get_cardinality(r1);
    printf("Cardinality = %d \n", cardinality);
    assert_int_equal(900, cardinality);

    // if your bitmaps have long runs, you can compress them by calling
    // run_optimize
    uint32_t size = roaring_bitmap_portable_size_in_bytes(r1);
    roaring_bitmap_run_optimize(r1);
    uint32_t compact_size = roaring_bitmap_portable_size_in_bytes(r1);
    printf("size before run optimize %d bytes, and after %d bytes\n", size,
           compact_size);
    // create a new bitmap with varargs
    roaring_bitmap_t *r2 = roaring_bitmap_of(5, 1, 2, 3, 5, 6);
    assert_ptr_not_equal(r2, NULL);
    roaring_bitmap_printf(r2);
    printf("\n");
    // we can also create a bitmap from a pointer to 32-bit integers
    const uint32_t values[] = {2, 3, 4};
    roaring_bitmap_t *r3 = roaring_bitmap_of_ptr(3, values);
    r3->copy_on_write = copy_on_write;
    // we can also go in reverse and go from arrays to bitmaps
    uint64_t card1 = roaring_bitmap_get_cardinality(r1);
    uint32_t *arr1 = new uint32_t[card1];
    assert_ptr_not_equal(arr1, NULL);
    roaring_bitmap_to_uint32_array(r1, arr1);

    roaring_bitmap_t *r1f = roaring_bitmap_of_ptr(card1, arr1);
    delete[] arr1;
    assert_ptr_not_equal(r1f, NULL);

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

    // we can compute intersection two-by-two
    roaring_bitmap_t *i1_2 = roaring_bitmap_and(r1, r2);
    roaring_bitmap_free(i1_2);

    // we can write a bitmap to a pointer and recover it later
    uint32_t expectedsize = roaring_bitmap_portable_size_in_bytes(r1);
    char *serializedbytes = (char *)malloc(expectedsize);
    roaring_bitmap_portable_serialize(r1, serializedbytes);
    roaring_bitmap_t *t = roaring_bitmap_portable_deserialize(serializedbytes);
    assert_true(roaring_bitmap_equals(r1, t));
    roaring_bitmap_free(t);
    free(serializedbytes);

    // we can iterate over all values using custom functions
    uint32_t counter = 0;
    roaring_iterate(r1, roaring_iterator_sumall, &counter);
    /**
     * void roaring_iterator_sumall(uint32_t value, void *param) {
     *        *(uint32_t *) param += value;
     *  }
     *
     */

    roaring_bitmap_free(r1);
    roaring_bitmap_free(r2);
    roaring_bitmap_free(r3);
}

void test_example_cpp(bool copy_on_write) {
    // create a new empty bitmap
    Roaring r1;
    r1.setCopyOnWrite(copy_on_write);
    // then we can add values
    for (uint32_t i = 100; i < 1000; i++) {
        r1.add(i);
    }

    // check whether a value is contained
    assert_true(r1.contains(500));

    // compute how many bits there are:
    uint32_t cardinality = r1.cardinality();
    std::cout << "Cardinality = " << cardinality << std::endl;

    // if your bitmaps have long runs, you can compress them by calling
    // run_optimize
    uint32_t size = r1.getSizeInBytes();
    r1.runOptimize();
    uint32_t compact_size = r1.getSizeInBytes();

    std::cout << "size before run optimize " << size << " bytes, and after "
              << compact_size << " bytes." << std::endl;

    // create a new bitmap with varargs
    Roaring r2 = Roaring::bitmapOf(5, 1, 2, 3, 5, 6);

    r2.printf();
    printf("\n");
    assert_true(r2.minimum() == 1);

    assert_true(r2.maximum() == 6);

    assert_true(r2.rank(4) == 3);

    // we can also create a bitmap from a pointer to 32-bit integers
    const uint32_t values[] = {2, 3, 4};
    Roaring r3(3, values);
    r3.setCopyOnWrite(copy_on_write);

    // we can also go in reverse and go from arrays to bitmaps
    uint64_t card1 = r1.cardinality();
    uint32_t *arr1 = new uint32_t[card1];
    assert_true(arr1 != NULL);
    r1.toUint32Array(arr1);
    Roaring r1f(card1, arr1);
    delete[] arr1;

    // bitmaps shall be equal
    assert_true(r1 == r1f);

    // we can copy and compare bitmaps
    Roaring z(r3);
    z.setCopyOnWrite(copy_on_write);
    assert_true(r3 == z);

    // we can compute union two-by-two
    Roaring r1_2_3 = r1 | r2;
    r1_2_3.setCopyOnWrite(copy_on_write);
    r1_2_3 |= r3;

    // we can compute a big union
    const Roaring *allmybitmaps[] = {&r1, &r2, &r3};
    Roaring bigunion = Roaring::fastunion(3, allmybitmaps);
    assert_true(r1_2_3 == bigunion);

    // we can compute intersection two-by-two
    Roaring i1_2 = r1 & r2;

    // we can write a bitmap to a pointer and recover it later

    uint32_t expectedsize = r1.getSizeInBytes();
    char *serializedbytes = new char[expectedsize];
    r1.write(serializedbytes);
    Roaring t = Roaring::read(serializedbytes);
    assert_true(r1 == t);
    delete[] serializedbytes;

    // we can iterate over all values using custom functions
    uint32_t counter = 0;
    r1.iterate(roaring_iterator_sumall, &counter);
    /**
     * void roaring_iterator_sumall(uint32_t value, void *param) {
     *        *(uint32_t *) param += value;
     *  }
     *
     */
     // we can also iterate the C++ way
     counter = 0;
     for(Roaring::const_iterator i = t.begin() ; i != t.end() ; i++) {
       ++counter;
     }
     assert_true(counter == t.cardinality());
}

void test_example_cpp_64(bool copy_on_write) {
    // create a new empty bitmap
    Roaring64Map r1;
    r1.setCopyOnWrite(copy_on_write);
    // then we can add values
    for (uint64_t i = 100; i < 1000; i++) {
        r1.add(i);
    }
    for (uint64_t i = 14000000000000000100ull; i < 14000000000000001000ull; i++) {
        r1.add(i);
    }

    // check whether a value is contained
    assert_true(r1.contains((uint64_t)14000000000000000500ull));

    // compute how many bits there are:
    uint64_t cardinality = r1.cardinality();
    std::cout << "Cardinality = " << cardinality << std::endl;

    // if your bitmaps have long runs, you can compress them by calling
    // run_optimize
    uint64_t size = r1.getSizeInBytes();
    r1.runOptimize();
    uint64_t compact_size = r1.getSizeInBytes();

    std::cout << "size before run optimize " << size << " bytes, and after "
              << compact_size << " bytes." << std::endl;

    // create a new bitmap with varargs
    Roaring64Map r2 = Roaring64Map::bitmapOf(5, 1ull, 2ull, 234294967296ull,
                                             195839473298ull, 14000000000000000100ull);

    r2.printf();
    printf("\n");
    assert_true(r2.minimum() == 1ull);

    assert_true(r2.maximum() == 14000000000000000100ull);

    assert_true(r2.rank(234294967296ull) == 4ull);

    // we can also create a bitmap from a pointer to 32-bit integers
    const uint32_t values[] = {2, 3, 4};
    Roaring64Map r3(3, values);
    r3.setCopyOnWrite(copy_on_write);

    // we can also go in reverse and go from arrays to bitmaps
    uint64_t card1 = r1.cardinality();
    uint64_t *arr1 = new uint64_t[card1];
    assert_true(arr1 != NULL);
    r1.toUint64Array(arr1);
    Roaring64Map r1f(card1, arr1);
    delete[] arr1;

    // bitmaps shall be equal
    assert_true(r1 == r1f);

    // we can copy and compare bitmaps
    Roaring64Map z(r3);
    z.setCopyOnWrite(copy_on_write);
    assert_true(r3 == z);

    // we can compute union two-by-two
    Roaring64Map r1_2_3 = r1 | r2;
    r1_2_3.setCopyOnWrite(copy_on_write);
    r1_2_3 |= r3;

    // we can compute a big union
    const Roaring64Map *allmybitmaps[] = {&r1, &r2, &r3};
    Roaring64Map bigunion = Roaring64Map::fastunion(3, allmybitmaps);
    assert_true(r1_2_3 == bigunion);

    // we can compute intersection two-by-two
    Roaring64Map i1_2 = r1 & r2;

    // we can write a bitmap to a pointer and recover it later

    uint32_t expectedsize = r1.getSizeInBytes();
    char *serializedbytes = new char[expectedsize];
    r1.write(serializedbytes);
    Roaring64Map t = Roaring64Map::read(serializedbytes);
    assert_true(r1 == t);
    delete[] serializedbytes;

    // we can iterate over all values using custom functions
    uint64_t counter = 0;
    r1.iterate(roaring_iterator_sumall64, &counter);
    /**
     * void roaring_iterator_sumall64(uint64_t value, void *param) {
     *        *(uint64_t *) param += value;
     *  }
     *
     */
     // we can also iterate the C++ way
     counter = 0;
     for(Roaring64Map::const_iterator i = t.begin() ; i != t.end() ; i++) {
       ++counter;
     }
     assert_true(counter == t.cardinality());
}

void test_example_true(void **) { test_example(true); }

void test_example_false(void **) { test_example(false); }

void test_example_cpp_true(void **) { test_example_cpp(true); }

void test_example_cpp_false(void **) { test_example_cpp(false); }

void test_example_cpp_64_true(void **) { test_example_cpp_64(true); }

void test_example_cpp_64_false(void **) { test_example_cpp_64(false); }

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_example_true),
        cmocka_unit_test(test_example_false),
        cmocka_unit_test(test_example_cpp_true),
        cmocka_unit_test(test_example_cpp_false),
        cmocka_unit_test(test_example_cpp_64_true),
        cmocka_unit_test(test_example_cpp_64_false)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}

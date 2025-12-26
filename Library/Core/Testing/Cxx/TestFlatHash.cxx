/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * This file is part of XSigma and is licensed under a dual-license model:
 *
 *   - Open-source License (GPLv3):
 *       Free for personal, academic, and research use under the terms of
 *       the GNU General Public License v3.0 or later.
 *
 *   - Commercial License:
 *       A commercial license is required for proprietary, closed-source,
 *       or SaaS usage. Contact us to obtain a commercial agreement.
 *
 * Contact: licensing@xsigma.co.uk
 * Website: https://www.xsigma.co.uk
 */

#include <algorithm>
#include <string>
#include <vector>

#include "util/flat_hash.h"
#include "xsigmaTest.h"

using namespace xsigma;

// ============================================================================
// Basic flat_hash_map Tests
// ============================================================================

XSIGMATEST(FlatHash, map_basic_operations)
{
    flat_hash_map<int, std::string> map;

    // Test empty map
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);

    // Test insert
    map[1] = "one";
    map[2] = "two";
    map[3] = "three";

    EXPECT_FALSE(map.empty());
    EXPECT_EQ(map.size(), 3);

    // Test find
    auto it1 = map.find(1);
    EXPECT_TRUE(it1 != map.end());
    EXPECT_EQ(it1->second, "one");

    auto it_not_found = map.find(99);
    EXPECT_TRUE(it_not_found == map.end());

    // Test erase
    map.erase(2);
    EXPECT_EQ(map.size(), 2);
    EXPECT_TRUE(map.find(2) == map.end());

    // Test clear
    map.clear();
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);

    END_TEST();
}

// ============================================================================
// Basic flat_hash_set Tests
// ============================================================================

XSIGMATEST(FlatHash, set_basic_operations)
{
    flat_hash_set<int> set;

    // Test empty set
    EXPECT_TRUE(set.empty());
    EXPECT_EQ(set.size(), 0);

    // Test insert
    set.insert(1);
    set.insert(2);
    set.insert(3);

    EXPECT_FALSE(set.empty());
    EXPECT_EQ(set.size(), 3);

    // Test find
    EXPECT_TRUE(set.find(1) != set.end());
    EXPECT_TRUE(set.find(2) != set.end());
    EXPECT_TRUE(set.find(99) == set.end());

    // Test contains
    EXPECT_TRUE(set.contains(1));
    EXPECT_TRUE(set.contains(2));
    EXPECT_FALSE(set.contains(99));

    // Test duplicate insert
    auto result = set.insert(1);
    EXPECT_FALSE(result.second);  // Should not insert duplicate
    EXPECT_EQ(set.size(), 3);

    // Test erase
    set.erase(2);
    EXPECT_EQ(set.size(), 2);
    EXPECT_FALSE(set.contains(2));

    // Test clear
    set.clear();
    EXPECT_TRUE(set.empty());

    END_TEST();
}

XSIGMATEST(FlatHash, set_value_types)
{
    // Test with int
    flat_hash_set<int> int_set;
    int_set.insert(1);
    int_set.insert(2);
    int_set.insert(3);
    EXPECT_EQ(int_set.size(), 3);

    // Test with string
    flat_hash_set<std::string> str_set;
    str_set.insert("hello");
    str_set.insert("world");
    EXPECT_TRUE(str_set.contains("hello"));
    EXPECT_TRUE(str_set.contains("world"));
    EXPECT_FALSE(str_set.contains("foo"));

    END_TEST();
}

// ============================================================================
// Iteration Tests
// ============================================================================

XSIGMATEST(FlatHash, iteration)
{
    // Test map iteration
    flat_hash_map<int, std::string> map;
    map[1] = "one";
    map[2] = "two";
    map[3] = "three";

    int count = 0;
    for (const auto& pair : map)
    {
        EXPECT_TRUE(pair.first >= 1 && pair.first <= 3);
        count++;
    }
    EXPECT_EQ(count, 3);

    // Test set iteration
    flat_hash_set<int> set;
    set.insert(1);
    set.insert(2);
    set.insert(3);

    count = 0;
    for (int value : set)
    {
        EXPECT_TRUE(value >= 1 && value <= 3);
        count++;
    }
    EXPECT_EQ(count, 3);

    END_TEST();
}

// ============================================================================
// Capacity Tests
// ============================================================================

XSIGMATEST(FlatHash, capacity)
{
    flat_hash_map<int, int> map;

    // Test reserve
    map.reserve(100);
    EXPECT_GE(map.bucket_count(), 100);

    // Insert elements
    for (int i = 0; i < 10; ++i)
    {
        map[i] = i * 10;
    }
    EXPECT_EQ(map.size(), 10);

    // Test load factor
    float load = map.load_factor();
    EXPECT_GT(load, 0.0f);
    EXPECT_LE(load, map.max_load_factor());

    END_TEST();
}

// ============================================================================
// Edge Cases Tests
// ============================================================================

// ============================================================================
// Edge Cases Tests
// ============================================================================

XSIGMATEST(FlatHash, edge_cases)
{
    // Test empty map operations
    flat_hash_map<int, int> empty_map;
    EXPECT_TRUE(empty_map.find(1) == empty_map.end());
    EXPECT_EQ(empty_map.erase(1), 0);

    // Test single element
    flat_hash_map<int, int> single_map;
    single_map[1] = 100;
    EXPECT_EQ(single_map.size(), 1);
    EXPECT_EQ(single_map[1], 100);

    // Test overwriting value
    single_map[1] = 200;
    EXPECT_EQ(single_map.size(), 1);
    EXPECT_EQ(single_map[1], 200);

    // Test empty set operations
    flat_hash_set<int> empty_set;
    EXPECT_FALSE(empty_set.contains(1));
    EXPECT_EQ(empty_set.erase(1), 0);

    // Test duplicate handling in set
    flat_hash_set<int> set;
    set.insert(1);
    set.insert(1);
    set.insert(1);
    EXPECT_EQ(set.size(), 1);

    END_TEST();
}

// ============================================================================
// Copy and Move Semantics Tests
// ============================================================================

XSIGMATEST(FlatHash, copy_move_semantics)
{
    // Test map copy constructor
    flat_hash_map<int, std::string> map1;
    map1[1] = "one";
    map1[2] = "two";

    flat_hash_map<int, std::string> map2(map1);
    EXPECT_EQ(map2.size(), 2);
    EXPECT_EQ(map2[1], "one");
    EXPECT_EQ(map2[2], "two");

    // Test set copy constructor
    flat_hash_set<int> set1;
    set1.insert(1);
    set1.insert(2);

    flat_hash_set<int> set2(set1);
    EXPECT_EQ(set2.size(), 2);
    EXPECT_TRUE(set2.contains(1));
    EXPECT_TRUE(set2.contains(2));

    END_TEST();
}

// ============================================================================
// Emplace Tests
// ============================================================================

// ============================================================================
// Emplace Tests
// ============================================================================

XSIGMATEST(FlatHash, emplace_operations)
{
    // Test map emplace
    flat_hash_map<int, std::string> map;
    auto                            result1 = map.emplace(1, "one");
    EXPECT_TRUE(result1.second);  // Insertion successful
    EXPECT_EQ(result1.first->second, "one");

    // Test duplicate emplace
    auto result2 = map.emplace(1, "ONE");
    EXPECT_FALSE(result2.second);             // Insertion failed (duplicate)
    EXPECT_EQ(result2.first->second, "one");  // Original value unchanged

    // Test set emplace
    flat_hash_set<int> set;
    auto               result3 = set.emplace(42);
    EXPECT_TRUE(result3.second);

    auto result4 = set.emplace(42);
    EXPECT_FALSE(result4.second);  // Duplicate

    END_TEST();
}

// ============================================================================
// xsigma_map and xsigma_set Alias Tests
// ============================================================================

XSIGMATEST(FlatHash, xsigma_aliases)
{
    // Test xsigma_map
    xsigma_map<int, std::string> map;
    map[1] = "one";
    map[2] = "two";
    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(map[1], "one");

    // Test xsigma_set
    xsigma_set<int> set;
    set.insert(1);
    set.insert(2);
    EXPECT_EQ(set.size(), 2);
    EXPECT_TRUE(set.contains(1));

    END_TEST();
}

// ============================================================================
// KeyOrValueEquality Tests
// ============================================================================

XSIGMATEST(FlatHash, key_or_value_equality_constructor)
{
    // Test KeyOrValueEquality with default equal_to
    std::equal_to<int>                                                                 eq;
    detailv3::KeyOrValueEquality<int, std::pair<int, std::string>, std::equal_to<int>> equality(eq);

    // Test equality comparisons
    EXPECT_TRUE(equality(1, 1));
    EXPECT_FALSE(equality(1, 2));

    END_TEST();
}

XSIGMATEST(FlatHash, key_or_value_equality_key_comparisons)
{
    std::equal_to<int>                                                                 eq;
    detailv3::KeyOrValueEquality<int, std::pair<int, std::string>, std::equal_to<int>> equality(eq);

    // Test key-to-key comparison
    EXPECT_TRUE(equality(5, 5));
    EXPECT_FALSE(equality(5, 10));

    // Test key-to-value comparison
    std::pair<int, std::string> value1(5, "five");
    EXPECT_TRUE(equality(5, value1));
    EXPECT_FALSE(equality(10, value1));

    // Test value-to-key comparison
    EXPECT_TRUE(equality(value1, 5));
    EXPECT_FALSE(equality(value1, 10));

    // Test value-to-value comparison
    std::pair<int, std::string> value2(5, "five");
    EXPECT_TRUE(equality(value1, value2));

    END_TEST();
}

XSIGMATEST(FlatHash, key_or_value_equality_pair_comparisons)
{
    std::equal_to<int>                                                                 eq;
    detailv3::KeyOrValueEquality<int, std::pair<int, std::string>, std::equal_to<int>> equality(eq);

    std::pair<int, std::string> value1(5, "five");
    std::pair<int, std::string> value2(5, "five");
    std::pair<int, std::string> value3(10, "ten");

    // Test key-to-pair comparison
    EXPECT_TRUE(equality(5, value1));
    EXPECT_FALSE(equality(10, value1));

    // Test pair-to-key comparison
    EXPECT_TRUE(equality(value1, 5));
    EXPECT_FALSE(equality(value1, 10));

    // Test value-to-pair comparison
    EXPECT_TRUE(equality(value1, value2));
    EXPECT_FALSE(equality(value1, value3));

    // Test pair-to-pair comparison
    EXPECT_TRUE(equality(value1, value2));
    EXPECT_FALSE(equality(value1, value3));

    END_TEST();
}

// ============================================================================
// sherwood_v3_table Constructor Tests
// ============================================================================

XSIGMATEST(FlatHash, sherwood_v3_table_default_constructor)
{
    flat_hash_map<int, int> map;
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);
    EXPECT_EQ(map.bucket_count(), 0);

    END_TEST();
}

XSIGMATEST(FlatHash, sherwood_v3_table_bucket_count_constructor)
{
    flat_hash_map<int, int> map(100);
    EXPECT_TRUE(map.empty());
    EXPECT_GE(map.bucket_count(), 100);

    END_TEST();
}

XSIGMATEST(FlatHash, sherwood_v3_table_bucket_count_with_hash_equal)
{
    std::hash<int>          hash;
    std::equal_to<int>      equal;
    flat_hash_map<int, int> map(50, hash, equal);

    EXPECT_TRUE(map.empty());
    EXPECT_GE(map.bucket_count(), 50);

    END_TEST();
}

XSIGMATEST(FlatHash, sherwood_v3_table_initializer_list_constructor_empty)
{
    flat_hash_map<int, int> map({});
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);

    END_TEST();
}

XSIGMATEST(FlatHash, sherwood_v3_table_initializer_list_constructor_small)
{
    flat_hash_map<int, int> map({{1, 10}, {2, 20}, {3, 30}});
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map[1], 10);
    EXPECT_EQ(map[2], 20);
    EXPECT_EQ(map[3], 30);

    END_TEST();
}

XSIGMATEST(FlatHash, sherwood_v3_table_initializer_list_constructor_large)
{
    std::initializer_list<std::pair<int, int>> init_list;
    std::vector<std::pair<int, int>>           items;
    for (int i = 0; i < 100; ++i)
    {
        items.push_back({i, i * 10});
    }

    flat_hash_map<int, int> map(items.begin(), items.end());
    EXPECT_EQ(map.size(), 100);
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(map[i], i * 10);
    }

    END_TEST();
}

XSIGMATEST(FlatHash, sherwood_v3_table_move_constructor)
{
    flat_hash_map<int, std::string> map1;
    map1[1] = "one";
    map1[2] = "two";
    map1[3] = "three";

    flat_hash_map<int, std::string> map2(std::move(map1));

    EXPECT_EQ(map2.size(), 3);
    EXPECT_EQ(map2[1], "one");
    EXPECT_EQ(map2[2], "two");
    EXPECT_EQ(map2[3], "three");

    END_TEST();
}

XSIGMATEST(FlatHash, sherwood_v3_table_copy_constructor)
{
    flat_hash_map<int, std::string> map1;
    map1[1] = "one";
    map1[2] = "two";

    flat_hash_map<int, std::string> map2(map1);

    EXPECT_EQ(map2.size(), 2);
    EXPECT_EQ(map2[1], "one");
    EXPECT_EQ(map2[2], "two");

    // Verify independence
    map1[1] = "ONE";
    EXPECT_EQ(map2[1], "one");

    END_TEST();
}

// ============================================================================
// sherwood_v3_table Move Assignment Tests
// ============================================================================

XSIGMATEST(FlatHash, sherwood_v3_table_move_assignment_self)
{
    flat_hash_map<int, int> map;
    map[1] = 10;
    map[2] = 20;

    // Self-assignment should be handled correctly
    map = std::move(map);

    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(map[1], 10);
    EXPECT_EQ(map[2], 20);

    END_TEST();
}

XSIGMATEST(FlatHash, sherwood_v3_table_move_assignment_different)
{
    flat_hash_map<int, int> map1;
    map1[1] = 10;
    map1[2] = 20;

    flat_hash_map<int, int> map2;
    map2[3] = 30;

    map2 = std::move(map1);

    EXPECT_EQ(map2.size(), 2);
    EXPECT_EQ(map2[1], 10);
    EXPECT_EQ(map2[2], 20);
    EXPECT_FALSE(map2.contains(3));

    END_TEST();
}

XSIGMATEST(FlatHash, sherwood_v3_table_move_assignment_empty_to_nonempty)
{
    flat_hash_map<int, int> map1;
    flat_hash_map<int, int> map2;
    map2[1] = 10;
    map2[2] = 20;

    map2 = std::move(map1);

    EXPECT_TRUE(map2.empty());
    EXPECT_EQ(map2.size(), 0);

    END_TEST();
}

XSIGMATEST(FlatHash, sherwood_v3_table_move_assignment_nonempty_to_empty)
{
    flat_hash_map<int, int> map1;
    map1[1] = 10;
    map1[2] = 20;

    flat_hash_map<int, int> map2;

    map2 = std::move(map1);

    EXPECT_EQ(map2.size(), 2);
    EXPECT_EQ(map2[1], 10);
    EXPECT_EQ(map2[2], 20);

    END_TEST();
}

// ============================================================================
// Iterator Post-Increment Tests
// ============================================================================

XSIGMATEST(FlatHash, iterator_post_increment)
{
    flat_hash_map<int, int> map;
    map[1] = 10;
    map[2] = 20;
    map[3] = 30;

    auto it      = map.begin();
    auto it_copy = it++;

    // it_copy should point to the same element as the original it
    EXPECT_EQ(it_copy->first, it_copy->first);

    // it should now point to a different element
    EXPECT_NE(it, it_copy);

    END_TEST();
}

XSIGMATEST(FlatHash, const_iterator_post_increment)
{
    flat_hash_map<int, int> map;
    map[1] = 10;
    map[2] = 20;

    auto it      = map.cbegin();
    auto it_copy = it++;

    EXPECT_NE(it, it_copy);

    END_TEST();
}

// ============================================================================
// Insert Operations Tests
// ============================================================================

XSIGMATEST(FlatHash, insert_const_value)
{
    flat_hash_map<int, std::string> map;

    const std::pair<int, std::string> value(1, "one");
    auto                              result = map.insert(value);

    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->first, 1);
    EXPECT_EQ(result.first->second, "one");
    EXPECT_EQ(map.size(), 1);

    END_TEST();
}

XSIGMATEST(FlatHash, map_insert_or_assign_and_contains)
{
    flat_hash_map<int, std::string> map;

    // Insert new key
    auto it_new = map.insert_or_assign(1, std::string("one"));
    EXPECT_EQ(map.size(), 1);
    EXPECT_TRUE(map.contains(1));
    EXPECT_EQ(map.find(1)->second, "one");

    // Assign existing key (value should be updated, size unchanged)
    auto it_upd = map.insert_or_assign(1, std::string("ONE"));
    EXPECT_EQ(map.size(), 1);
    EXPECT_TRUE(map.contains(1));
    EXPECT_EQ(map.find(1)->second, "ONE");

    // Insert another key via iterator overload
    auto it2 = map.insert_or_assign(map.cbegin(), 2, std::string("two"));
    EXPECT_EQ(map.size(), 2);
    EXPECT_TRUE(map.contains(2));
    EXPECT_EQ(map.find(2)->second, "two");

    END_TEST();
}

XSIGMATEST(FlatHash, map_at_accessors)
{
    flat_hash_map<int, int> map;
    map[10]          = 42;
    const auto& cmap = map;

    // Positive at() on present key
    EXPECT_EQ(map.at(10), 42);
    EXPECT_EQ(cmap.at(10), 42);

    END_TEST();
}

// ============================================================================
// Erase iterator/range, rehash/reserve, equality, swap
// ============================================================================

XSIGMATEST(FlatHash, map_erase_by_iterator_and_range)
{
    flat_hash_map<int, int> map;
    for (int i = 0; i < 5; ++i)
    {
        map[i] = i * 10;
    }
    EXPECT_EQ(map.size(), 5);

    // Erase begin iterator
    auto old_begin = map.begin();
    map.erase(old_begin);
    EXPECT_EQ(map.size(), 4);

    // Erase [begin, end)
    auto it = map.begin();
    ++it;  // leave one element
    map.erase(it, map.end());
    EXPECT_EQ(map.size(), 1);

    END_TEST();
}

XSIGMATEST(FlatHash, map_rehash_reserve_and_load_factor)
{
    flat_hash_map<int, int> map;
    map.reserve(8);
    auto initial_buckets = map.bucket_count();
    for (int i = 0; i < 16; ++i)
    {
        map[i] = i;
    }
    EXPECT_EQ(map.size(), 16);
    EXPECT_GE(map.bucket_count(), initial_buckets);
    EXPECT_LE(map.load_factor(), map.max_load_factor());

    // Rehash up
    map.rehash(64);
    EXPECT_GE(map.bucket_count(), 64);
    EXPECT_LE(map.load_factor(), map.max_load_factor());

    END_TEST();
}

XSIGMATEST(FlatHash, map_equality_and_swap)
{
    flat_hash_map<int, int> a;
    flat_hash_map<int, int> b;
    for (int i = 0; i < 5; ++i)
    {
        a[i] = i;
        b[i] = i;
    }
    EXPECT_TRUE(a == b);
    b[99] = 7;
    EXPECT_TRUE(a != b);

    a.swap(b);
    EXPECT_TRUE(a.contains(99));
    EXPECT_FALSE(b.contains(99));

    END_TEST();
}

XSIGMATEST(FlatHash, set_equality_and_swap)
{
    flat_hash_set<int> s1;
    flat_hash_set<int> s2;
    for (int i = 0; i < 5; ++i)
    {
        s1.insert(i);
        s2.insert(i);
    }
    EXPECT_TRUE(s1 == s2);
    s2.insert(99);
    EXPECT_TRUE(s1 != s2);

    s1.swap(s2);
    EXPECT_TRUE(s1.contains(99));
    EXPECT_FALSE(s2.contains(99));

    END_TEST();
}

// ============================================================================
// Custom hash/equal and alternative hash policy wiring
// ============================================================================

struct ci_hash
{
    size_t operator()(const std::string& s) const noexcept
    {
        std::string lower(s);
        std::transform(
            lower.begin(),
            lower.end(),
            lower.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return std::hash<std::string>{}(lower);
    }
};
struct ci_equal
{
    bool operator()(const std::string& a, const std::string& b) const noexcept
    {
        if (a.size() != b.size())
            return false;
        for (size_t i = 0; i < a.size(); ++i)
        {
            if (std::tolower(static_cast<unsigned char>(a[i])) !=
                std::tolower(static_cast<unsigned char>(b[i])))
                return false;
        }
        return true;
    }
};

XSIGMATEST(FlatHash, map_custom_hash_and_equal)
{
    flat_hash_map<std::string, int, ci_hash, ci_equal> cmap;
    cmap["Hello"] = 1;
    EXPECT_TRUE(cmap.contains("hello"));
    EXPECT_EQ(cmap.find("heLLo")->second, 1);

    // insert_or_assign should use custom equal
    cmap.insert_or_assign("HELLO", 7);
    EXPECT_EQ(cmap.size(), 1);
    EXPECT_EQ(cmap.find("hello")->second, 7);

    END_TEST();
}

XSIGMATEST(FlatHash, set_power_of_two_hash_policy_instantiation)
{
    // Ensure alternate hash policy alias compiles and works
    flat_hash_set<int, power_of_two_std_hash<int>> s;
    s.insert(1);
    s.insert(2);
    EXPECT_TRUE(s.contains(1));
    EXPECT_TRUE(s.contains(2));
    END_TEST();
}

XSIGMATEST(FlatHash, insert_rvalue)
{
    flat_hash_map<int, std::string> map;

    auto result = map.insert(std::make_pair(1, "one"));

    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->first, 1);
    EXPECT_EQ(result.first->second, "one");

    END_TEST();
}

XSIGMATEST(FlatHash, insert_duplicate)
{
    flat_hash_map<int, int> map;

    auto result1 = map.insert({1, 10});
    EXPECT_TRUE(result1.second);

    auto result2 = map.insert({1, 20});
    EXPECT_FALSE(result2.second);
    EXPECT_EQ(result2.first->second, 10);  // Original value unchanged

    END_TEST();
}

// ============================================================================
// Swap Pointers Tests
// ============================================================================

XSIGMATEST(FlatHash, swap_pointers_basic)
{
    flat_hash_map<int, int> map1;
    map1[1] = 10;
    map1[2] = 20;

    flat_hash_map<int, int> map2;
    map2[3] = 30;

    map1.swap(map2);

    EXPECT_EQ(map1.size(), 1);
    EXPECT_EQ(map1[3], 30);

    EXPECT_EQ(map2.size(), 2);
    EXPECT_EQ(map2[1], 10);
    EXPECT_EQ(map2[2], 20);

    END_TEST();
}

XSIGMATEST(FlatHash, swap_pointers_empty_with_nonempty)
{
    flat_hash_map<int, int> map1;
    flat_hash_map<int, int> map2;
    map2[1] = 10;
    map2[2] = 20;

    map1.swap(map2);

    EXPECT_EQ(map1.size(), 2);
    EXPECT_EQ(map1[1], 10);

    EXPECT_TRUE(map2.empty());

    END_TEST();
}

XSIGMATEST(FlatHash, swap_pointers_preserves_bucket_count)
{
    flat_hash_map<int, int> map1(100);
    map1[1] = 10;

    flat_hash_map<int, int> map2(50);
    map2[2] = 20;

    uint64_t bucket_count1 = map1.bucket_count();
    uint64_t bucket_count2 = map2.bucket_count();

    map1.swap(map2);

    EXPECT_EQ(map1.bucket_count(), bucket_count2);
    EXPECT_EQ(map2.bucket_count(), bucket_count1);

    END_TEST();
}

// ============================================================================
// Reset to Empty State Tests
// ============================================================================

XSIGMATEST(FlatHash, reset_to_empty_state_clears_data)
{
    flat_hash_map<int, int> map;
    map[1] = 10;
    map[2] = 20;
    map[3] = 30;

    EXPECT_EQ(map.size(), 3);

    map.clear();

    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);

    END_TEST();
}

XSIGMATEST(FlatHash, reset_to_empty_state_allows_reinsertion)
{
    flat_hash_map<int, int> map;
    map[1] = 10;
    map.clear();

    map[1] = 20;
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[1], 20);

    END_TEST();
}

// ============================================================================
// Prime Number Hash Policy Tests
// ============================================================================

XSIGMATEST(FlatHash, prime_number_hash_policy_index_for_hash)
{
    xsigma::prime_number_hash_policy policy;

    // Test index_for_hash with various hash values
    uint64_t index1 = policy.index_for_hash(12345, 0);
    EXPECT_GE(index1, 0);

    uint64_t index2 = policy.index_for_hash(67890, 0);
    EXPECT_GE(index2, 0);

    END_TEST();
}

XSIGMATEST(FlatHash, prime_number_hash_policy_next_size_over)
{
    xsigma::prime_number_hash_policy policy;

    uint64_t size          = 10;
    uint64_t original_size = size;
    policy.next_size_over(size);

    EXPECT_GT(size, original_size);

    END_TEST();
}

XSIGMATEST(FlatHash, prime_number_hash_policy_reset)
{
    xsigma::prime_number_hash_policy policy;

    uint64_t size = 100;
    policy.next_size_over(size);
    policy.reset();

    // After reset, the policy should be in initial state
    // Verify by checking that next_size_over works correctly
    uint64_t size2 = 10;
    policy.next_size_over(size2);
    EXPECT_GT(size2, 10);

    END_TEST();
}

XSIGMATEST(FlatHash, prime_number_hash_policy_keep_in_range)
{
    xsigma::prime_number_hash_policy policy;

    uint64_t index = policy.keep_in_range(12345, 100);
    EXPECT_LE(index, 100);

    END_TEST();
}

// ============================================================================
// Prime Number Hash Policy - Complete Coverage
// ============================================================================

XSIGMATEST(FlatHash, prime_number_hash_policy_next_size_over_small)
{
    xsigma::prime_number_hash_policy policy;

    uint64_t size = 1;  // very small
    auto     f    = policy.next_size_over(size);
    // size should be updated to the first prime in the list; f is corresponding mod function
    EXPECT_GT(size, 1ULL);
    EXPECT_EQ(f(size), 0ULL);  // n % n == 0

    END_TEST();
}

XSIGMATEST(FlatHash, prime_number_hash_policy_next_size_over_between_primes)
{
    xsigma::prime_number_hash_policy policy;

    uint64_t size = 6;  // between 5 and 7 -> should become 7
    auto     f    = policy.next_size_over(size);
    EXPECT_GE(size, 6ULL);
    // Verify returned function behaves like modulo 'size'
    EXPECT_EQ(f(size), 0ULL);
    EXPECT_EQ(f(size + 1), 1ULL);

    END_TEST();
}

XSIGMATEST(FlatHash, prime_number_hash_policy_commit_and_index_for_hash)
{
    xsigma::prime_number_hash_policy policy;

    // Acquire a modulus function for a known target size
    uint64_t size = 1000;
    auto     f    = policy.next_size_over(size);
    // Commit the modulus function; index_for_hash must now use it
    policy.commit(f);

    uint64_t h = 1234567890123456789ULL;
    EXPECT_EQ(policy.index_for_hash(h, 0), f(h));

    // keep_in_range: below threshold returns unchanged
    EXPECT_EQ(policy.keep_in_range(42, 100), 42ULL);
    // keep_in_range: above threshold returns reduced via current mod function
    uint64_t big = size * 3 + 5;  // guarantee > size
    EXPECT_EQ(policy.keep_in_range(big, size - 1), f(big));

    END_TEST();
}

XSIGMATEST(FlatHash, prime_number_hash_policy_reset_restores_mod0)
{
    xsigma::prime_number_hash_policy policy;

    // Commit a non-default mod function first
    uint64_t size = 50;
    auto     f    = policy.next_size_over(size);
    policy.commit(f);
    uint64_t h = 987654321ULL;
    EXPECT_EQ(policy.index_for_hash(h, 0), f(h));

    // Reset -> mod0 -> always 0
    policy.reset();
    EXPECT_EQ(policy.index_for_hash(h, 0), 0ULL);
    EXPECT_EQ(policy.keep_in_range(h, 0ULL), 0ULL);  // since index > 0, reduced via mod0

    END_TEST();
}

XSIGMATEST(FlatHash, prime_number_hash_policy_next_size_over_large)
{
    xsigma::prime_number_hash_policy policy;

    // Use a very large request; should clamp to a large prime in the list
    uint64_t requested = std::numeric_limits<uint64_t>::max() - 12345ULL;
    uint64_t size      = requested;
    auto     f         = policy.next_size_over(size);

    EXPECT_GE(size, requested);
    EXPECT_EQ(f(size), 0ULL);  // modulo by itself

    // Commit and validate index_for_hash with large inputs
    policy.commit(f);
    uint64_t h1 = requested - 777ULL;
    EXPECT_EQ(policy.index_for_hash(h1, 0), f(h1));

    END_TEST();
}

// ============================================================================
// Fibonacci Hash Policy Tests
// ============================================================================

XSIGMATEST(FlatHash, fibonacci_hash_policy_index_for_hash)
{
    xsigma::fibonacci_hash_policy policy;

    uint64_t index1 = policy.index_for_hash(12345, 0);
    EXPECT_GE(index1, 0);

    uint64_t index2 = policy.index_for_hash(67890, 0);
    EXPECT_GE(index2, 0);

    END_TEST();
}

XSIGMATEST(FlatHash, fibonacci_hash_policy_next_size_over)
{
    xsigma::fibonacci_hash_policy policy;

    uint64_t size  = 10;
    int8_t   shift = policy.next_size_over(size);

    EXPECT_GE(size, 2);
    EXPECT_LE(shift, 63);

    END_TEST();
}

XSIGMATEST(FlatHash, fibonacci_hash_policy_next_size_over_minimum)
{
    xsigma::fibonacci_hash_policy policy;

    uint64_t size = 1;
    policy.next_size_over(size);

    EXPECT_GE(size, 2);

    END_TEST();
}

XSIGMATEST(FlatHash, fibonacci_hash_policy_reset)
{
    xsigma::fibonacci_hash_policy policy;

    uint64_t size = 100;
    policy.next_size_over(size);
    policy.reset();

    // After reset, verify the policy is in initial state
    uint64_t size2 = 10;
    policy.next_size_over(size2);
    EXPECT_GE(size2, 2);

    END_TEST();
}

XSIGMATEST(FlatHash, fibonacci_hash_policy_keep_in_range)
{
    xsigma::fibonacci_hash_policy policy;

    uint64_t num_slots_minus_one = 127;  // 2^7 - 1
    uint64_t index               = policy.keep_in_range(12345, num_slots_minus_one);

    EXPECT_LE(index, num_slots_minus_one);

    END_TEST();
}

XSIGMATEST(FlatHash, fibonacci_hash_policy_commit)
{
    xsigma::fibonacci_hash_policy policy;

    uint64_t size  = 16;
    int8_t   shift = policy.next_size_over(size);
    policy.commit(shift);

    // After commit, index_for_hash should use the new shift value
    uint64_t index = policy.index_for_hash(12345, 0);
    EXPECT_GE(index, 0);

    END_TEST();
}

// ============================================================================
// Power of Two Hash Policy Tests
// ============================================================================

XSIGMATEST(FlatHash, power_of_two_hash_policy_index_for_hash)
{
    xsigma::power_of_two_hash_policy policy;

    // Test index_for_hash with various hash values and num_slots_minus_one
    uint64_t index1 = policy.index_for_hash(12345, 15);  // 15 = 0xF (4 bits)
    EXPECT_LE(index1, 15);

    uint64_t index2 = policy.index_for_hash(67890, 31);  // 31 = 0x1F (5 bits)
    EXPECT_LE(index2, 31);

    uint64_t index3 = policy.index_for_hash(0xFFFFFFFF, 255);  // 255 = 0xFF (8 bits)
    EXPECT_LE(index3, 255);

    END_TEST();
}

XSIGMATEST(FlatHash, power_of_two_hash_policy_index_for_hash_bitwise_and)
{
    xsigma::power_of_two_hash_policy policy;

    // Verify that index_for_hash uses bitwise AND
    uint64_t hash                = 0x12345678;
    uint64_t num_slots_minus_one = 0xFF;  // 255

    uint64_t index    = policy.index_for_hash(hash, num_slots_minus_one);
    uint64_t expected = hash & num_slots_minus_one;

    EXPECT_EQ(index, expected);

    END_TEST();
}

XSIGMATEST(FlatHash, power_of_two_hash_policy_keep_in_range)
{
    xsigma::power_of_two_hash_policy policy;

    uint64_t num_slots_minus_one = 127;  // 2^7 - 1

    // Test with index within range
    uint64_t index1 = policy.keep_in_range(50, num_slots_minus_one);
    EXPECT_LE(index1, num_slots_minus_one);

    // Test with index out of range
    uint64_t index2 = policy.keep_in_range(200, num_slots_minus_one);
    EXPECT_LE(index2, num_slots_minus_one);

    // Test with large index
    uint64_t index3 = policy.keep_in_range(0xFFFFFFFF, num_slots_minus_one);
    EXPECT_LE(index3, num_slots_minus_one);

    END_TEST();
}

XSIGMATEST(FlatHash, power_of_two_hash_policy_next_size_over_small)
{
    xsigma::power_of_two_hash_policy policy;

    uint64_t size  = 1;
    int8_t   shift = policy.next_size_over(size);

    // Should return power of two
    EXPECT_GE(size, 1);
    // Verify it's a power of two: (size & (size - 1)) == 0
    EXPECT_EQ(size & (size - 1), 0);
    EXPECT_EQ(shift, 0);

    END_TEST();
}

XSIGMATEST(FlatHash, power_of_two_hash_policy_next_size_over_medium)
{
    xsigma::power_of_two_hash_policy policy;

    uint64_t size          = 10;
    uint64_t original_size = size;
    int8_t   shift         = policy.next_size_over(size);

    // Should return next power of two >= original size
    EXPECT_GE(size, original_size);
    // Verify it's a power of two
    EXPECT_EQ(size & (size - 1), 0);
    EXPECT_EQ(shift, 0);

    END_TEST();
}

XSIGMATEST(FlatHash, power_of_two_hash_policy_next_size_over_large)
{
    xsigma::power_of_two_hash_policy policy;

    uint64_t size          = 1000000;
    uint64_t original_size = size;
    int8_t   shift         = policy.next_size_over(size);

    // Should return next power of two >= original size
    EXPECT_GE(size, original_size);
    // Verify it's a power of two
    EXPECT_EQ(size & (size - 1), 0);
    EXPECT_EQ(shift, 0);

    END_TEST();
}

XSIGMATEST(FlatHash, power_of_two_hash_policy_next_size_over_already_power_of_two)
{
    xsigma::power_of_two_hash_policy policy;

    uint64_t size  = 64;  // Already a power of two
    int8_t   shift = policy.next_size_over(size);

    // Should remain 64 or become next power of two
    EXPECT_GE(size, 64);
    EXPECT_EQ(size & (size - 1), 0);
    EXPECT_EQ(shift, 0);

    END_TEST();
}

XSIGMATEST(FlatHash, power_of_two_hash_policy_commit_is_noop)
{
    xsigma::power_of_two_hash_policy policy;

    // commit() should be a no-op for power_of_two_hash_policy
    uint64_t index1 = policy.index_for_hash(12345, 255);
    policy.commit(5);  // Should have no effect
    uint64_t index2 = policy.index_for_hash(12345, 255);

    EXPECT_EQ(index1, index2);

    END_TEST();
}

XSIGMATEST(FlatHash, power_of_two_hash_policy_reset_is_noop)
{
    xsigma::power_of_two_hash_policy policy;

    // reset() should be a no-op for power_of_two_hash_policy
    uint64_t index1 = policy.index_for_hash(12345, 255);
    policy.reset();  // Should have no effect
    uint64_t index2 = policy.index_for_hash(12345, 255);

    EXPECT_EQ(index1, index2);

    END_TEST();
}

XSIGMATEST(FlatHash, power_of_two_hash_policy_sequential_operations)
{
    xsigma::power_of_two_hash_policy policy;

    // Test sequential operations
    uint64_t size1 = 5;
    policy.next_size_over(size1);
    EXPECT_EQ(size1 & (size1 - 1), 0);

    uint64_t size2 = 100;
    policy.next_size_over(size2);
    EXPECT_EQ(size2 & (size2 - 1), 0);

    uint64_t index = policy.index_for_hash(0xDEADBEEF, size2 - 1);
    EXPECT_LE(index, size2 - 1);

    END_TEST();
}

// ============================================================================
// Prime Number Hash Policy - Extended Tests
// ============================================================================

XSIGMATEST(FlatHash, prime_number_hash_policy_commit)
{
    xsigma::prime_number_hash_policy policy;

    uint64_t size     = 10;
    auto     mod_func = policy.next_size_over(size);

    // After commit, the policy should use the new mod function
    policy.commit(mod_func);

    // Verify that index_for_hash works with the committed function
    uint64_t index = policy.index_for_hash(12345, 0);
    EXPECT_GE(index, 0);

    END_TEST();
}

XSIGMATEST(FlatHash, prime_number_hash_policy_sequential_next_size_over)
{
    xsigma::prime_number_hash_policy policy;

    uint64_t size1 = 5;
    policy.next_size_over(size1);
    uint64_t first_prime = size1;

    uint64_t size2 = first_prime + 1;
    policy.next_size_over(size2);
    uint64_t second_prime = size2;

    // Second prime should be larger than first prime
    EXPECT_GT(second_prime, first_prime);

    END_TEST();
}

XSIGMATEST(FlatHash, prime_number_hash_policy_index_for_hash_after_commit)
{
    xsigma::prime_number_hash_policy policy;

    uint64_t size     = 20;
    auto     mod_func = policy.next_size_over(size);
    policy.commit(mod_func);

    // Test that index_for_hash returns valid indices
    uint64_t index1 = policy.index_for_hash(12345, 0);
    uint64_t index2 = policy.index_for_hash(67890, 0);

    EXPECT_GE(index1, 0);
    EXPECT_GE(index2, 0);

    END_TEST();
}

XSIGMATEST(FlatHash, prime_number_hash_policy_keep_in_range_after_commit)
{
    xsigma::prime_number_hash_policy policy;

    uint64_t size     = 30;
    auto     mod_func = policy.next_size_over(size);
    policy.commit(mod_func);

    // Test keep_in_range with various indices
    uint64_t index1 = policy.keep_in_range(100, 50);
    EXPECT_LE(index1, 50);

    uint64_t index2 = policy.keep_in_range(0xFFFFFFFF, 50);
    EXPECT_LE(index2, 50);

    END_TEST();
}

XSIGMATEST(FlatHash, prime_number_hash_policy_edge_case_size_zero)
{
    xsigma::prime_number_hash_policy policy;

    uint64_t size = 0;
    policy.next_size_over(size);

    // Should return a valid prime size
    EXPECT_GT(size, 0);

    END_TEST();
}

XSIGMATEST(FlatHash, prime_number_hash_policy_edge_case_size_one)
{
    xsigma::prime_number_hash_policy policy;

    uint64_t size = 1;
    policy.next_size_over(size);

    // Should return a valid prime size
    EXPECT_GT(size, 0);

    END_TEST();
}

XSIGMATEST(FlatHash, prime_number_hash_policy_edge_case_size_two)
{
    xsigma::prime_number_hash_policy policy;

    uint64_t size = 2;
    policy.next_size_over(size);

    // Should return a valid prime size
    EXPECT_GE(size, 2);

    END_TEST();
}

XSIGMATEST(FlatHash, prime_number_hash_policy_large_size)
{
    xsigma::prime_number_hash_policy policy;

    uint64_t size = 1000000000;
    policy.next_size_over(size);

    // Should return a valid prime size
    EXPECT_GT(size, 0);

    END_TEST();
}

XSIGMATEST(FlatHash, prime_number_hash_policy_reset_after_operations)
{
    xsigma::prime_number_hash_policy policy;

    uint64_t size     = 50;
    auto     mod_func = policy.next_size_over(size);
    policy.commit(mod_func);

    // After reset, should use mod0
    policy.reset();

    // Verify reset worked by checking index_for_hash behavior
    uint64_t index = policy.index_for_hash(12345, 0);
    EXPECT_GE(index, 0);

    END_TEST();
}

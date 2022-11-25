#include <gtest/gtest.h>

#include "melon/data_structures/static_map.hpp"
#include "melon/static_digraph.hpp"

#include "ranges_test_helper.hpp"

using namespace fhamonic::melon;

GTEST_TEST(static_map, empty_constructor) {
    static_map<std::size_t, int> map;
    ASSERT_EQ(map.size(), 0);
    ASSERT_EQ(map.begin(), map.end());
    ASSERT_EQ(std::as_const(map).begin(), std::as_const(map).end());
}

GTEST_TEST(static_map, size_constructor) {
    static_map<std::size_t, int> map(0);
    ASSERT_EQ(map.size(), 0);
    ASSERT_EQ(map.begin(), map.end());
    ASSERT_EQ(std::as_const(map).begin(), std::as_const(map).end());

    static_map<std::size_t, int> map2(5);
    ASSERT_EQ(map2.size(), 5);
    ASSERT_NE(map2.begin(), map.end());
    ASSERT_NE(std::as_const(map2).begin(), std::as_const(map).end());
}

GTEST_TEST(static_map, size_init_constructor) {
    static_map<std::size_t, int> map(0, 0);
    ASSERT_EQ(map.size(), 0);
    ASSERT_EQ(map.begin(), map.end());
    ASSERT_EQ(std::as_const(map).begin(), std::as_const(map).end());

    static_map<std::size_t, int> map2(5, 113);
    ASSERT_EQ(map2.size(), 5);
    ASSERT_NE(map2.begin(), map.end());
    ASSERT_NE(std::as_const(map2).begin(), std::as_const(map).end());

    auto test = [](const int & a) { return a == 113; };
    // ASSERT_TRUE(std::ranges::all_of(std::views::values(map2), test));
    ASSERT_TRUE(
        std::ranges::all_of(std::views::values(std::as_const(map2)), test));
}

GTEST_TEST(static_map, range_constructor) {
    static_map<std::size_t, int> map(0, 0);
    ASSERT_EQ(map.size(), 0);
    ASSERT_EQ(map.begin(), map.end());
    ASSERT_EQ(std::as_const(map).begin(), std::as_const(map).end());

    std::vector<int> datas = {0, 7, 3, 5, 6, 11};
    static_map<std::size_t, int> map2(datas.begin(), datas.end());
    ASSERT_EQ(map2.size(), datas.size());
    ASSERT_NE(map2.begin(), map.end());
    ASSERT_NE(std::as_const(map2).begin(), std::as_const(map).end());

    ASSERT_TRUE(std::ranges::equal(std::views::values(map2), datas));
}

GTEST_TEST(static_map, accessor) {
    std::vector<int> datas = {0, 7, 3, 5, 6, 11};
    static_map<std::size_t, int> map(datas.begin(), datas.end());
    for(std::size_t i = 0; i < map.size(); ++i) {
        ASSERT_EQ(map[i], datas[i]);
    }
}

GTEST_TEST(static_map, resize) {
    static_map<std::size_t, int> map(20);
    map.resize(10);
    ASSERT_EQ(map.size(), 10);
}

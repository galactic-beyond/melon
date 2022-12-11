#include <gtest/gtest.h>

#include "melon/concepts/graph.hpp"
#include "melon/static_digraph.hpp"

#include "ranges_test_helper.hpp"

using namespace fhamonic;
using namespace fhamonic::melon;

static_assert(melon::concepts::graph<static_digraph>);
static_assert(melon::concepts::outward_incidence_graph<static_digraph>);
static_assert(melon::concepts::outward_adjacency_graph<static_digraph>);
static_assert(melon::concepts::inward_incidence_graph<static_digraph>);
static_assert(melon::concepts::inward_adjacency_graph<static_digraph>);
static_assert(melon::concepts::has_vertex_map<static_digraph>);
static_assert(melon::concepts::has_arc_map<static_digraph>);

GTEST_TEST(static_digraph, empty_constructor) {
    static_digraph graph;
    ASSERT_EQ(graph.nb_vertices(), 0);
    ASSERT_EQ(graph.nb_arcs(), 0);
    ASSERT_TRUE(EMPTY(graph.vertices()));
    ASSERT_TRUE(EMPTY(graph.arcs()));
    ASSERT_TRUE(EMPTY(graph.arc_entries()));

    ASSERT_FALSE(graph.is_valid_vertex(0));
    ASSERT_FALSE(graph.is_valid_arc(0));

    EXPECT_DEATH((void)graph.out_arcs(0), "");
    EXPECT_DEATH((void)graph.target(0), "");
    EXPECT_DEATH((void)graph.out_arcs(0), "");
    EXPECT_DEATH((void)graph.source(0), "");
}

GTEST_TEST(static_digraph, empty_vectors_constructor) {
    std::vector<vertex_t<static_digraph>> sources;
    std::vector<vertex_t<static_digraph>> targets;

    static_digraph graph(0, std::move(sources), std::move(targets));
    ASSERT_EQ(graph.nb_vertices(), 0);
    ASSERT_EQ(graph.nb_arcs(), 0);
    ASSERT_TRUE(EMPTY(graph.vertices()));
    ASSERT_TRUE(EMPTY(graph.arcs()));
    ASSERT_TRUE(EMPTY(graph.arc_entries()));

    ASSERT_FALSE(graph.is_valid_vertex(0));
    ASSERT_FALSE(graph.is_valid_arc(0));

    EXPECT_DEATH((void)graph.out_arcs(0), "");
    EXPECT_DEATH((void)graph.target(0), "");
    EXPECT_DEATH((void)graph.out_arcs(0), "");
    EXPECT_DEATH((void)graph.source(0), "");
}

GTEST_TEST(static_digraph, vectors_constructor_1) {
    std::vector<
        std::pair<arc_t<static_digraph>, std::pair<vertex_t<static_digraph>,
                                                   vertex_t<static_digraph>>>>
        arc_pairs(
            {{0, {0, 1}}, {1, {0, 2}}, {2, {1, 2}}, {3, {2, 0}}, {4, {2, 1}}});

    static_digraph graph(
        3, std::ranges::views::keys(std::ranges::views::values(arc_pairs)),
        std::ranges::views::values(std::ranges::views::values(arc_pairs)));
    ASSERT_EQ(graph.nb_vertices(), 3);
    ASSERT_EQ(graph.nb_arcs(), 5);
    ASSERT_TRUE(EQ_MULTISETS(graph.vertices(), {0, 1, 2}));
    ASSERT_TRUE(EQ_MULTISETS(graph.arcs(), {0, 1, 2, 3, 4}));

    for(auto u : graph.vertices()) ASSERT_TRUE(graph.is_valid_vertex(u));
    ASSERT_FALSE(
        graph.is_valid_vertex(vertex_t<static_digraph>(graph.nb_vertices())));

    for(auto a : graph.arcs()) ASSERT_TRUE(graph.is_valid_arc(a));
    ASSERT_FALSE(graph.is_valid_arc(arc_t<static_digraph>(graph.nb_arcs())));

    ASSERT_TRUE(EQ_MULTISETS(graph.out_neighbors(0), {1, 2}));
    ASSERT_TRUE(EQ_MULTISETS(graph.out_neighbors(1), {2}));
    ASSERT_TRUE(EQ_MULTISETS(graph.out_neighbors(2), {0, 1}));

    ASSERT_TRUE(EQ_MULTISETS(graph.in_neighbors(0), {2}));
    ASSERT_TRUE(EQ_MULTISETS(graph.in_neighbors(1), {0, 2}));
    ASSERT_TRUE(EQ_MULTISETS(graph.in_neighbors(2), {0, 1}));

    ASSERT_TRUE(EQ_MULTISETS(graph.arc_entries(), arc_pairs));

    for(arc_t<static_digraph> a : graph.arcs()) {
        ASSERT_EQ(graph.source(a), arc_pairs[a].second.first);
        ASSERT_EQ(graph.target(a), arc_pairs[a].second.second);
    }
}

GTEST_TEST(static_digraph, vectors_constructor_2) {
    std::vector<
        std::pair<arc_t<static_digraph>, std::pair<vertex_t<static_digraph>,
                                                   vertex_t<static_digraph>>>>
        arc_pairs({{0, {1, 2}},
                   {1, {1, 6}},
                   {2, {1, 7}},
                   {3, {2, 3}},
                   {4, {2, 4}},
                   {5, {3, 4}},
                   {6, {5, 2}},
                   {7, {5, 3}},
                   {8, {6, 5}}});

    static_digraph graph(
        8, std::ranges::views::keys(std::ranges::views::values(arc_pairs)),
        std::ranges::views::values(std::ranges::views::values(arc_pairs)));
    ASSERT_EQ(graph.nb_vertices(), 8);
    ASSERT_EQ(graph.nb_arcs(), 9);

    ASSERT_TRUE(EQ_MULTISETS(graph.vertices(), {0, 1, 2, 3, 4, 5, 6, 7}));
    ASSERT_TRUE(EQ_MULTISETS(graph.arcs(), {0, 1, 2, 3, 4, 5, 6, 7, 8}));

    for(auto u : graph.vertices()) ASSERT_TRUE(graph.is_valid_vertex(u));
    ASSERT_FALSE(
        graph.is_valid_vertex(vertex_t<static_digraph>(graph.nb_vertices())));

    for(auto a : graph.arcs()) ASSERT_TRUE(graph.is_valid_arc(a));
    ASSERT_FALSE(graph.is_valid_arc(arc_t<static_digraph>(graph.nb_arcs())));

    ASSERT_TRUE(EMPTY(graph.out_neighbors(0)));
    ASSERT_TRUE(EQ_MULTISETS(graph.out_neighbors(1), {2, 6, 7}));
    ASSERT_TRUE(EQ_MULTISETS(graph.out_neighbors(2), {3, 4}));
    ASSERT_TRUE(EQ_MULTISETS(graph.out_neighbors(6), {5}));
    ASSERT_TRUE(EMPTY(graph.out_neighbors(7)));

    ASSERT_TRUE(EQ_MULTISETS(graph.arc_entries(), arc_pairs));

    for(arc_t<static_digraph> a : graph.arcs()) {
        ASSERT_EQ(graph.source(a), arc_pairs[a].second.first);
        ASSERT_EQ(graph.target(a), arc_pairs[a].second.second);
    }
}

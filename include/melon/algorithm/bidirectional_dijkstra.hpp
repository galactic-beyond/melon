#ifndef MELON_ALGORITHM_BIDIRECTIONAL_DIJKSTA_HPP
#define MELON_ALGORITHM_BIDIRECTIONAL_DIJKSTA_HPP

#include <algorithm>
#include <cassert>
#include <concepts>
#include <optional>
#include <ranges>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "melon/concepts/graph.hpp"
#include "melon/concepts/map_of.hpp"
#include "melon/concepts/priority_queue.hpp"
#include "melon/data_structures/d_ary_heap.hpp"
#include "melon/utils/constexpr_ternary.hpp"
#include "melon/utils/intrusive_view.hpp"
#include "melon/utils/prefetch.hpp"
#include "melon/utils/semirings.hpp"
#include "melon/utils/traversal_iterator.hpp"

namespace fhamonic {
namespace melon {

// clang-format off
namespace concepts {
template <typename T>
concept bidirectional_dijkstra_trait = semiring<typename T::semiring> &&
    updatable_priority_queue<typename T::heap> && requires() {
    { T::store_path } -> std::convertible_to<bool>;
};
}  // namespace concepts
// clang-format on

template <typename G, typename L>
struct bidirectional_dijkstra_default_traits {
    using semiring = shortest_path_semiring<mapped_value_t<L, arc_t<G>>>;
    using heap =
        d_ary_heap<2, vertex_t<G>, mapped_value_t<L, arc_t<G>>,
                   decltype(semiring::less), vertex_map_t<G, std::size_t>>;

    static constexpr bool store_path = true;
};

template <concepts::incidence_list G, concepts::input_map<arc_t<G>> L,
          concepts::bidirectional_dijkstra_trait T =
              bidirectional_dijkstra_default_traits<G, L>>
    requires concepts::has_vertex_map<G>
class bidirectional_dijkstra {
private:
    using vertex = vertex_t<G>;
    using arc = arc_t<G>;

public:
    using value_t = mapped_value_t<L, vertex>;
    using traits = T;

private:
    enum vertex_status : char { PRE_HEAP = 0, IN_HEAP = 1, POST_HEAP = 2 };

    using heap = traits::heap;
    using vertex_status_map =
        vertex_map_t<G, std::pair<vertex_status, vertex_status>>;


    using optional_arc = std::optional<arc>;
    using pred_arcs_map = std::conditional<traits::store_path,
                                           vertex_map_t<G, optional_arc>,
                                           std::monostate>::type;
    using optional_midpoint =
        std::conditional<traits::store_path, std::optional<vertex>,
                         std::monostate>::type;

private:
    const G & _graph;
    const L & _length_map;

    heap _forward_heap;
    heap _reverse_heap;
    vertex_status_map _vertex_status_map;

    pred_arcs_map _forward_pred_arcs_map;
    pred_arcs_map _reverse_pred_arcs_map;
    optional_midpoint _midpoint;

public:
    bidirectional_dijkstra(const G & g, const L & l)
        : _graph(g)
        , _length_map(l)
        , _forward_heap(g.template create_vertex_map<std::size_t>())
        , _reverse_heap(g.template create_vertex_map<std::size_t>())
        , _vertex_status_map(g.template create_vertex_map<
                             std::pair<vertex_status, vertex_status>>(
              std::make_pair(PRE_HEAP, PRE_HEAP)))
        , _forward_pred_arcs_map(constexpr_ternary<traits::store_path>(
              g.template create_vertex_map<optional_arc>(), std::monostate{}))
        , _reverse_pred_arcs_map(constexpr_ternary<traits::store_path>(
              g.template create_vertex_map<optional_arc>(), std::monostate{})) {}

    bidirectional_dijkstra(const G & g, const L & l, const vertex & s,
                           const vertex & t)
        : bidirectional_dijkstra(g, l) {
        add_source(s);
        add_target(t);
    }

    bidirectional_dijkstra & reset() noexcept {
        _forward_heap.clear();
        _reverse_heap.clear();
        for(auto && u : _graph.vertices())
            _vertex_status_map[u] = std::make_pair(PRE_HEAP, PRE_HEAP);
        _midpoint.reset();
        return *this;
    }
    bidirectional_dijkstra & add_source(
        const vertex & s, const value_t dist = traits::semiring::zero) noexcept {
        assert(_vertex_status_map[s].first == PRE_HEAP);
        _forward_heap.push(s, dist);
        _vertex_status_map[s].first = IN_HEAP;
        if constexpr(traits::store_path) _forward_pred_arcs_map[s].reset();
        return *this;
    }
    bidirectional_dijkstra & add_target(
        const vertex & t, const value_t dist = traits::semiring::zero) noexcept {
        assert(_vertex_status_map[t].second == PRE_HEAP);
        _reverse_heap.push(t, dist);
        _vertex_status_map[t].second = IN_HEAP;
        if constexpr(traits::store_path) _reverse_pred_arcs_map[t].reset();
        return *this;
    }

public:
    value_t run() noexcept {
        value_t st_dist = traits::semiring::infty;
        while(!_forward_heap.empty() && !_reverse_heap.empty()) {
            const auto && [u1, u1_dist] = _forward_heap.top();
            const auto && [u2, u2_dist] = _reverse_heap.top();
            if(traits::semiring::less(st_dist,
                                      traits::semiring::plus(u1_dist, u2_dist)))
                break;
            if(traits::semiring::less(u1_dist, u2_dist)) {
                const auto & out_arcs = _graph.out_arcs(u1);
                prefetch_range(out_arcs);
                prefetch_mapped_values(out_arcs, _graph.targets_map());
                prefetch_mapped_values(out_arcs, _length_map);
                _vertex_status_map[u1].first = POST_HEAP;
                _forward_heap.pop();
                for(const arc a : out_arcs) {
                    const vertex w = _graph.target(a);
                    auto [w_forward_status, w_reverse_status] =
                        _vertex_status_map[w];
                    if(w_forward_status == IN_HEAP) {
                        const value_t new_w_dist =
                            traits::semiring::plus(u1_dist, _length_map[a]);
                        if(traits::semiring::less(new_w_dist,
                                                  _forward_heap.priority(w))) {
                            _forward_heap.promote(w, new_w_dist);
                            if(w_reverse_status == IN_HEAP) {
                                const value_t new_st_dist =
                                    new_w_dist + _reverse_heap.priority(w);
                                if(traits::semiring::less(new_st_dist,
                                                          st_dist)) {
                                    st_dist = new_st_dist;
                                    _midpoint.emplace(w);
                                }
                            }
                            if constexpr(traits::store_path)
                                _forward_pred_arcs_map[w].emplace(a);
                        }
                    } else if(w_forward_status == PRE_HEAP) {
                        const value_t new_w_dist =
                            traits::semiring::plus(u1_dist, _length_map[a]);
                        _forward_heap.push(w, new_w_dist);
                        _vertex_status_map[w].first = IN_HEAP;
                        if(w_reverse_status == IN_HEAP) {
                            const value_t new_st_dist =
                                new_w_dist + _reverse_heap.priority(w);
                            if(traits::semiring::less(new_st_dist, st_dist)) {
                                st_dist = new_st_dist;
                                _midpoint.emplace(w);
                            }
                        }
                        if constexpr(traits::store_path)
                            _forward_pred_arcs_map[w].emplace(a);
                    }
                }
            } else {
                const auto & in_arcs = _graph.in_arcs(u2);
                prefetch_range(in_arcs);
                prefetch_mapped_values(in_arcs, _graph.sources_map());
                prefetch_mapped_values(in_arcs, _length_map);
                _vertex_status_map[u2].second = POST_HEAP;
                _reverse_heap.pop();
                for(const arc a : in_arcs) {
                    const vertex w = _graph.source(a);
                    auto [w_forward_status, w_reverse_status] =
                        _vertex_status_map[w];
                    if(w_reverse_status == IN_HEAP) {
                        const value_t new_w_dist =
                            traits::semiring::plus(u2_dist, _length_map[a]);
                        if(traits::semiring::less(new_w_dist,
                                                  _reverse_heap.priority(w))) {
                            _reverse_heap.promote(w, new_w_dist);
                            if(w_forward_status == IN_HEAP) {
                                const value_t new_st_dist =
                                    new_w_dist + _forward_heap.priority(w);
                                if(traits::semiring::less(new_st_dist,
                                                          st_dist)) {
                                    st_dist = new_st_dist;
                                    _midpoint.emplace(w);
                                }
                            }
                            if constexpr(traits::store_path)
                                _reverse_pred_arcs_map[w].emplace(a);
                        }
                    } else if(w_reverse_status == PRE_HEAP) {
                        const value_t new_w_dist =
                            traits::semiring::plus(u2_dist, _length_map[a]);
                        _reverse_heap.push(w, new_w_dist);
                        _vertex_status_map[w].second = IN_HEAP;
                        if(w_forward_status == IN_HEAP) {
                            const value_t new_st_dist =
                                new_w_dist + _forward_heap.priority(w);
                            if(traits::semiring::less(new_st_dist, st_dist)) {
                                st_dist = new_st_dist;
                                _midpoint.emplace(w);
                            }
                        }
                        if constexpr(traits::store_path)
                            _reverse_pred_arcs_map[w].emplace(a);
                    }
                }
            }
        }
        return st_dist;
    }

    arc pred_arc(const vertex & u) const noexcept
        requires(traits::store_path)
    {
        assert(_vertex_status_map[u].first != PRE_HEAP);
        return _forward_pred_arcs_map[u].value();
    }
    arc succ_arc(const vertex & u) const noexcept
        requires(traits::store_path)
    {
        assert(_vertex_status_map[u].second != PRE_HEAP);
        return _reverse_pred_arcs_map[u].value();
    }
    auto retrieve_path() const noexcept {
        assert(_midpoint.has_value());
        return std::views::join(
            intrusive_view(
                _forward_pred_arcs_map[_midpoint.value()],
                [](const std::optional<arc> & oa) -> arc { return oa.value(); },
                [this](const std::optional<arc> & oa) -> optional_arc {
                    return _forward_pred_arcs_map[_graph.source(oa.value())];
                },
                [](const std::optional<arc> & oa) -> bool { return oa.has_value(); }),
            intrusive_view(
                _reverse_pred_arcs_map[_midpoint.value()],
                [](const std::optional<arc> & oa) -> arc { return oa.value(); },
                [this](const std::optional<arc> & oa) -> optional_arc {
                    return _reverse_pred_arcs_map[_graph.target(oa.value())];
                },
                [](const std::optional<arc> & oa) -> bool { return oa.has_value(); }));
    }
};

}  // namespace melon
}  // namespace fhamonic

#endif  // MELON_ALGORITHM_BIDIRECTIONAL_DIJKSTA_HPP

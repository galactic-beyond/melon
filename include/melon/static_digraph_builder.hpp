#ifndef MELON_STATIC_DIGRAPH_BUILDER_HPP
#define MELON_STATIC_DIGRAPH_BUILDER_HPP

#include <algorithm>
#include <numeric>
#include <ranges>
#include <vector>

#include <range/v3/algorithm/sort.hpp>
#include <range/v3/view/zip.hpp>

#include "melon/static_digraph.hpp"

namespace fhamonic {
namespace melon {

template <typename... ArcProperty>
class StaticDigraphBuilder {
public:
    using Node = StaticDigraph::Node;
    using Arc = StaticDigraph::Arc;

    using PropertyMaps = std::tuple<std::vector<ArcProperty>...>;

private:
    std::size_t _nb_nodes;
    std::vector<Arc> _nb_out_arcs;
    std::vector<Node> _arc_sources;
    std::vector<Node> _arc_targets;
    PropertyMaps _arc_property_maps;

public:
    StaticDigraphBuilder() : _nb_nodes(0) {}
    StaticDigraphBuilder(std::size_t nb_nodes)
        : _nb_nodes(nb_nodes), _nb_out_arcs(nb_nodes, 0) {}

private:
    template <class Maps, class Properties, std::size_t... Is>
    void addProperties(Maps && maps, Properties && properties,
                       std::index_sequence<Is...>) {
        (get<Is>(maps).push_back(get<Is>(properties)), ...);
    }

public:
    void add_arc(Node u, Node v, ArcProperty... properties) {
        assert(_nb_nodes > std::max(u, v));
        ++_nb_out_arcs[u];
        _arc_sources.push_back(u);
        _arc_targets.push_back(v);
        addProperties(
            _arc_property_maps, std::make_tuple(properties...),
            std::make_index_sequence<std::tuple_size<PropertyMaps>{}>{});
    }

    auto build() {
        // sort _arc_sources, arc_tagrets and _arc_property_maps
        auto arcs_zipped_view = std::apply(
            [this](auto &&... property_map) {
                return ranges::view::zip(_arc_sources, _arc_targets,
                                         property_map...);
            },
            _arc_property_maps);
        ranges::sort(arcs_zipped_view, [](const auto & a, const auto & b) {
            if(std::get<0>(a) == std::get<0>(b))
                return std::get<1>(a) < std::get<1>(b);
            return std::get<0>(a) < std::get<0>(b);
        });
        // compute out_arc_begin
        std::exclusive_scan(_nb_out_arcs.begin(), _nb_out_arcs.end(),
                            _nb_out_arcs.begin(), 0);
        // create graph
        StaticDigraph graph(std::move(_nb_out_arcs), std::move(_arc_targets));
        return std::apply(
            [this, &graph](auto &&... property_map) {
                return std::make_tuple(graph, property_map...);
            },
            _arc_property_maps);
    }
};

}  // namespace melon
}  // namespace fhamonic

#endif  // MELON_STATIC_DIGRAPH_BUILDER_HPP

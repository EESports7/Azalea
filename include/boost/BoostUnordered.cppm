module;

#include <boost/unordered/unordered_set.hpp>
#include <boost/unordered/unordered_node_set.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
#include <boost/unordered/concurrent_node_set.hpp>
#include <boost/unordered/concurrent_flat_set.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/unordered/unordered_node_map.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/concurrent_node_map.hpp>
#include <boost/unordered/concurrent_flat_map.hpp>

export module boost.unordered;

export namespace boost::unordered {
    // Sets
    using ::boost::unordered::unordered_set;
    using ::boost::unordered::unordered_multiset;
    using ::boost::unordered::unordered_node_set;
    using ::boost::unordered::unordered_flat_set;
    using ::boost::unordered::concurrent_node_set;
    using ::boost::unordered::concurrent_flat_set;

    // Maps
    using ::boost::unordered::unordered_map;
    using ::boost::unordered::unordered_multimap;
    using ::boost::unordered::unordered_node_map;
    using ::boost::unordered::unordered_flat_map;
    using ::boost::unordered::concurrent_node_map;
    using ::boost::unordered::concurrent_flat_map;
}
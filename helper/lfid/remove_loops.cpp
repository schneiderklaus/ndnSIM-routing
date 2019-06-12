/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2019 Klaus Schneider, The University of Arizona
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Klaus Schneider <klaus@cs.arizona.edu>
 */

#include <queue>

#include <boost/graph/graph_utility.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/ref.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/visitors.hpp>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/property_map/property_map.hpp>

#include "remove_loops.hpp"
#include "AbsFib.hpp"

namespace ns3 {
namespace ndn {

using std::cout;
using std::set;
using Fib = AbsFib;

/*
 * Fill directed graph only with edges existing in the FIB.
 */
void getDigraphFromFib(DiGraph &dg, const AllNodeFib &allNodeFIB,
    const int dstId) {

  // 1. Erase All Arcs:
  dg.clear();

  // 2. Add Arcs from FIB
  for (const auto& node : allNodeFIB) {
    int nodeId = node.first;
    if (dstId == nodeId) {
      continue;
    }

    for (const auto &fibNh : node.second.getNhs(dstId)) {
      assert(fibNh.getType() <= NextHopType::UPWARD);
      boost::add_edge(static_cast<uint64_t>(nodeId), static_cast<uint64_t>(fibNh.getNhId()), 1, dg);
    }
  }
}


class NodePrio {
public:
    NodePrio(int nodeId, int remainingNh, set<FibNextHop> nhSet)
			: m_nodeId {nodeId}, m_remainingNh {remainingNh}, uwSet {nhSet}
	{
	  assert(remainingNh > 0 && uwSet.size() > 0);
	  assert(static_cast<int>(uwSet.size()) < remainingNh);
	}

	int getId() const {
	  return m_nodeId;
	}

	int getRemainingUw() const {
      return static_cast<int>(uwSet.size());
	}

	/*
	 * Order by Remamining UW NHs, Highest DeltaCost, and then id.
	 */
    bool operator<(const NodePrio& other) const {
        return std::make_tuple(m_remainingNh, getHighestCostUw(), m_nodeId)
                < std::make_tuple(other.m_remainingNh, other.getHighestCostUw(), other.m_nodeId);
    }

	// Setters:
    FibNextHop popHighestCostUw() {
      const FibNextHop& tmp = getHighestCostUw();
      eraseUw(tmp);
      return tmp;
    }

	void reduceRemainingNh() {
		m_remainingNh--;
		// Check that remaining nexthops >= remaining uw nexthops.
		assert(m_remainingNh > 0 && m_remainingNh > getRemainingUw());
	}

private:
    void eraseUw(FibNextHop nh) {
        assert(uwSet.size() > 0);
        auto success = uwSet.erase(nh);
        assert(success == 1);
    }

    FibNextHop getHighestCostUw() const {
      assert(uwSet.size() > 0);
      assert(std::prev(uwSet.end()) != uwSet.end());
      return *(std::prev(uwSet.end()));
    }

private:
	int m_nodeId;
	int m_remainingNh;
	set<FibNextHop> uwSet;
	friend std::ostream& operator<<(std::ostream&,  const NodePrio &node);
};

std::ostream& operator<< (std::ostream& os, const NodePrio &node) {
    return os << "Id: " << node.m_nodeId << ", remaining NH: "
				<< node.m_remainingNh << ", remaining UW: " << node.getRemainingUw() << " ";
}


int removeLoops(AllNodeFib &allNodeFIB, bool printOutput) {
  (void)printOutput;
  int removedLoopCounter = 0;
  int upwardCounter = 0;

  const int NUM_NODES {static_cast<int>(allNodeFIB.size())};

  // Build graph with boost graph library:
  DiGraph dg{};

  // Add all Arcs that fit into FIB. // O(n)
  for (int dstId = 0; dstId < NUM_NODES; dstId++) {
    // 1. Get DiGraph from Fib //
    getDigraphFromFib(dg, allNodeFIB, dstId);

    // Maybe use for optimization:
    // ArcMap arcMap = getArcMap(dg);

    // NodeId -> set<UwNexthops>
    std::priority_queue<NodePrio> q;

    // 2. Put nodes in the queue, ordered by # remaining nexthops, then CostDelta // O(n^2)
    for (const auto& node : allNodeFIB) {
      int nodeId {node.first};
      const Fib &fib {node.second};
      if (nodeId == dstId) {
        continue;
      }

      const auto& uwNhSet = fib.getUpwardNhs(dstId);
      if (!uwNhSet.empty()) {
        upwardCounter += uwNhSet.size();

        int fibSize {fib.numEnabledNhPerDst(dstId)};
        // NodePrio tmpNode {nodeId, fibSize, uwNhSet};
        q.emplace(nodeId, fibSize, uwNhSet);
      }
    }

    // 3. Iterate PriorityQueue //
    while (!q.empty()) {
      NodePrio node = q.top();
      q.pop();

      int nodeId = node.getId();
      int nhId = node.popHighestCostUw().getNhId();

      // Remove opposite of Uphill link
//      int arcId1 {getArcId(arcMap, nhId, nodeId)};
      auto res = boost::edge(static_cast<uint64_t>(nhId), static_cast<uint64_t>(nodeId), dg);

      auto arc = res.first;
      bool arcExists = res.second;

      if (arcExists) {
        boost::remove_edge(arc, dg);
//        dg.erase(dg.arcFromId(arcId1));
//        arcMap.erase({nhId, nodeId});
      }

      // 2. Loop Check: Is the current node still reachable for the uphill nexthop?
      // Uses BFS:
      // bool willLoop = bfs(dg).run(dg.nodeFromId(nhId), dg.nodeFromId(nodeId)); // O(m^2n)

      std::vector<int> dists(num_vertices(dg));

      auto weightmap = get(boost::edge_weight, dg);

      const auto& x = boost::edges(dg);
      for(auto e = x.first; e != x.second; e++){
        int weight = get(weightmap, *e);
        assert(weight == 1); // Only use uniform weights.
      }

      // TODO: Could be replaced by BFS/DFS to improve speed.
      boost::dijkstra_shortest_paths(dg, static_cast<uint64_t>(nhId),
          distance_map(boost::make_iterator_property_map(dists.begin(), get(boost::vertex_index, dg)))
      );

      bool willLoop = (dists.at(static_cast<size_t>(nodeId)) < (std::numeric_limits<int>::max()-1));

      // Uphill nexthop loops back to original node
      if (willLoop) {
        node.reduceRemainingNh();
        removedLoopCounter++;

        // Erase FIB entry
        allNodeFIB.at(node.getId()).erase(dstId, nhId);

        // Erase UW nexthop from Digraph:
//        int arcId2 = getArcId(arcMap, node.getId(), nhId);
//        assert(arcId2 >=0);

        auto res2 = boost::edge(static_cast<uint64_t>(node.getId()), static_cast<uint64_t>(nhId), dg);
        auto arc2 = res2.first;
        assert(res.second);

        boost::remove_edge(arc2, dg);

//        dg.erase(dg.arcFromId(arcId2));
//        arcMap.erase({node.getId(), nhId});
      }

      // Add opposite of UW link back:
      if (arcExists) {
        boost::add_edge(static_cast<uint64_t>(nhId), static_cast<uint64_t>(nodeId), 1, dg);
//        auto arc = dg.addArc(dg.nodeFromId(nhId), dg.nodeFromId(nodeId));
//        arcMap.emplace(std::make_pair(nhId, nodeId), dg.id(arc));
      }

      // If not has further UW nexthops: Requeue.
      if (node.getRemainingUw() > 0) {
        q.push(node);
      }
    }
  }

  if (printOutput) {
    std::cout << "Found " << upwardCounter << " UW nexthops, Removed " << removedLoopCounter
        << " Looping UwNhs, Remaining: " << upwardCounter - removedLoopCounter << " NHs\n";
  }
  assert((upwardCounter - removedLoopCounter) >= 0);

  return removedLoopCounter;
}


int removeDeadEnds(AllNodeFib &allNodeFIB, bool printOutput) {
  int NUM_NODES {static_cast<int>(allNodeFIB.size())};
  int checkedUwCounter {0};
  int uwCounter {0};
  int removedDeCounter {0};

  for (int dstId = 0; dstId < NUM_NODES; dstId++) {
    // NodeId -> FibNexthops (Order important)
    set<std::pair<int, FibNextHop>> nhSet;

    // 1. Put all uwNexthops in set<NodeId, FibNexhtop>:
    for (const auto& node : allNodeFIB) {
      int nodeId {node.first};
      if (nodeId == dstId) {
        continue;
      }

      const auto& uwNhSet = node.second.getUpwardNhs(dstId);
      uwCounter+= uwNhSet.size();
      for (const FibNextHop &fibNh : uwNhSet) {
        nhSet.emplace(nodeId, fibNh);
      }
    }

    // FibNexthops ordered by (costDelta, cost, nhId).
    // Start with nexthop with highest cost:
    while (!nhSet.empty()) {
      checkedUwCounter++;

      // Pop from queue:
      const auto &nhPair = nhSet.begin();
      assert(nhPair != nhSet.end());
      nhSet.erase(nhPair);

      int nodeId = nhPair->first;
      const FibNextHop &nh = nhPair->second;
      Fib& fib = allNodeFIB.at(nodeId);

      if (nh.getNhId() == dstId) {
        continue;
      }

      int reverseEntries {allNodeFIB.at(nh.getNhId()).numEnabledNhPerDst(dstId)};

      // Must have at least one FIB entry.
      assert(reverseEntries > 0);

      // If it has exactly 1 entry -> Is downward back through the upward nexthop!
      // Higher O-Complexity below:
      if (reverseEntries <= 1) {
        removedDeCounter++;

        // Erase NhEntry from FIB:
        fib.erase(dstId, nh.getNhId());

        // Push into Queue: All NhEntries that lead to m_nodeId!
        const auto &nexthops = fib.getNhs(dstId);

        for (const auto& ownNhs : nexthops) {
          if (ownNhs.getType() == NextHopType::DW && ownNhs.getNhId() != dstId) {
            const auto &reverseNh = allNodeFIB.at(ownNhs.getNhId()).getNhs(dstId);

            for (const auto& y : reverseNh) {
              if (y.getNhId() == nodeId) {
                assert(y.getType() == NextHopType::UPWARD);
//                cout << "Dst: " << dstId << ", Node " << ownNhs.getNhId() << " Pushing nh: "
//                    << y << "!\n";
                nhSet.emplace(ownNhs.getNhId(), y);
                break;
              }
            }
          }
        }
      }
    }
  }

  if (printOutput) {
    cout << "Checked " << checkedUwCounter << " Upward NHs, Removed " << removedDeCounter
        << " Deadend UwNhs, Remaining: " << uwCounter - removedDeCounter << " NHs\n";
  }

  return removedDeCounter;
}


} // namespace ndn
} // namespace ns3

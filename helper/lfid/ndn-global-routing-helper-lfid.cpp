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

#include "../ndn-global-routing-helper.hpp"
#include "../boost-graph-ndn-global-routing-helper.hpp"
//
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
//
#include "helper/ndn-fib-helper.hpp"
#include "model/ndn-global-router.hpp"
//
#include "remove_loops.hpp"
#include "AbsFib.hpp"


NS_LOG_COMPONENT_DEFINE("ndn.GlobalRoutingHelperLfid");

namespace ns3 {
namespace ndn {

using std::unordered_map;
using std::string;
using std::get;

void GlobalRoutingHelper::CalculateLFIDRoutes() {
  BOOST_CONCEPT_ASSERT((boost::VertexListGraphConcept<boost::NdnGlobalRouterGraph>));
  BOOST_CONCEPT_ASSERT((boost::IncidenceGraphConcept<boost::NdnGlobalRouterGraph>));

  // Creates graph from nodeList:
  boost::NdnGlobalRouterGraph graph {};

  AllNodeFib allNodeFIB;

  // Store mapping nodeId -> faceId -> Ptr<Face>
  unordered_map<int, unordered_map<nfd::FaceId, shared_ptr<Face>>> faceMap;

  // For all existing nodes:
  for (auto node = NodeList::Begin(); node != NodeList::End(); node++) {

    int nodeId = static_cast<int>((*node)->GetId());
    const Ptr<GlobalRouter>& source = (*node)->GetObject<GlobalRouter>();

    AbsFib nodeFib = AbsFib {source, static_cast<int>(NodeList::GetNNodes())};

    if (source == nullptr) {
      NS_LOG_ERROR("Node " << (*node)->GetId() << " does not export GlobalRouter interface");
      continue;
    }

    // map: neighborId -> DistancesMap
    unordered_map<int, boost::DistancesMap> nbSp;

    // map: Destination (GlobalRouter) -> Distance
    boost::DistancesMap distMap;
    unordered_map<int, boost::DistancesMap> neighborSpMap;

    dijkstra_shortest_paths(graph, source,
        distance_map(boost::ref(distMap)).distance_inf(boost::WeightInf).distance_zero(
            boost::WeightZero).distance_compare(boost::WeightCompare()).distance_combine(
            boost::WeightCombine()));

    // 1. Get all neighbors of node.
    unordered_map<nfd::FaceId, uint64_t> originalMetrics;
    auto& originalFace = faceMap[nodeId];

    const GlobalRouter::IncidencyList &neighbors = source->GetIncidencies();
    // Set link weight of all neighbors to infinity
    for (const auto& neighbor : neighbors) {
      int nbId = get<2>(neighbor)->GetObject<ns3::Node>()->GetId();
      assert(nbId != nodeId);

      auto& face = get<shared_ptr<Face>>(neighbor);
      assert(face != nullptr);

      originalMetrics[nbId] = face->getMetric();
      originalFace[nbId] = face; // Is only a copy
      face->setMetric(get<uint16_t>(boost::WeightInf));
    }

    // 2. Calculate Dijkstra for neighbors
    for (const auto& neighbor : neighbors) {
      auto& nSource = get<0>(neighbor);
      auto& target = get<2>(neighbor);

      int nbId = target->GetObject<ns3::Node>()->GetId();
      Ptr<GlobalRouter> nbRouter = target;

      assert(target != source && nbId != nodeId);
      assert(nSource == source);

      dijkstra_shortest_paths(graph, nbRouter,
          distance_map(boost::ref(neighborSpMap[nbId])).distance_inf(boost::WeightInf).distance_zero(
              boost::WeightZero).distance_compare(boost::WeightCompare()).distance_combine(
              boost::WeightCombine()));
    }

    // 3. Reset link weights
    for (const auto& neighbor : neighbors) {
      int nbId = get<2>(neighbor)->GetObject<ns3::Node>()->GetId();
      assert(nbId != nodeId);

      auto& face = get<shared_ptr<Face>>(neighbor);
      assert(face->getMetric() == get<uint16_t>(boost::WeightInf));
      face->setMetric(originalMetrics.at(nbId));
    }

    // 4. Fill Abstract FIB:
    // For each destination:
    for (const auto& dstEntry : distMap) {
      Ptr<GlobalRouter> dstRouter = dstEntry.first;
      int dstId = dstRouter->GetObject<ns3::Node>()->GetId();
      if (dstRouter == source) continue; // Skip destination == source.

      int spTotalCost = static_cast<int>(get<uint32_t>(dstEntry.second));

      // For each neighbor:
      for (const auto& nb : neighborSpMap) {
        int neighborId = nb.first;
        const auto& nbDist {get<uint32_t>(nb.second.at(dstRouter))};

        int neighborCost = static_cast<int>(nbDist);
        int neighborTotalCost = neighborCost
            + static_cast<int>(originalFace.at(neighborId)->getMetric());

        assert(neighborTotalCost >= spTotalCost);

        // Skip routers that would loop back
        if (neighborTotalCost >= static_cast<int>(get<uint16_t>(boost::WeightInf)))
          continue;

        NextHopType nbType;
        if (neighborCost < spTotalCost) {
          nbType = NextHopType::DW;
        }
        else {
          nbType = NextHopType::UPWARD;
        }

        int costDelta = neighborTotalCost - spTotalCost;
        FibNextHop nh = {neighborTotalCost, neighborId, costDelta, nbType};
        nodeFib.insert(dstId, nh);
      }

    } // End for all dsts

    nodeFib.checkFib();
    allNodeFIB.emplace(nodeId, nodeFib);
  } // End for all nodes


  ///  4. Remove loops and Deadends ///
  removeLoops(allNodeFIB, true);
  removeDeadEnds(allNodeFIB, true);
  std::cout << "\n";

  // 5. Insert from AbsFIB into real FIB!
  // For each node in the AbsFIB: Insert into real fib.
  for (const auto& nodeEntry : allNodeFIB) {
    int nodeId = nodeEntry.first;
    const auto& fib = nodeEntry.second;

    // For each destination:
    for (const auto& dst : fib) {
      int dstId = dst.first;
      const auto& dstRouter = allNodeFIB.at(dstId).getGR();

      // Each fibNexthop
      for (const auto& nh : dst.second) {
        int neighborId = nh.getNhId();
        int neighborTotalCost = nh.getCost();

        for (const auto& prefix : dstRouter->GetLocalPrefixes()) {
          Ptr<Node> node = NodeList::GetNode(static_cast<uint32_t>(nodeId));
          FibHelper::AddRoute(node, *prefix, faceMap.at(nodeId).at(neighborId),
              neighborTotalCost);
        }
      }
    }
  }
}



} // namespace ndn
} // namespace ns3



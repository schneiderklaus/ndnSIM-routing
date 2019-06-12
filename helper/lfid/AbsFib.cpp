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

#include "AbsFib.hpp"

#include <algorithm>

#include "ns3/node.h"
#include "ns3/names.h"
#include "ns3/ndnSIM/model/ndn-global-router.hpp"
#include "model/ndn-l3-protocol.hpp"
#include "daemon/fw/forwarder.hpp"

namespace ns3 {
namespace ndn {

using std::set;

AbsFib::AbsFib(const Ptr<GlobalRouter>& own, int nodes) :
//      nodeId {own->GetL3Protocol()->getForwarder()->getNodeId()},
//      nodeName {own->GetL3Protocol()->getForwarder()->getNodeName()},
      nodeId {static_cast<int>(own->GetObject<ns3::Node>()->GetId())},
      nodeName {ns3::Names::FindName(own->GetObject<ns3::Node>())},
      numNodes {nodes},
      nodeDegree {static_cast<int>(own->GetIncidencies().size())},
      ownRouter {own},
      upwardCounter {0},
      totalNhCounter {0}
{
  checkInputs();

  createEmptyFib();
}

void AbsFib::checkInputs() {
  const auto MAX_SIZE {1e5};
  assert(nodeId >= 0 && nodeId <= MAX_SIZE);
  assert(nodeName.size() > 0 && nodeName.size() <= MAX_SIZE);
  assert(nodeDegree > 0 && nodeDegree <= MAX_SIZE);
  assert(numNodes > 1 && numNodes <= MAX_SIZE);
}

void AbsFib::createEmptyFib() {
  // Create empty FIB:
  for (int dstId = 0; dstId < numNodes; dstId++) {
    if (dstId == nodeId) {
      continue;
    }
    perDstFib.insert( {dstId, {}});
    upwardPerDstFib.insert( {dstId, {}});
  }
}

Ptr<GlobalRouter> AbsFib::getGR() const {
  return ownRouter;
}

// Setters:
void AbsFib::insert(int dstId, const FibNextHop &nh) {
  assert(nh.getType() == NextHopType::DW || nh.getType() == NextHopType::UPWARD);
  assert(nh.getCost() > 0 && nh.getCostDelta() >= 0);
  assert(nh.getNhId() != nodeId);

  bool inserted1 = perDstFib.at(dstId).insert(nh).second;
  assert(inserted1); // Check if it didn't exist yet.
  totalNhCounter++;

  if (nh.getType() == NextHopType::UPWARD) {
    bool inserted2 = upwardPerDstFib.at(dstId).insert(nh).second;
    assert(inserted2);
    upwardCounter++;
  }
}

size_t AbsFib::erase(int dstId, int nhId) {
  auto &fib {perDstFib.at(dstId)};

  auto fibNh = std::find_if(fib.begin(), fib.end(), [&](const FibNextHop& item)
  {
    return item.getNhId() == nhId;
  });
  assert(fibNh != perDstFib.at(dstId).end());
  assert(fibNh->getType() == NextHopType::UPWARD);
  totalNhCounter--;

  fib.erase(fibNh);
  auto numErased2 = upwardPerDstFib.at(dstId).erase(*fibNh);
  assert(numErased2 == 1);
  upwardCounter--;

  return numErased2;
}

std::ostream& operator<<(std::ostream &os, const AbsFib &fib) {
    for (const auto& entry : fib.perDstFib) {
    	os << "\nFIB node: " << fib.nodeName << fib.nodeId << "\n";
    	os << "Dst: " << entry.first << "\n";
    	for (const auto &nh : entry.second) {
				os << nh <<"\n";
    	}
    }
    return os;
}

// O(1)
FibNextHop AbsFib::getNhAtPos(int dstId, int pos) const {
  assert(dstId != nodeId);

  const auto &nhSet = getNhs(dstId);
  auto nh = std::next(nhSet.begin(), pos);
  if (pos == 0) {
    assert(nh == nhSet.begin());
  }

  assert(nh != nhSet.end());
  return *nh;
}

// O(1)
set<FibNextHop> AbsFib::getNhs(int dstId) const {
  assert(dstId != nodeId);
  if (perDstFib.count(dstId) == 0) {
    std::cerr << "Node " << nodeId << " No nexthops for dst: " << dstId << "\n";
    return {};
  }
  assert(perDstFib.count(dstId) != 0);
  return perDstFib.at(dstId);
}

set<FibNextHop> AbsFib::getUpwardNhs(int dstId) const {
  assert(dstId != nodeId);
  if (upwardPerDstFib.count(dstId) == 0) {
    std::cerr << "Node " << nodeId << " No nexthops for dst: " << dstId << "\n";
    return {};
  }
  return upwardPerDstFib.at(dstId);
}


int AbsFib::getTotalNexthops() const {
//    int tmp {0};
//    for (auto x : perDstFib) {
//      tmp += x.second.size();
//    }
//    std::cout << "TotalNH: " << totalNhCounter << ", tmp: " << tmp << "\n";
//    assert(totalNhCounter == tmp);
  return totalNhCounter;
}


int AbsFib::countUwNexthops() const {
//    int tmp {0};
//    for (auto x : upwardPerDstFib){
//      tmp+=x.second.size();
//    }
//    assert(upwardCounter == tmp);
  return upwardCounter;
}


int AbsFib::numTypePerDst(int dstId, NextHopType type) const {
  assert(dstId != nodeId);
  const auto allNhs {numEnabledNhPerDst(dstId)};
  const auto uwNhs {static_cast<int>(getUpwardNhs(dstId).size())};

  if (type == NextHopType::UPWARD) {
    return uwNhs;
  }
  else if (type == NextHopType::DW) {
    assert(allNhs > uwNhs);
    return allNhs - uwNhs;
  }
  else {
    assert(false);
    return 0;
  }
}


void AbsFib::checkFib() const {
  assert(perDstFib.size() > 0);

  for (const auto& fibSet : perDstFib) {
    assert(fibSet.second.size() > 0);

    bool hasDownward{false};
    std::unordered_set<int> nextHopSet {};

    for (const FibNextHop& nextHop : fibSet.second) {
      assert(nextHop.getCost() > 0 && nextHop.getCost() < MAX_COST);
      if (nextHop.getType() == NextHopType::DW){
        hasDownward = true;
      }

      // Only one FIB entry per nexthop allowed!
      assert(nextHopSet.count(nextHop.getNhId()) == 0);
      nextHopSet.emplace(nextHop.getNhId());
    }
    assert(hasDownward);
    assert(nextHopSet.size() == fibSet.second.size());
  }
}


} // namespace ndn
} // namespace ns-3

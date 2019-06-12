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

#pragma once

#include <set>
#include <unordered_map>
#include <ns3/ptr.h>

#include "FibNextHop.hpp"

namespace ns3 {

namespace ndn {

using std::unordered_map;
using std::string;

class AbsFib;
class GlobalRouter;

using AllNodeFib = std::unordered_map<int, AbsFib>;

class AbsFib
{
public:
    AbsFib(const Ptr<GlobalRouter>& own, int nodes);

public:

// Getters:
  /*
   * Returns FibNexthop at given position. Enter 0 or leave blank for SP NH.
   */
  FibNextHop getNhAtPos(int dstId, int pos = 0) const;

  /**
   * Return reference to NH set per dst
   */
  std::set<FibNextHop> getNhs(int dstId) const;

  std::set<FibNextHop> getUpwardNhs(int dstId) const;

  void checkFib() const;

  int numEnabledNhPerDst(int dstId) const {
    assert(dstId != nodeId);
    return static_cast<int>(getNhs(dstId).size());
  }

  int numTypePerDst(int dstId, NextHopType type) const;

  int getId() const {
    return nodeId;
  }

  Ptr<GlobalRouter> getGR() const;

  std::string getName() const {
    return nodeName;
  }

  int getDegree() const {
    return nodeDegree;
  }

  int getNumDsts() const {
    return static_cast<int>(perDstFib.size());
  }

  int getTotalNexthops() const;

  int countUwNexthops() const;

  bool contains(int dstId) const {
    return perDstFib.count(dstId) > 0;
  }

  // Functions for range-based loop:
  auto begin() const {
    return perDstFib.cbegin();
  }

  auto end() const {
    return perDstFib.cend();
  }

// Setters:
  void insert(int dstId, const FibNextHop &nh);

  size_t erase(int dstId, int nhId);

private:

  void checkInputs();

  void createEmptyFib();

private:
	const int nodeId; // Own node id
	const std::string nodeName; // Own node name
	const int numNodes; // = numDsts.
	const int nodeDegree;
	const Ptr<GlobalRouter> ownRouter;

	int upwardCounter;
	int totalNhCounter;

	// DstId -> set<FibNextHop>
	unordered_map<int, std::set<FibNextHop>> perDstFib;
	unordered_map<int, std::set<FibNextHop>> upwardPerDstFib;

	friend std::ostream& operator<<(std::ostream&,  const AbsFib &fib);
};

// Output operator.
std::ostream &operator<<(std::ostream &os, const AbsFib &fib);

} // namespace ndn
} // namespace ns-3

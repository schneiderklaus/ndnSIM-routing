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

#include <cassert>
#include <unordered_set>

namespace ns3 {
namespace ndn {

using ::std::unordered_set;

constexpr int NODE_ID_LIMIT = 1000;
constexpr long MAX_COST = 1 * 1000 * 1000;

enum class NextHopType
{
	DW, UPWARD, DISABLED
};

class FibNextHop
{
public:
	// Cost has to be larger than 0!
    FibNextHop(int cost, int nhId, int costDelta = -1, NextHopType type =
        NextHopType::DISABLED);

// Getters
	// Order of FibNexthop:
	bool operator<(const FibNextHop &other) const;

	bool operator==(const FibNextHop &other) const {
	  if (other.m_nhId == m_nhId){
	    assert(other.m_cost == m_cost);
	    assert(other.m_costDelta == m_costDelta);
	    return true;
	  }
	  else {
	    return false;
	  }
	  //return other.nhId == nhId;
	}

	int getNhId() const {
		return m_nhId;
	}

	int getCost() const {
	  return m_cost;
	}

	int getCostDelta() const {
	  return m_costDelta;
	}

	NextHopType getType() const {
	  return m_type;
	}

	unordered_set<int> getPath() const {
		return path;
	}


// Setters:
  void setType(const NextHopType& newType) {
    assert(newType != NextHopType::DISABLED);
    this->m_type = newType;
  }

  // Only used in old fillFib:
  void setCost(const int newCost, const int newCostDelta) {
    assert(newCost > 0);
    assert(newCostDelta >= 0);
    this->m_cost = newCost;
    this->m_costDelta = newCostDelta;
  }

private:
	int m_cost;
	int m_nhId;
	NextHopType m_type;
	int m_costDelta;
	unordered_set<int> path;

	friend ::std::ostream& operator<<(::std::ostream&, const FibNextHop &fib);
};


std::ostream& operator<< (std::ostream &os, const NextHopType &type);
std::ostream& operator<< (std::ostream &os, const FibNextHop &a);


} // namespace ndn
} // namespace ns-3


namespace std {
template<>
struct hash<ns3::ndn::FibNextHop>;
}


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

#include "FibNextHop.hpp"

#include "AbsFib.hpp"

namespace ns3 {
namespace ndn {

FibNextHop::FibNextHop(int cost, int nhId, int costDelta, NextHopType type) {
  assert(cost >= 0);
  assert(cost <= MAX_COST);
  assert(nhId >= 0 && nhId <= NODE_ID_LIMIT);

  this->m_nhId = nhId;
  this->m_cost = cost;
  this->m_type = type;
  this->m_costDelta = costDelta;
  this->path = {};
}

// Order of FibNexthop:
bool
FibNextHop::operator<(const FibNextHop &other) const {
    assert(m_nhId != -1);

    return std::tie(m_costDelta, m_cost, m_nhId)
      < std::tie(other.m_costDelta, other.m_cost, other.m_nhId);

    // Order Nexthops: Cost, Type, Id.
//      return std::tie(cost, type, nhId) < std::tie(other.cost, other.type, other.nhId);
}

std::ostream& operator<< (std::ostream& os, const NextHopType &type)
{
    switch (type)
    {
        case NextHopType::DW : return os << "DW" ;
        case NextHopType::UPWARD : return os << "UPWARD" ;
        case NextHopType::DISABLED : return os << "DISABLED" ;
        // omit default case to trigger compiler warning for missing cases
    };
    return os << static_cast<std::uint16_t>(type);
}


std::ostream& operator<<(std::ostream &os, const FibNextHop &a) {
  return os << "Id: " << a.getNhId() << ", cost " << a.m_cost << ", type:" << a.m_type;
}

} // namespace ndn
} // namespace ns-3


namespace std {

using ns3::ndn::FibNextHop;

template<>
struct hash<FibNextHop>
{
    size_t
    operator()(const FibNextHop& k) const {
        // Compute individual hash values for two data members
        // and combine them using XOR and bit shifting
        return ((hash<int>()(k.getNhId()) ^ (hash<int>()(k.getCost()) << 1)) >> 1);
    }
};

}


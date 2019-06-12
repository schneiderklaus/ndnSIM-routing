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

#include <boost/graph/adjacency_list.hpp>

#include "AbsFib.hpp"

namespace ns3 {
namespace ndn {

class ArcMap;
using AllNodeFib = std::unordered_map<int, AbsFib>;

// No Vertex Property
// Edge Property: Weight.
using DiGraph = boost::adjacency_list<boost::listS, boost::vecS, boost::bidirectionalS,
    boost::no_property, boost::property<boost::edge_weight_t, int>>;

void getDigraphFromFib(DiGraph &dg, const AllNodeFib &allNodeFIB, const int dstId);

int removeLoops(AllNodeFib &allNodeFIB, bool printOutput = true);

int removeDeadEnds(AllNodeFib &allNodeFIB, bool printOutput = true);

} // namespace ndn
} // namespace ns3
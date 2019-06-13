#include "ns3/double.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/uinteger.h"

#include "../helper/ndn-app-helper.hpp"
#include "../helper/ndn-stack-helper.hpp"
#include "../helper/ndn-global-routing-helper.hpp"
#include "../helper/ndn-strategy-choice-helper.hpp"
#include "../model/ndn-net-device-transport.hpp"
#include "../model/ndn-l3-protocol.hpp"
#include "../NFD/daemon/fw/random-strategy.hpp"
#include "../utils/topology/annotated-topology-reader.hpp"

namespace ns3 {

void displayRoutes(const NodeContainer& allNodes, std::string prefix){
  for (const auto& n : allNodes) {
    const auto& fib = n->GetObject<ndn::L3Protocol>()->getForwarder()->getFib();
    auto& e = fib.findLongestPrefixMatch(prefix);

    std::cout << "Node " << n->GetId() << ", prefix: " << prefix << "\n";

    for (const auto& nh : e.getNextHops()) {
      // Get remote nodeId from face:
      const auto& transport = dynamic_cast<ndn::NetDeviceTransport*>(nh.getFace().getTransport());
      if (transport == nullptr) continue;

      const auto& nd1 = transport->GetNetDevice()->GetObject<PointToPointNetDevice>();
      if (nd1 == nullptr) continue;

      const auto& ppChannel = DynamicCast<PointToPointChannel>(nd1->GetChannel());
      if (ppChannel == nullptr) continue;

      auto nd2 = ppChannel->GetDevice(0);
      if (nd2->GetNode() == n) nd2 = ppChannel->GetDevice(1);

      std::cout << "NextHop: " << nd2->GetNode()->GetId() << ", cost: "
          << nh.getCost() << "\n";
    }
    std::cout << "\n";
  }
}

int
main()
{
  AnnotatedTopologyReader topologyReader{};
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/topo-abilene.txt");
  topologyReader.Read();

  ndn::StackHelper stackHelper {};
  stackHelper.SetDefaultRoutes(false);
  stackHelper.InstallAll();

  // IMPORTANT: Has to be run after StackHelper!
  topologyReader.ApplyOspfMetric();

  const std::string prefix {"/prefix"};

  const NodeContainer allNodes {topologyReader.GetNodes()};
  const auto& consumerN {allNodes.Get(0)};
  const auto& producerN {allNodes.Get(10)};

  ndn::GlobalRoutingHelper routingHelper {};
  routingHelper.InstallAll(); // Fills GlobalRouter with incidencies.
  routingHelper.AddOrigin(prefix, producerN);

  /// NOTE: Select one of the following 3: ///
//  routingHelper.CalculateRoutes();
//  routingHelper.CalculateAllPossibleRoutes();

  routingHelper.CalculateLFIDRoutes();

  ndn::StrategyChoiceHelper str;
  str.InstallAll<nfd::fw::RandomStrategy>("/");

  displayRoutes(allNodes, prefix);

  // Installing applications
  ndn::AppHelper consumerHelperX {"ns3::ndn::ConsumerCbr"};
  consumerHelperX.SetPrefix(prefix);
  consumerHelperX.SetAttribute("Frequency", DoubleValue(100.0));
  consumerHelperX.Install(consumerN);

  ndn::AppHelper producerHelper0 {"ns3::ndn::Producer"};
  producerHelper0.SetPrefix(prefix);
  producerHelper0.Install(producerN);

  Simulator::Stop(Seconds(30));
  Simulator::Run();
  Simulator::Destroy();

  std::cout << "\nSimulation finished!\n";
  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main();
}

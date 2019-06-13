# LFID (Loop-Free Inport-Dependent) Routing for ndnSIM

LFID (Loop-Free Inport-Dependent) Routing extends the ndnSIM route calculation to provide more and shorter loop-free paths than existing work. For details see the [tech report.](https://named-data.net/publications/techreports/mp_routing_tech_report/)

LFID provides a much better trade-off than the existing route calculation algorithms:

1. CalculateRoutes(): Only provides a single shortest path nexthop. 
2. CalculateAllPossibleRoutes(): Provides all possible nexthops, but many of them lead to loops. 

LFID, on the other hand, maximizes the nexthop choice while also completely avoiding loops.


## Installation

The installation procedure is straight forward and similar to the one of [ndnSIM](https://ndnsim.net/2.7/getting-started.html):

### Prerequisites

First install all dependencies from https://ndnsim.net/2.7/getting-started.html

### Compiling the Code

```bash
git clone https://github.com/named-data-ndnSIM/ns-3-dev.git ns-3
git clone --recursive https://github.com/schneiderklaus/ndnSIM-routing ns-3/src/ndnSIM

cd <ns-3-folder>
./waf configure --enable-examples
./waf
```


## Example Experiments

I provide an example 3x3 grid topology to compare against the existing route calculation methods. Simply uncomment one of 
- routingHelper.CalculateRoutes();
- routingHelper.CalculateAllPossibleRoutes();
- routingHelper.CalculateLFIDRoutes();
 
in the [ndnSIM/examples/grid.cpp](examples/grid.cpp) file, then run:

```bash
./waf --run grid
```

The output will show the nexthops at each node for destination node 8, and any loops during forwarding.


## Changes in Forwarding strategy 

LFID requires that the forwarding strategy always excludes the incoming face from the outgoing options. I provide an example implementation in [ndnSIM/NFD/daemon/fw/random-strategy.cpp](NFD/daemon/fw/random-strategy.cpp).



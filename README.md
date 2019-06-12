# Implementation of LFID (Loop-Free Inport-Dependent) Routing

LFID (Loop-Free Inport-Dependent) Routing extends the ndnSIM route calculation to provide many more loop-free paths/nexthops than existing work. The only constraint is that the forwarding strategy has to exclude the incoming interface at each router.

This provides a much better trade-off than the existing route calculation algorithms:

1. CalculateRoutes(): Only provides a single shortest path nexthop. 
2. CalculateAllPossibleRoutes(): Provides all possible nexthops, but many of them lead to loops. 
 

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

I provide an example to compare the route calculation methods in the **ndnSIM/examples/grid.cpp** file. Simply uncomment one of 
- routingHelper.CalculateRoutes();
- routingHelper.CalculateAllPossibleRoutes();
- routingHelper.CalculateLFIDRoutes();
 
then run:

```bash
./waf --run grid
```

The output will show the nexthops at each node for destination node 8, and any loops during forwarding.



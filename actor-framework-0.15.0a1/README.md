# NF Actor Library

The NF actor library is based on libcaf https://github.com/actor-framework/actor-framework

# An Overview of NF Actor

Traditional NFV system usually runs network function software on a virtual machine/container (we refer to this as NF VM). The network operator usually launches multiple NF VMs to construct a service chain. The NF VMs are usually controlled by a management system. Input packets are sent to their true destination after they have traversed all the NF VMs on their service chain. 

A NF VM could be viewed as a black-box from the perspective of a management system. All the packets sent to a NF VM are usually processed by a single network function. When a NF VM finishes processing the packet, the NF VM sends the packet to the next NF VM for further processing. The packet is sent to its true destination after it has traversed all the NF VMs on its service chain.

One of the problem with the current NFV architecture is that it lacks resilience. Achieving flow migration and fault tolerance functionalities in existing NFV systems are tedious and hard, due to the following reasons.

* __1__. NF VM is built using the abstraction of running one network function software on one virtual machine/container. From the perspective of the NFV management system, NF VMs could be viewed as black-boxes. The NFV management system could only launch/shutdown NF VMs and direct traffic flows through different NF VMs, but it is hard for NFV management system to access to the internal information (i.e. flow states, shared states) of each NF VM. Different NF VMs also treat each other as black-boxes and it is hard for them to interact with each other. We can see that there lacks dedicated and effective communication channel between different NF VMs and the management system. 

* __2__. There is no clean separation between the core processing logic of the network function software and the flow states. This problem complicates the manipulation of flow states (the manipulation may include copying and serializing flow states), which is a crucial task for resilience. 

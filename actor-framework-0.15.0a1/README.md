# NF Actor Library

The NF actor library is based on libcaf https://github.com/actor-framework/actor-framework

## An Overview of NF Actor

Traditional NFV system usually runs network function software on a virtual machine/container (we refer to this as NF VM). The network operator usually launches multiple NF VMs to construct a service chain. The NF VMs are usually controlled by a management system. Input packets are sent to their true destination after they have traversed all the NF VMs on their service chain. 

A NF VM could be viewed as a black-box from the perspective of a management system. All the packets sent to a NF VM are usually processed by a single network function. When a NF VM finishes processing the packet, the NF VM sends the packet to the next NF VM for further processing. The packet is sent to its true destination after it has traversed all the NF VMs on its service chain.

One of the problem with the current NFV architecture is that it lacks resilience. Achieving flow migration and fault tolerance functionalities in existing NFV systems are tedious and hard, due to the following reasons.

* __1__. NF VM is built using the abstraction of running one network function software on one virtual machine/container. From the perspective of the NFV management system, NF VMs could be viewed as black-boxes. The NFV management system could only launch/shutdown NF VMs and direct traffic flows through different NF VMs, but it is hard for NFV management system to access to the internal information (i.e. flow states, shared states) of each NF VM. Different NF VMs also treat each other as black-boxes and it is hard for them to interact with each other. We can see that there lacks dedicated and effective communication channel between different NF VMs and the management system. 

* __2__. There is no clean separation between the core processing logic of the network function software and the flow states. This problem complicates the manipulation of flow states (the manipulation may include copying and serializing flow states), which is a crucial task for resilience. 

We propose to use actor programming model to construct resilient NFV software, which effectively overcomes the obstacles of the traditional NFV system. We call the system NF actor system.

### NF Actor Architecture

* __1__. In NF actor system, instead of directly running network function software, a virtual machine/container runs a NF runtime system. 

* __2__. The NF runtime system creates several NF actors for each flow. Each NF actor is created using an implementation of a network function. All the NF actors created for a flow act as the service chain. 
 
* __3__. Compared with a virtual machine, a NF actor is a logical thread running inside NF runtime system (virtual machine). It is extremely lightweight and can be dynamically created and configured to suit different service chain policies.

* __4__. Different NF actors and NF runtime systems can communicate with each other through efficient message passing.

* __5__. NF actor system uses a typical controller-worker nodes architecture, which is extensively used among existing NFV systems. But the tasks related to resilience are mostly done in a distributed way. 

### APIs for Implementing Network Function

* __6__. The NF actor system exposes a set of APIs for programmers to implement network function. This set of APIs hides the implementation detail of a NF actor. Programmer using this API only needs to focus on the core processing logic of the network function. 

### Resilient design of NF actors

* __7__. The NF actor implements all the mechanism for fault tolerance and flow migration. An NF actor that loads a network function could automatically be migrated among different NF runtime systems and be replicated.

* __8__. The resilience design makes full use of an actor system. Both flow migration and fault tolerance could be executed independently by NF actors and NF runtime systems, with minimal assistance from the centralized controller (only basic view service is required). This is unlike existing systems, where centralized controller has an important role in resilience operations.

* __9__. The resilience operation in NF actor system is lightweight and fast.    

### Sample Use case:

Basic NF actor interface: Please see the psudo_interface/interface.cpp file in this. The file includes pseudo code for important base classes of NF actor system. 

Implement network functions: Please see the psudo_interface/use_case.cpp file. We give a demo about how to implement network functions using the base classes provided in interface.cpp file. We can see that the programmer only needs to concentrate on implementing the core logic of a network function. The complicated details of replication and migration are hidden from the programmer.

Environment set up: 

* __1__. Use a server cluster. Servers are connected through either a SDN switch or software switch running in a server. (NFV system needs flexible programmable switching environment.) 

* __2__. Create several virtual machines on the server, launch NF runtime system on these virtual machines. 

* __3__. Each NF runtime system is assigned a unique identifier. Each NF runtime system loads the service chain policies from the controller. 




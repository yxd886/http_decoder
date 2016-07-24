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

NF actors:

* __1__. How to create NF actors. The NF runtime system implements a polling thread, which create NF actors of a service chain upon the first arrival of a flow packet. (Refer to interface.cpp for the definition of service_chain)

``` cpp
//The NF runtime system should implement a polling thread, which
//constantly get packets from the NIC

void poll_loop(){
    //poll the packet from the 
    packet* p = poll_nic();

    //check if there is a service chain
    //to serve packet p
    if(service_chain_exist(p)){

        //obtain the serivce chain
        service_chain sc = get_service_chain(p);

        //this enqueues the packet to the 
        //mailbox of the first NF actor on the service chain
        sc.process_packet(p);
    }
    else{

        //create the service chain according to the policy
        service_chain sc(get_policy(p));

        //create NF actors of the service chain and enqueue the 
        //actor to the first NF actor.
        sc.init(nf_runtime_system, p);
    }
}
```
* __2__. Each NF actor processes the received packet using a message handler. (Refer to the interface.cpp file for packet_processing_handler)

``` cpp
//handler used by the nf_actor to process the packet
auto packet_processing_handler = [=](packet *p){
    //create a copy of the received packet
    //because the packet may be modified during processing
    packet* copy_p = copy(p);

    //use the network_function to process the packet
    action a = nf_ptr_->process_packet(p, fs);

    //execute the action returned after processing the packet
    a(rt_sys_, sc, nf_ptr_);

    //perform replication
    replicate(copy_p);
};
```

* __3__. Replication is automatically handled by the NF actor system when processing packets. A replication target is selected by consulting the view service. (Refer to the interface.cpp file for replicate)

``` cpp
//a member function that handles replication
void replicate(packet* p){
    if( (replicate_on_id == -1) || 
        (rt_sys_.is_failed(replicate_on_id)) ){

        //obtain a runtime system node to perform replication
        replicate_on_id = rt_sys_.get_replication_node();
    }

    //perform replication
    do_replicate(p, fs);
}
```

* __4__. Flow migration is defined as a set of behaviors of an actor. So that the flow migration process becomes a distributed process, with minimal requirement for central control. It only needs to contact view service for a proper migration target. (Refer to the interface.cpp file for all the behaviors related to migration)

## Workload Aware NF Actor Replication

Motivation:
What existing NFV system do: Existing NFV system uses NF VM abstraction. In order to create a checkpoint for the current state of a NF VM, the entire memory image of the NF VM must be checkpointed, which is a costly operation and can not be performed frequently. Existing NFV system also uses dedicated servers to store checkpoints and logs, which may waste resource.

What actor abstraction bring us:  NF actors are lightweight logical thread, which could be created and destroyed flexibly at small cost. Replicating a NF actor is an extremely lightweight operation compared with replicating the entire memory footprint of a virtual machine. Besides, different NF actors are independent with each other, so their states could be replicated separately at different locations. These features may lead to a unique design choice, which is replicating NF actors on other NF runtime systems running in the same cluster. This design choice simplifies the structure of a NFV system, as we don’t need a dedicated replication server. It may also improve the resource utilization rate, as “replication servers” could also act as network functions to process input flow packets.

Design:

* __1__. In order to facilitate the NF actor replication process, the APIs exposed to the network function programmer is designed as the following (refer to interface.cpp file). The process_packet function forces the programmer to store the flow state inside a  flow_state object. This interface enforces a complete separation between the processing logic and the flow state. Taking a snapshot of flow state is just calling the serialize() member function on the flow_state object. This API makes replicating flow state easy.

``` cpp
//The network_function is created by nf_runtime_system, and passed to different
//nf_actor.
class network_function {
public:

    //The constructor of nf_actor uses this function to get a flow_state
    virtual flow_state new_flow_state() = 0;

    //The packet handler, which implement the core logic of the network function.
    //Programmers need to implement their own version in the derived class.
    //In process_packet, it is required that all flow-related state are stored 
    //in fs argument. This member function returns a action functor. Caller of 
    //process_packet should call the action functor.
    virtual action process_packet(packet* p, flow_state& fs) = 0;

    //programer could define arbitrary shared_state here
};
```

* __2__. Using the actor abstraction enables us to replicate individual NF actor state instead of replicating the entire memory footprint of a virtual machine, making the replication process lightweight and fast. Also, the replication process of a NF actor could be made independent with the replication process of other NF actors.

Due to the lightweight, independent NF actor replication, we choose a replication design that replicate NF actors on other NF runtime systems available in the cluster.

For each NF actor, we perform replication after the actor has finished processing the packet in the actor’s packet processing handler. The actor replicate its state and packet logs on to one of the NF runtime system with the smallest workload. The replication process is described with the following pseudo code (refer to interface.cpp file).

``` cpp
//a member function that handles replication
void replicate(packet* p){
    if( (replicate_on_id == -1) || 
        (rt_sys_.is_failed(replicate_on_id)) ){

        //obtain a runtime system node to perform replication
        replicate_on_id = rt_sys_.get_replication_node();
    }

    //perform replication
    do_replicate(p, fs);
}


//handler used by the nf_actor to process the packet
auto packet_processing_handler = [=](packet *p){
    //create a copy of the received packet
    //because the packet may be modified during processing
    packet* copy_p = copy(p);

    //use the network_function to process the packet
    action a = nf_ptr_->process_packet(p, fs);

    //execute the action returned after processing the packet
    a(rt_sys_, sc, nf_ptr_);

    //perform replication
    replicate(copy_p);
};
```

The benefits is that, first we can decrease the complexity of the NFV system by removing the dedicated replication server, second we can improve resource utilization. The scalability of the NFV system is also preserved. There is no hot spot in NF actor system and NF actor system could scale-out by creating more NF runtime systems.

* __3__. During recovery, the node under recovery pulls the replicated NF actor state and packet logs from all the NF runtime systems and recover to the state before the failure.

## Faultless and lossless flow migration

Motivation: 
What existing NFV system do: The flow migration process is usually controlled by a centralized controller. The centralized controller and the NF VM need to exchange many messages in order to correctly migrate a single flow. The current design is not scalable. And the design does not consider some of the potential failures which may happen during the migration process. 

What NF actor system can do: Using the actor abstraction, different actors and NF runtime system have effective communication channel through message passing. We can build the migration process as a set of message handlers for each NF actors, turning the migration of NF actors a fully distributed operation (The actor being migrated needs to consult the view service at the beginning of the migration only once though) without the assistance from a centralized controller. 

We build the migration process using only 3 request-response pairs passing in the network (2, 4, 5, 6, 7, 9 in the state transition graph). The migration process could be batched to improve performance. And we fully consider different kinds of failures that may happen during the migration process, including message lost, hardware/software crashes and buffer overflows.

Design:

* __1__. How migration is started (taking scale-out as an example, scale-in is similar): The NF actor runtime system monitors the resource usage. Taking CPU usage as an example, once the CPU usage exceeds MIGRATION_START threshold,  the runtime system start migrating its NF actors out to other NF runtime systems. The migration is stopped once the CPU usage is below MIGRATION_START threshold. The NF runtime system selects some actors and send these actors migration_start_atom message. This message will be trapped by the a message handler, and the migration process is automatically started. See the following code from interface.cpp.

``` cpp
//handler when nf_runtime_system send a migration start message to the
//nf_actor 
auto migration_msg_handler = [=](migration_start_atom){
    //obtain the id of the runtime system to migrate to
    migrate_to_id = rt_sys.get_migration_node(*this);

    if(migration_to_id != -1){
        //get a valid migration target
        
        //send prepare_to_buffer message and the flow signature of this actor
        //to the migration target
        send(rt_sys_.obtain_contact_actor(migrate_to_id),
            prepare_to_buffer_atom::value,
            obtain_flow_signature());

        //change the behavior
        become(wait_prepare_to_buffer_response());
    }
    
    //if can't get any valid migration target,
    //the nf_runtime_system will try to notify 
    //this actor sometime in the future.
}
```

* __2__. The migration process could be briefly described with the following procedure. The pseudo code of interface.cpp includes all the behaviors during flow migration. It includes different error handling mechanism to handle message loss, failure and buffer overflows.

1. The NF runtime system 1 sends a start migration message to the NF actor being migrated.
2. The NF actor being migrated send a prepare-to-buffer message and its flow signature to NF runtime system 2
3. The NF runtime system 2 create a migrate actor which is used to accept the migration. 
4. The NF runtime system 2 responds to the actor being migrated with an acknowledgement message and the actor reference of the migrate actor
5. The NF actor notifies the switch to start sending a copy of the flow packet to NF runtime system 2. In the meantime, the migrate actor will buffer all the received packets without processing them
6. The switch responds a start state transfer message to the NF actor being migrated.
7. The NF actor being migrated stops processing flow packets, only buffers them. And it sends its flow state to the migrate actor
8. The migration actor saves the received flow state and start taking the role of the original NF actor. 
9. The migrate actor responds the NF actor being migrated with a migration succeed message.
10. The NF actor being migrated notifies the switch to stop sending any more flow packets to itself and quit.

## View service of NF actor runtime system

Motivation:
Resilient NFV is a distributed system running in a rack of servers. The servers are interconnected using a high speed switch. Each server has multiple virtual machines/ containers. Each virtual machine/container has a NF actor runtime system running on it. 

Since resilient NFV needs multiple NF actor runtime systems to coordinate with each other when replicating NF actor states and migrating NF actors, it needs a view service so that every NF runtime system knows the existence of other NF runtime systems.

Besides knowing the existence, the NF actor runtime system also needs to know the resource usage and failure condition of other NF actor runtime systems. All this information should be provided by the view service.

Design: 
Use a master-slave strategy to manage the view. Launch one master node and multiple slave nodes. The master node handles the view service and assists the launching/destroying of slave nodes. Each slave node runs a NF actor runtime system. 

The interaction between master node and slave nodes is listed in the following

Initialization:
On the server rack, create a virtual machine/container running master node process.
On the server rack, create several virtual machine/containers running slave node processes (NF actor runtime system), passing the IP address of the master node as initialization argument.

Slave node join:
New slave node performs basic initialization, including registering itself with a virtual switch on the server. 
The new slave node sends a JOIN message to the master node. The JOIN message contains an contactable IP address of the slave node.
When master node receives JOIN message from a slave node, it assigns a unique identifier to the slave node. The master node appends the identifier and IP address of the new slave node to the JOIN message and publishes the JOIN message to all the other slave nodes. Then the master node appends the identifiers and IP addresses of all the other slave nodes to the JOIN message and sends it back to the new slave node.
For each of the other slave nodes, it learns about the joining of the new slave node. For the slave node, it learns about the existence of all the other slave nodes. 

Slave node heartbeats:
When the new slave node receives the identifier, it starts to generate regular HEARTBEAT message to the master node every HEARTBEAT_INTERVAL seconds. The HEARTBEAT message contains the identifier and resource usage. The resource usage contains CPU usage and number of active NF actors running on this slave node. It is used to indicate the workload on this slave node. 
When master node receives HEARTBEAT message from slave node A, it publishes it to all other slave nodes except A. Then the master sends a HEARTBEAT ACK message back to slave node A.

Slave node normal leave:
The normal leave process could be designed as  a 2-stage interaction so that other slave nodes could choose new replication node. But is this really needed?
First stage: prepare to leave 
Slave node prepares to leave the system (it has low workload.). It stops to accept new migration offer from other slave nodes. It also stops to accept replication messages from other slave nodes. It also deregisters itself from the virtual switch, so that no new flows will be routed to the leaving slave node. It starts to migrate all running NF actors to other slave nodes. It sends a PREPARE TO LEAVE message to master node.
Master node publishes the message to all other slave nodes, so that these slave nodes could stop using the leaving slave node to replicate actor state, or migrating NF actors to the leaving slave node. Then the master node sends an PREPARE TO LEAVE ACK message back to the leaving slave node.
The leaving slave receives the PREPARE TO LEAVE ACK message, it waits until there are no active NF actors and no NF actor replications. 
  Second stage: complete the leave.
The leaving slave node sends a LEAVE message to the master. 
The master node deletes the identifier of the slave node. The master node broadcasts the LEAVE message to all the other slave nodes. Then the master node shutdown the leaving slave node.

Slave node failure:
The master node fails to receive HEARTBEAT message from a node for FAILURE_DETECTION seconds, it tags the slave node to be failed. It publishes a FAILURE DETECTED message to the rest of the slave nodes. The message contains the unique identifier of the failed node. 
When slave node receives FAILURE DETECTED message, the slave node marks the failed node as FAILED. For all the NF actors that are replicated on the failed node, the slave node finds a new target node to replicate the NF actors. 
The master node launches a new slave node, passing the identifier of the failed node as initialization arguments. The slave node is launched in failure recovery mode. After initialization, it sends a FAILURE JOIN message to the master after initialization.
The master node assigns a new identifier to the recovery node. It first publishes the old identifier and new identifier to the rest of the nodes. So that the slave nodes who own replication of recovery node could start transmitting replicated NF actors back to the recovery node. Then the master node sends a FAILURE JOIN ACK message back to the recovery node, containing the view of all the other slave nodes inside the cluster.
When a slave node finishes transmitting all the replicated NF actors, it sends a REPLICATION FINISH message back to the recovery node. When recovery node receives all the messages, it registers itself with the virtual switch to accept new traffic.

Master node failure:
The state of master node is persisted on the disk to prevent failure.




//To use nf actor, the network function software must be rewritten. Here
//we give a simple demo about how a network function should be implemented
//using nf actor framework.

//1. programmer should create a derived class by inheriting 
//network_function, and create implementions for the 2 virtual functions.

class nf1 : public network_function{
	
	flow_state new_flow_state() override{
		
		//implementation...
		nf1_flow_state fs;
		return fs;
	};
	
	action process_packet(packet* p, flow_state& fs) override{
		//implementation...
		nf1_action a(p, fs);
		return a;
	};

	//add the shared states here
	nf1_shared_state1 ss1;
	nf1_shared_state2 ss2;
};

//2. create a unique customized flow_state class for nf1. 

class nf1_flow_state : public flow_state {

	serialized_obj serialize() override;

	//...
}; 

//3. create several shared_state class for nf1

class nf1_shared_state1 : public shared_state{

	serialized_obj serialize() override;

	//...
};

class nf1_shared_state2 : public shared_state{

	serialized_obj serialize() override;

	//...
};

//4. create an action after finishing processing the packet

class nf1_action : public action{
	void operator() (nf_runtime_system& rt_sys, service_chain& sc, network_function* nf_ptr) override;
};

//5. following step 1 to 4, create multiple network function class

class nf2 : public network_function{
	//..
};

class nf3 : public network_function{
	//..
};

//6. Then the network operator could create a service chain policy, 
//describing what flow should use what service chain

service_chain_policy<flow_signature, nf1, nf2, nf3> scp;



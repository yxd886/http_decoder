/*
The service_chain indicates the service chain of a flow.
It is created when the first packet of a flow is created. 
*/
class service_chain {
private:

	//The sc_vec_ field contains pointers of all the network function actors
	//on this service chain. When a network function actor finishes processing
	//the packet, it can use sc_vec_ to find out the next actor along the
	//service chain.
	std::vector<nf_actor*> actor_vec_;

	//The nf_vec_ filed indicates the network functions along the service chain.
	//Each nf_actor is created using one of the network_function object.
	std::vector<network_function*> nf_vec_;

public:

	//constructor, the nf_runtime_system passes the service chain structure
	//of the flow to nf_vec_
	service_chain(std::vector<network_function*> nf_vec):nf_vec_(nf_vec){};

	//The service_chain object for a new flow is created when the first pakcet
	//of the new flow is received, 
	//create all the nf_actor for the service chain and pass a initial packet
	//to the nf_actor
	void init(nf_runtime_system& rt_sys, packet* first_packet){
		for(auto& nf_ptr : nf_vec_){

			//create a nf_actor using runtime system, push it to actor_vec_
			actor_vec_.push_back(rt_sys.spawn(*this, nf_ptr));
		}

		//passes the first packet of the flow to the service chain
		rt_sys.external_enqueue(actor_vec_[0], first_packet);
	}
};

/* 
A nf_actor has only one flow_state object. The packet processing
handler of nf_actor's network_network modifies the flow_state object.
*/
class flow_state{
public:

	//Member function for serializing the flow state, used by flow migration 
	//and state replication. User must define their own versionof flow_state
	//for a network function by inheriting the flow_state base class.
	//The user defined derived class contain all the per-flow state
	//information used by the network function.
	virtual serialized_obj serialize() = 0;

	//Arbitrary flow states and handler function could be added in the user
	//defined derived class
};

/*
Each network_function has several shared_state. The shared_state may be
updated when network_function's packet handler is called. It is protected
by a mutex
*/
class shared_state{
public:

	//Depending on what replication strategy we choose, we may not need
	//to serialize shared_state and replicate it.
	virtual serialized_obj serialize() = 0;

private:

	//a mutex protecting this shared_state
	mutex_t m_;

	//users can define arbitrary shared data in the derived class.
	//network_function can contain multiple shared_satte.
};

/*
The action object is returned from the packet handler of the network_function 
object. The nf_actor calls the action functor to execute some post-packet
processing, which may include forwading packet to the next actor along the 
service chain, sending the packet out to the real destination, spawning some
other nf_actors, or passing somee messages to some important actors.
*/
class action{
public:

	//overload () operator, making action into a functor
	//require nf_runtime_system and service_chain
	virtual void operator() (nf_runtime_system& rt_sys, service_chain& sc, network_function* nf_ptr) = 0;

	//the constructor
	explicit action(packet* p, flow_state& fs):p_(p), fs_(fs);

private:
	packet* p_;
	flow_state fs;
};

/*
The network_function is created by nf_runtime_system, and passed to different
nf_actor.
*/
class network_function {
public:

	//The constructor of nf_actor uses this function to get a flow_state
	virtual flow_state new_flow_state() = 0;

	//The packet handler, which implement the core logic of the network function.
	//Programmers need to implement their own version in the derived class.
	//In process_packet, it is requierd that all flow-related state are stored 
	//in fs argument. This member function returns a action functor. Caller of 
	//process_packet should call the action functor.
	virtual action process_packet(packet* p, flow_state& fs) = 0;

	//programer could define arbitrary shared_state here
};

/*
The nf_actor class. It is created by the nf_runtime_system.
*/
class nf_actor : public scheduled_actor{
public:
	nf_actor(nf_runtime_system& rt_sys, service_chain& sc, network_function* nf_ptr):
		rt_sys_(rt_sys), sc_(sc), nf_ptr_(nf_ptr){};

private:
	//nf runtime system
	nf_runtime_system rt_sys_;

	//service chain
	service_chain sc_;

	//network function
	network_function* nf_ptr_;

	//flow state associated with this nf_actor
	flow_state fs;

	//id of the runtime system to migrate to
	//if the actor is created as an actor to be migrated to
	//then migrate_to_id becomes the runtime system id of
	//the migration source
	int migrate_to_id = -1;

	//actor reference of the actor to be migrated to
	//if the actor is created as an actor to be migrated to
	//then migrate_actor becomes the actor referece of
	//the migration source
	actor& migrate_actor;

	//id of the runtime system to replicate on
	int replicate_on_id = -1;

	//a fifo queue used during migration to buffer
	//packets
	std::queue<packet*> buffer_q_;

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


	//the default nf_actor behavior, which handles the input packets,
	//and listens for the migration message sent from the nf_runtime_system
	behavior handle_packets(){
		return {packet_processing_handler, migration_msg_handler};
	}

	//wait for the response from the migration target
	behavior wait_prepare_to_buffer_response(){
		return {
			//receive acknowledge from the migration target
			//and the actor reference of the actor to be migrated to.
			//From now on, the migration process becomes a communication
			//process between local actor and migrate actor.
			//start to instruct the switch to send packet to 
			//migrate_actor
			[=](ack_atom, actor& migrate_actor){
			 	this->migrate_actor = migrate_actor;

			 	//notify the switch to start copying flow packets to 
				//the destination nf_runtime_system.
				rt_sys_.switch_copy_flow_to(obtain_flow_signature(), migrate_to_id);

				become(wait_migration_start_msg());
			},

			//the migration target reject the migration request,
			//probably because the migration target is preparing to
			//leave, notify the nf_runtime_system and revert to the 
			//default behavior
			[=](reject_atom){
				rt_sys_.prepare_to_buffer_reject(*this);
				migrate_to_id = -1;
				
				become(handle_packets());
			},

			//packets still come while waiting for the reply
			//need to process the packet
			packet_processing_handler,

			//after 10ms, the if still no response is received
			after(std::chrono::milliseconds(10)) >> [=] {
				//notify runtime system that the migration target is 
				//not responding
				rt_sys_.prepare_to_buffer_not_responding(*this);
				migrate_to_id = -1;
				
				//revert to the default behavior
				become(handle_packets());
			}
		};
	}

	behavior wait_migration_start_msg(){ 
		return {
			//receive start_migration_atom from the switch,
			//send the serialized flow state to the migrate_actor
			//wait for the response from migrate_actor
			[=](migration_start_atom::value){
				send(migrate_actor, fs.serialize(), replication_on_id);
				become(wait_migration_complete_msg());
			},

			//packets arrived at the nf_actor is processed as usual
			packet_processing_handler,

			//the start_migration message is not correctly received within
			//a timeout. cancel the migration. 
			after(std::chrono::milliseconds(10)) >> [=]{
				rt_sys_.switch_cancel_copy_flow(obtain_flow_signature(), migrate_to_id);
				rt_sys_.migration_start_msg_lost(*this);
				migrate_to_id = -1;

				//revert to the default packet handler
				beomce(handle_packets());
			}
		};
	}

	//this behavior waits for migration_complete message.
	//If this message is successfully received, then the migration
	//is successful.
	//If the message is not received within a timeout, or the
	//buffer queue overflows, we change the behavior into 
	//disaster saving
	behavior wait_migration_complete_msg(){

		//initialize a counter with 0 value
		this->counter = 0;
		return {
			[=](migration_complete_atom){
				//the migration is succesfully completed, notify
				//the switch to stop sending packets to this actor
				rt_sys_.switch_cancel_send_flow(obtain_flow_signature(), rt_sys_.get_id());
				rt_sys_.migration_succeed();
				//shutdown the actor
				quit();
			},

			//packets arrived at the nf_actor is buffered
			[=](packet* p){

				//only replicate the packet, not the state, the state won't
				//change here
				replicate_packet(p);
				
				//we append too many packet to the packet log,
				//can't go on, need to recover from the disaster
				this->counter += 1;
				if(this->counter > MAX_PACKET_LOG_LENGTH){
					save_disaster();
				}
			},

			//migration_complete message is not received within the
			//timeout, cancel the whole migration process
			after(std::chrono::milliseconds(10)) >> [=]{
				save_disaster();
			}
		};
	}

	//This is unlikely to happen. But if it happens, it is a
	//disaster to the flow migration process. 
	//If migrate_actor is correctly functioning, then we just 
	//delete the source actor.
	//If migrate_actor is not correctly functioning, then we 
	//rollback the flow state from the replica.
	void save_disaster(){
		auto do_local_recovery = [=]{
			rt_sys_.switch_cancel_copy_flow(obtain_flow_signature(), migrate_to_id);
			rt_sys_.do_local_recovery(*this);
			migrate_to_id = -1;
			recover(replicate_on_id);
		}

		if(rt_sys_.is_failed(migrate_to_id)){
			do_local_recovery();
		}
		else{
			
			//if the check fails, 
			on_sync_failure(do_local_recovery);

			//contact the nf_runtime_system for the state of the 
			//migrate_actor. If the migrate_actor is functioning,
			//just quit
			synchronized_send(rt_sys_.obtain_contact_actor(migrate_to_id), 
				query_actor_state_atom::value,
				migrate_actor).then(
				[=](normal_mode_atom){
					//the migrate_actor is correctly functioning, quit
					rt_sys_.switch_cancel_send_flow(obtain_flow_signature(), rt_sys_.get_id());
					rt_sys_.migration_succeed();
					quit();
				},
				[=](bad_mode_atom){
					do_local_recovery();
				},
				[=](packet* p){
					drop(p);
				}
			);
		}
	}

	//the following functions describes how the nf_actor should do during recovery
	//caused by the disaster
	void recover(int replication_on_id){
		on_sync_failure([=]{
			//the replica may fail, we have to abandon this flow
			//and quit
			rt_sys_.failed_to_recover(*this);
			quit();
		});

		sync_send(rt_sys_.obtain_contact_actor(replication_on_id), 
			acquire_replica_atom::value,
			obtain_flow_signature()).then(
			[=](flow_state fs, std::queue<packet*> packet_log){
				this->fs = fs;

				//rollback
				while(!packet_log.is_empty()){
					//use the network_function to process the packet
					action a = nf_ptr_->process_packet(packet_log.pop(), fs);

					//execute the action returned after processing the packet
					a(rt_sys_, sc, nf_ptr_);
				}

				//revert to default packet handler
				become(handle_packets());
			},

			//if receive packets, drop them.
			[=](packet* p){
				drop(p);
			}
		);
	}

	//The following behaviors describe what migrate_actor should be doing
	//migrate_actor is created by the nf_runtime_system with a special config
	//parameter, so that it is started with a different default behavior.

	//the nf_runtime_system on the migration target is responsible handling the 
	//prepare_to_buffer message sent from the source. After receiving this 
	//message, nf_runtime_system create the migrate_actor. Then the control is 
	//completely transfered to migrate_actor. The migrate_actor is initialized
	//with the following behavior.
	behavior wait_for_migration(){
		return{
			[=](pakcet* p){
				
				//buffer queue full, quit
				if(buffer_q_.is_full()){
					rt_sys_.notify_buffer_overflow(*this);
					quit();
				}

				//receive a packet, buffer
				buffer_q_.enqueue(p);
			},

			[=](flow_state& fs, int replication_on_id){
				//from the view of this actor, the actor that initiates the migration
				//is migrate_actor
				send(migrate_actor, migration_complete_atom::value);

				//correctly receive the state 
				this->fs = fs;
				this->replication_on_id = replication_on_id;
				
				//drain the buffer queue, by repeated calling
				//message handler.
				while(!buffer_q_.is_empty()){
					packet_processing_handler(buffer_q.pop());
				}

				become(handle_packets());
			},

			//the flow_state message is not correctly received within
			//a timeout. The primary reason for triggering this timeout is that
			//the source of migration fails.If the source of migration fails, 
			//the replication mechanism will bring the source actor back into
			// life. The second reason is that the flow_state message is lost.  
			//No matter what reason, shutdown this actor
			after(std::chrono::milliseconds(10)) >> [=]{
				rt_sys_.start_transfer_failed(*this);
				quit();
			}
		};
	}

};
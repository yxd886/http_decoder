#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "caf/all.hpp"
#include "caf/io/all.hpp"
#include <stdio.h>

#include <linux/unistd.h>     /* 包含调用 _syscallX 宏等相关信息*/
#include <linux/kernel.h>
using std::endl;
using std::string;


using namespace caf;
namespace {
using step_atom = atom_constant<atom("step")>;
using start_atom = atom_constant<atom("start")>;
using heartbeat_atom = atom_constant<atom("heartbeat")>;
using reconect_atom = atom_constant<atom("reconect")>;

string trim(std::string s) {
  auto not_space = [](char c) { return ! isspace(c); };
  // trim left
  s.erase(s.begin(), find_if(s.begin(), s.end(), not_space));
  // trim right
  s.erase(find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
  return s;
}


string getcpu()
{
    int pid = getpid();
    string t;
    t="ps -p "+std::to_string(pid)+" -o %cpu --no-headers"; 
    const char*s = t.c_str();
    FILE *pp = popen(s, "r");
    char tmp[1024]; //设置一个合适的长度，以存储每一行输出
    fgets(tmp, sizeof(tmp), pp);
        if (tmp[strlen(tmp) - 1] == '\n') {
            tmp[strlen(tmp) - 1] = '\0'; //去除换行符
        }
        string mem(tmp);
        mem=trim(mem);
       
        pclose(pp); //关闭管道
        return mem;
        
    
    
}

class client_actor : public event_based_actor{
public:
    client_actor(actor_config& cfg, const int& id,const int& conn_state, const string& host_,const uint16_t& port_,const actor& master_actor): event_based_actor(cfg),
   id(id),conn_state(conn_state),host_(host_),port_(port_),master_actor(master_actor){

 
    }
     //this actor should be created first on the service chain
    behavior make_behavior() override {
        //return firewall_fun(this);
     // send(this, step_atom::value);
    // philosophers start to think after receiving {think}
    return behavior{
      [=](step_atom) {
        if(1)
        {
          aout(this)<<"connection normal"<<endl;
          this->delayed_send(this, std::chrono::milliseconds(2000), step_atom::value);
          aout(this)<<"step message sent"<<endl;
          string cpu=getcpu();
          this->send(master_actor,heartbeat_atom::value,id,cpu);
          aout(this)<<"heartbeat message sent"<<endl;
          //conn_state=0;
        }else{
          aout(this)<<"try to reconnect"<<endl;
          this->send(this,reconect_atom::value);
        }

      },
      [=](heartbeat_atom) {
        aout(this)<<"receive heartbeat from master"<<endl;
        //conn_state=1;
        
      }/*,
      [=](reconect_atom) {
        aout(this)<<"reconnecting"<<endl;
        auto mm = system().middleman().actor_handle();
        send(mm, connect_atom::value, host_, port_);
        
      },
      [=](ok_atom, node_id&, strong_actor_ptr& new_server, std::set<std::string>&) {
        if (! new_server) {
          aout(this) << "*** received invalid remote actor" << endl;
          return;
        }
        aout(this) << "*** connection succeeded" << endl;
        master_actor = actor_cast<actor>(new_server);
      },
      [=](const error& err) {
        aout(this) << "*** could not connect to " << host_
                   << " at port " << port_
                   << ": " << system().render(err)
                   << " [try again in 2s]"
                   << endl;
        this->delayed_send(this,std::chrono::milliseconds(2000),reconect_atom::value);
      }*/
    };
  
  }

    int64_t id;
    int conn_state;
    string host_;
    uint16_t port_;

    actor master_actor;



    
    //std::regex reg;

    
};


class config : public actor_system_config {
public:
  int64_t id = 0;

  config() {
    opt_group{custom_options_, "global"}
    .add(id, "id,i", "id number");
  }
};

void caf_main(actor_system& system,const config& cfg) {
  // Read conf file.
  
  char ip[1000];
 std::ifstream fin("/home/sunmmer/actor/actor-framework/examples/worker.conf", std::ios::in);
  fin.getline(ip, sizeof(ip));

  std::cout<<"ip:"<<ip<<endl;

  // create a new actor that calls 'mirror()'
  // create another actor that calls 'hello_world(mirror_actor)';

  auto master_actor_ptr=system.middleman().remote_actor("localhost", 8888);
  std::cout<<"get remote actor"<<endl;
  auto master_actor=*master_actor_ptr;
  auto worker_actor=system.spawn<client_actor>(cfg.id,1,"localhost",8888,master_actor);
  anon_send(worker_actor,step_atom::value);
  fin.clear();
  fin.close();
  getchar();
  // system will wait until both actors are destroyed before leaving main
}
}
CAF_MAIN(io::middleman)

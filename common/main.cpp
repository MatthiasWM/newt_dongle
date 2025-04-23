

namespace NC {

// init list of endpoints
// init list of data and command buffers
// init list of pipes
// connect pipes as needed
// loop {
//   run the endpoint tasks in a wheel
//   endpoints can receive outside data and forward it to the connected pipe
//   endpoints can forward commands to the pipe or to the main task
//   the main task can disconnect and reconnect endpoints via pipe
//   it keeps global settings and saves and restores them as needed
// }

extern EndpointList endpoint_list;

void init() {
  for(auto &ep: endpoint_list) {
    ep.init();
  }
}


int main() {
  init();
  for(auto &ep: endpoint_list) {
    ep.task();
  }
}

} // namespace NC

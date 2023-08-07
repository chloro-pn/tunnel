#include <cstdlib>
#include <iostream>
#include <numeric>
#include <string>

#include "asio.hpp"
#include "gflags/gflags.h"
#include "switcher/switcher.h"
#include "tunnel/event_count.h"

asio::ip::tcp::endpoint CreateEndPoint(const std::string& ip, uint16_t port) {
  return asio::ip::tcp::endpoint{asio::ip::address::from_string(ip), port};
}

DEFINE_string(ip, "127.0.0.1", "switcher server's ip");
DEFINE_int32(port, 12345, "switcher server's port");

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  using namespace tunnel;
  Switcher server(CreateEndPoint(FLAGS_ip, FLAGS_port));
  asio::io_context& ctx = server.GetIoContext();

  EventCount ec(1);
  asio::signal_set signals(ctx, SIGINT, SIGTERM);
  signals.async_wait([&](auto, auto) { ec.Succ(); });
  server.Start();
  ec.Wait();
  server.Stop();
  std::cout << "switcher stop" << std::endl;
  return 0;
}
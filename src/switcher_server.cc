#include <cstdlib>
#include <iostream>
#include <numeric>
#include <string>

#include "asio.hpp"
#include "gflags/gflags.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"
#include "switcher/switcher.h"
#include "tunnel/event_count.h"

asio::ip::tcp::endpoint CreateEndPoint(const std::string& ip, uint16_t port) {
  return asio::ip::tcp::endpoint{asio::ip::address::from_string(ip), port};
}

DEFINE_string(ip, "127.0.0.1", "switcher server's ip");
DEFINE_int32(port, 12345, "switcher server's port");
DEFINE_string(logfile, "switcher.log", "switcher server's logfile");

int main(int argc, char* argv[]) {
  spdlog::set_default_logger(spdlog::basic_logger_mt("basic_logger", "logs/" + FLAGS_logfile));
  spdlog::set_level(spdlog::level::info);

  gflags::ParseCommandLineFlags(&argc, &argv, true);
  using namespace tunnel;
  Switcher server(CreateEndPoint(FLAGS_ip, FLAGS_port));
  asio::io_context& ctx = server.GetIoContext();

  EventCount ec(1);
  asio::signal_set signals(ctx, SIGINT, SIGTERM);
  signals.async_wait([&](auto, auto) { ec.Succ(); });
  server.Start();
  SPDLOG_INFO("switcher server start");
  ec.Wait();
  server.Stop();
  SPDLOG_INFO("switcher server stop");
  return 0;
}
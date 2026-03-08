// IRC server entry point.
// Single-threaded, event-driven architecture using poll() for I/O multiplexing.
// All sockets are set to non-blocking mode; no threads or forking are used.
//
// Event loop contract:
//   1. poll() blocks until at least one fd is ready.
//   2. If the server socket fires: accept a new client connection.
//   3. If a client socket fires: recv() into per-client input buffer,
//      then parse complete lines and dispatch to command handlers.
//   4. After all events: flush each client's outbox via send().

#include <poll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <string>

#include "handlers/ServerState.hpp"
#include "handlers/State.hpp"
#include "handlers/Handlers.hpp"
#include "handlers/Dispatcher.hpp"
#include "handlers/CommandProcessor.hpp"
#include "handlers/SendHelpers.hpp"
#include "parser/Parser.hpp"

const int MAX_CLIENTS = 128;

const int RECV_BUFFER_SIZE = 4096;

int create_server_socket(int port) {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    std::cerr << "Error: socket() failed\n";
    return -1;
  }

  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  fcntl(server_fd, F_SETFL, O_NONBLOCK);

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port        = htons(static_cast<uint16_t>(port));

  if (bind(server_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
    std::cerr << "Error: bind() failed on port " << port << "\n";
    close(server_fd);
    return -1;
  }

  if (listen(server_fd, 16) < 0) {
    std::cerr << "Error: listen() failed\n";
    close(server_fd);
    return -1;
  }

  return server_fd;
}

void add_client_to_poll(struct pollfd fds[], int& nfds, int client_fd) {
  fds[nfds].fd      = client_fd;
  fds[nfds].events  = POLLIN;
  fds[nfds].revents = 0;
  nfds++;
}

void remove_client_from_poll(struct pollfd fds[], int& nfds, int& i) {
  close(fds[i].fd);

  fds[i] = fds[nfds - 1];
  nfds--;

  i--;
}

void handle_new_connection(int server_fd,
                           struct pollfd fds[], int& nfds,
                           State& stateOps) {
  int client_fd = accept(server_fd, NULL, NULL);
  if (client_fd < 0)
    return;

  fcntl(client_fd, F_SETFL, O_NONBLOCK);

  if (nfds >= MAX_CLIENTS + 1) {
    std::cerr << "Warning: max clients reached, rejecting fd=" << client_fd << "\n";
    close(client_fd);
    return;
  }

  stateOps.ensure_client(client_fd);

  add_client_to_poll(fds, nfds, client_fd);

  std::cout << "Client connected: fd=" << client_fd << "\n";
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: ./ircserv <port> <password>\n";
    return 1;
  }

  int port = atoi(argv[1]);
  if (port <= 0 || port > 65535) {
    std::cerr << "Error: invalid port number\n";
    return 1;
  }
  std::string password = argv[2];

  ServerState serverState("ircserv", password);

  State stateOps(&serverState);

  Parser parser;

  Handlers handlers;

  Dispatcher dispatcher;

  CommandProcessor processor;

  std::map<int, std::string> inbuf;

  int server_fd = create_server_socket(port);
  if (server_fd < 0)
    return 1;

  std::cout << "Server listening on port " << port << "...\n";

  struct pollfd fds[MAX_CLIENTS + 1];
  memset(fds, 0, sizeof(fds));

  fds[0].fd     = server_fd;
  fds[0].events = POLLIN;

  int nfds = 1;

  while (true) {
    int ret = poll(fds, static_cast<nfds_t>(nfds), -1);

    if (ret < 0) {
      continue;
    }
    if (ret == 0) {
      continue;
    }

    if (fds[0].revents & POLLIN) {
      handle_new_connection(server_fd, fds, nfds, stateOps);
    }

    for (int i = 1; i < nfds; i++) {
      int revents = fds[i].revents;

      if (revents == 0)
        continue;

      int client_fd = fds[i].fd;

      if (revents & (POLLIN | POLLERR | POLLHUP)) {
        char buf[RECV_BUFFER_SIZE];
        memset(buf, 0, sizeof(buf));

        int bytes = recv(client_fd, buf, sizeof(buf) - 1, 0);

        if (bytes <= 0) {
          std::cout << "Client disconnected: fd=" << client_fd << "\n";

          stateOps.remove_client_from_all_channels(client_fd);
          stateOps.remove_client(client_fd);
          inbuf.erase(client_fd);

          remove_client_from_poll(fds, nfds, i);
          continue;
        }

        inbuf[client_fd].append(buf, bytes);

        std::string& clientBuf = inbuf[client_fd];
        std::string::size_type pos;

        while ((pos = clientBuf.find("\r\n")) != std::string::npos) {
          std::string line = clientBuf.substr(0, pos);
          clientBuf.erase(0, pos + 2);

          if (line.empty())
            continue;

          processor.processLine(&serverState,
                                &dispatcher,
                                &handlers,
                                &parser,
                                client_fd,
                                line);

          if (serverState.clients.find(client_fd) == serverState.clients.end())
            break;
        }

      }
    }

    for (std::map<int, Client>::iterator ci = serverState.clients.begin();
         ci != serverState.clients.end(); ++ci) {
      const std::vector<std::string>& box = ci->second.outbox;
      for (size_t k = 0; k < box.size(); k++)
        send(ci->first, box[k].c_str(), box[k].size(), 0);
      ci->second.outbox.clear();
    }
  }

  close(server_fd);
  return 0;
}
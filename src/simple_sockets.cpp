#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>

#include "simple_sockets.hpp"
#include "temporarily_unavailable.hpp"

//Ignore broken pipe errors so that we can just check the return code of write
//commands to see if they succeed or fail.
#include <signal.h>

void socketCleanup(int& sock_fd) {
  if (sock_fd >= 0) {
    //Shut down sending and receiving on this socket
    shutdown(sock_fd, SHUT_RDWR);
    //Block until read returns 0 or the socket locks up
    char buff[100];
    //TODO FIXME Not actually blocking here
    while (0 != read(sock_fd, buff, sizeof(buff)) and
           EWOULDBLOCK != errno);
    //TODO FIXME Should check error codes here
    close(sock_fd);
    sock_fd = -1;
  }
}

ClientSocket::ClientSocket(int domain, int type, int protocol, uint32_t port, const std::string& ip_address, int sock_flags) :
  _port(port), _ip_address(ip_address) {
  //Set up an interrupt handler to ignore broken pipes.
  signal(SIGPIPE, SIG_IGN);
  sock_fd = socket(domain, type | sock_flags, protocol);
  if (-1 == sock_fd) {
    std::string err_str(strerror(errno));
    std::cerr<<"Error making socket: "<<err_str<<'\n';
  }
  else {
    sockaddr_in s_addr;
    {
      //Get an address to connect to using the specified interface
      struct addrinfo* addresses;
      struct addrinfo hints;
      memset(&hints, 0, sizeof(hints));
      hints.ai_family   = domain;      //Allows IPv4 or IPv6
      hints.ai_socktype = type;    //TCP connection
      hints.ai_flags    = AI_V4MAPPED | AI_ADDRCONFIG;//= AI_NUMERICHOST;
      int err = getaddrinfo(_ip_address.c_str(), std::to_string(port).c_str(), &hints, &addresses);
      bool found = false;
      if (0 == err) {
        for (addrinfo* addr = addresses; not found and addr != NULL; addr = addr->ai_next) {
          //Copy the accepted sockaddr structure.
          memcpy(&s_addr, addr->ai_addr, sizeof(struct sockaddr));
          found = true;
          break;
        }
        freeaddrinfo(addresses);
      }
      else {
        std::string err_str(gai_strerror(err));
        std::cerr<<"Error with internet address "<<_ip_address<<": "<<err_str<<'\n';
        close(sock_fd);
        sock_fd = -1;
      }
      if (not found) {
        std::cerr<<"No possible connection found for "<<_ip_address<<":"<<port<<'\n';
      }
    }
    if (sock_fd >= 0) {
      int result = connect(sock_fd, (sockaddr*)&s_addr, sizeof(s_addr));
      //If the socket is non-blocking it could return before fulling connecting
      if (-1 == result and errno == EINPROGRESS) {
        //Wait until it is ready
        pollfd ufd;
        ufd.fd = sock_fd;
        //Found out when the socket supports input or output
        ufd.events = POLLIN;
        //Wait up to 5 seconds for each poll to finish
        result = poll(&ufd, 1, 5000);
        if (result == 1 and (ufd.revents & (POLLERR | POLLHUP | POLLNVAL)) == 0) {
          ufd.events = POLLOUT;
          result = poll(&ufd, 1, 5000);
        }
        if (result == 0) {
          std::cerr<<"Error connecting: "<<"timed out"<<'\n';
          close(sock_fd);
          sock_fd = -1;
        }
        else if (result == 1) {
          if ((ufd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
            std::string err_str = "";
            if (ufd.revents & POLLERR) err_str = "socket error";
            else if (ufd.revents & POLLHUP) err_str = "remote side disconnected";
            else if (ufd.revents & POLLNVAL) err_str = "bad file descriptor";
            std::cerr<<"Error connecting: "<<err_str<<'\n';
            close(sock_fd);
            sock_fd = -1;
          }
        }
      }
      if (-1 == result) {
      std::string err_str(strerror(errno));
      std::cerr<<"Error connecting: "<<err_str<<'\n';
      close(sock_fd);
      sock_fd = -1;
      }
    }
  }
}

ClientSocket::ClientSocket(uint32_t port, const std::string& ip_address, int sock) :
  _port(port), _ip_address(ip_address), sock_fd(sock) {
  //Set up an interrupt handler to ignore broken pipes.
  signal(SIGPIPE, SIG_IGN);
}

bool ClientSocket::inputReady(int msec_timeout) {
  //Wait until data is ready
  pollfd ufd;
  ufd.fd = sock_fd;
  ufd.events = POLLIN;
  //Wait up to timeout milliseconds for the poll to finish
  int result = poll(&ufd, 1, msec_timeout);
  if (result == 1 and (ufd.revents & (POLLERR | POLLHUP | POLLNVAL)) == 0) {
    return true;
  }
  else {
    //See if the socket has an error.
    if ((ufd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
      std::string err_str = "";
      if (ufd.revents & POLLERR) err_str = "socket error";
      else if (ufd.revents & POLLHUP) err_str = "remote side disconnected";
      else if (ufd.revents & POLLNVAL) err_str = "bad file descriptor";
      //Clean up the socket (close connection and file descriptor)
      socketCleanup(sock_fd);
      throw std::runtime_error("Error connecting: "+err_str);
    }
    else {
      return false;
    }
  }
  return false;
}

ssize_t ClientSocket::receive(std::vector<unsigned char>& buff) {
  //std::array or std::val_array might be more efficient here
  ssize_t bytes_read = read(sock_fd, buff.data(), buff.size());
  return bytes_read;
}

void ClientSocket::send(const std::vector<unsigned char>& buff) {
  uint64_t written = 0;
  std::string arr(buff.begin(), buff.end());
  while (written < arr.length()) {
    //Wait for the socket to become ready (we will block on sends for 1 second)
    pollfd fds;
    fds.fd = sock_fd;
    fds.events = POLLOUT;
    poll(&fds, 1, 1000);
    if ( (fds.revents & POLLOUT) != POLLOUT) {
      throw temporarily_unavailable();
    }

    //Send the previously unsent portions of the message.
    int result = ::send(sock_fd, arr.c_str()+written, arr.length() - written, MSG_NOSIGNAL);
    if (-1 == result) {
      std::string err_str(strerror(errno));
      if (err_str == "Broken pipe") {
        err_str = "Connection closed by receiver (broken pipe)";
      }
      err_str = "Error sending data over socket: " + err_str;
      //Clean up the socket (close connection and file descriptor)
      socketCleanup(sock_fd);
      throw std::runtime_error(err_str);
    }
    else {
      written += result;
    }
  }
  return;
}

//Evalute to true if socket is open, false otherwise
ClientSocket::operator bool() const {
  return sock_fd >= 0;
}

//Close the connection in the destructor
ClientSocket::~ClientSocket() {
  socketCleanup(sock_fd);
}

ClientSocket& ClientSocket::operator=(ClientSocket&& other) {
  //Close the existing socket if we are assigning over it
  if (sock_fd >= 0) {
    //Shut down sending and receiving on this socket
    shutdown(sock_fd, SHUT_RDWR);
    //Block until read returns 0 or the socket locks up
    char buff[100];
    while (0 != read(sock_fd, buff, sizeof(buff)) and
           EWOULDBLOCK != errno);
    close(sock_fd);
    sock_fd = -1;
  }
  _port = other._port;
  _ip_address = other._ip_address;
  sock_fd = other.sock_fd;
  other.sock_fd = -1;
  return *this;
}

/**
 * Allow copy from an rvalue as with the assignment operator.
 */
ClientSocket::ClientSocket(ClientSocket&& other) {
  _port = other._port;
  _ip_address = other._ip_address;
  sock_fd = other.sock_fd;
  other.sock_fd = -1;
}

uint32_t ClientSocket::port() {
  return _port;
}

std::string ClientSocket::ip_address() {
  return _ip_address;
}

//Close the connection in the destructor
ServerSocket::~ServerSocket() {
  if (sock_fd >= 0) {
    //Shut down sending and receiving on this socket
    shutdown(sock_fd, SHUT_RDWR);
    //Block until read returns 0 or the socket locks up
    char buff[100];
    while (0 != read(sock_fd, buff, sizeof(buff)) and
           EWOULDBLOCK != errno);
    close(sock_fd);
    sock_fd = -1;
  }
}

ServerSocket::ServerSocket(int domain, int type, int sock_flags, uint32_t port) : _port(port) {
  //Set up an interrupt handler to ignore broken pipes.
  signal(SIGPIPE, SIG_IGN);
  sockaddr_in s_addr;
  {
    //Get an address to connect to using the specified interface
    struct addrinfo* addresses;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = domain;  //IPv4, IPv6, or both
    hints.ai_socktype = type;    //TCP connection
    hints.ai_flags    = AI_ADDRCONFIG | AI_V4MAPPED | AI_PASSIVE | AI_NUMERICSERV; //Wildcard IP address
    int err = getaddrinfo(NULL, std::to_string(_port).c_str(), &hints, &addresses);
    bool found = false;
    if (0 == err) {
      for (addrinfo* addr = addresses; not found and addr != NULL; addr = addr->ai_next) {
        //Copy the accepted sockaddr structure.
        memcpy(&s_addr, addr->ai_addr, sizeof(struct sockaddr));

        //Make the socket
        sock_fd = socket(addr->ai_family, addr->ai_socktype | sock_flags, addr->ai_protocol);
        //Increase the socket send buffer size
        int sock_buf_size = 128000;
        setsockopt(sock_fd, SOL_SOCKET, SO_SNDBUF, (const void*)&sock_buf_size, sizeof(sock_buf_size));
        //Make the socket ignore address in use errors.
        //int opt = 1;
        //setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(opt));
        if (0 <= sock_fd) {
          //Now try to bind with the socket.
          int succ = bind(sock_fd, addr->ai_addr, addr->ai_addrlen);
          if (0 == succ) {
            //Successfully bound
            found = true;
            break;
          }
          else {
            //Close the socket and go on to the next one.
            std::cerr<<"Error making socket: "<<strerror(errno)<<'\n';
            close(sock_fd);
            sock_fd = -1;
          }
        }
      }
      freeaddrinfo(addresses);
    }
    else {
      std::string err_str(gai_strerror(err));
      std::cerr<<"Error finding a socket for listening: "<<err_str<<'\n';
      close(sock_fd);
      sock_fd = -1;
    }
    if (not found) {
      std::cerr<<"No possible connection for receiving.\n";
    }
  }
  if (sock_fd >= 0) {
    int result = listen(sock_fd, 10);
    if (-1 == result) {
      std::string err_str(strerror(errno));
      std::cerr<<"Error listening: "<<err_str<<'\n';
      close(sock_fd);
      sock_fd = -1;
    }
  }
}

ClientSocket ServerSocket::next(int flags) {
  struct sockaddr_storage peer_addr;
  socklen_t peer_addr_size = sizeof(sockaddr_storage);
  memset(&peer_addr, 0, sizeof(peer_addr));

  //Poll the socket to see if a new connection has arrived
  //Wait until data is ready
  pollfd ufd;
  ufd.fd = sock_fd;
  ufd.events = POLLIN;
  //Wait up to 10 milliseconds for the poll to finish
  int result = poll(&ufd, 1, 10);
  //Accept a new socket is an input event is polled
  if (result == 1 and (ufd.revents & (POLLERR | POLLHUP | POLLNVAL)) == 0) {
    int in_sock = accept4(sock_fd, (struct sockaddr*)&peer_addr, &peer_addr_size, flags);

    std::string ip = "";

    if (0 <= in_sock) {
      //Get the ip address of this new connection
      char addr_buf[1000] = {0};
      int err = getnameinfo((struct sockaddr*)&peer_addr, peer_addr_size,
          addr_buf, sizeof(addr_buf) - 1, NULL, 0, NI_NUMERICHOST);
      if (err != 0) {
        std::cerr<<"Error getting client address: "<<gai_strerror(errno)<<'\n';
      }

      ip = std::string(addr_buf);
      std::cerr<<"Found ip address "<<addr_buf<<'\n';
    }
    //Don't print an error if the socket is simply non-blocking
    else if (errno != EAGAIN and errno != EWOULDBLOCK) {
      std::cerr<<"Socket failure: "<<strerror(errno)<<"\n";
    }

    //Control of the socket is handed over to the ClientSocket class.
    //The socket may be invalid if no new connection was available
    return ClientSocket(_port, ip, in_sock);
  }
  else {
    //Return an invalid socket
    return ClientSocket(_port, "", -1);
  }
}

ServerSocket::operator bool() const {
  return sock_fd >= 0;
}

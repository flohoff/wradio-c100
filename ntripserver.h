#ifndef NTRIPSERVER_H
#define NTRIPSERVER_H

#include <thread>
#include <string>
#include <iostream>
#include <sstream>
#include <cstring>
#include <memory>

extern "C" {

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
}

#include "base64.h"
#include "concurrent_queue.h"
#include "rtcmframe.h"

class ntripServer {
	private:
		std::string m_host;
		std::string m_port;
		std::string m_user;
		std::string m_pass;
		std::string m_mount;

		std::thread	m_ntripThread;

		bool	m_shutdown{false};
		bool	m_connected{false};

		int	m_socket{-1};

		ConcurrentQueue<std::shared_ptr<RtcmFrame>>	m_rtcmQueue;

	public:
		ntripServer(std::string host, std::string port, std::string user, std::string pass, std::string mount)
			: m_host(host), m_port(port), m_user(user), m_pass(pass), m_mount(mount) {

			m_ntripThread = std::thread(&ntripServer::ntripThread, this);
		}

		~ntripServer(void ) {
			m_shutdown=true;
			m_ntripThread.join();
		}

		void setsockettimeout(int sock, int seconds) {
			struct timeval tv_read;

			tv_read.tv_sec = 5;
			tv_read.tv_usec = 0;

			setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_read, sizeof tv_read);
		}

		void dropConnection(void ) {
			close(m_socket);
			m_socket=-1;
			m_connected=false;
		}

		void pushRtcm(std::shared_ptr<RtcmFrame> frame) {
			/* Dont queue if not connected */
			if (!m_connected)
				return;

			m_rtcmQueue.push(frame);

		}

		void loginCaster(void ) {
			std::stringstream ss;

			ss << "POST " << m_mount
				<< " HTTP/1.1\r\n"
				<< "Ntrip-Version: Ntrip/2.0\r\n"
				<< "User-Agent: NTRIP wradio DAB\r\n";

			if (m_user.size() > 0) {
				ss << "Authorization: Basic "
					<< base64_encode(m_user + ":" + m_pass)
					<< "\r\n";
			}
			ss << "\r\n";

			auto sss=ss.str();
			send(m_socket, (char*)sss.data(), sss.size(), MSG_NOSIGNAL);


			char	buffer[500];
			ssize_t len=recv(m_socket, buffer, sizeof(buffer), 0);

			/* FIXME Ugly hack - not really HTTP conformant - enough to get going */
			if (len > 15 && strncasecmp(buffer, "HTTP/1.1 200 OK", 15) != 0) {
				buffer[len]=0x0;
				std::cout << "NTripCaster returned " << buffer << std::endl;
				dropConnection();
				return;
			}
		}

		void connectCaster(void ) {
			struct	addrinfo	hints;
			struct	addrinfo	*result;
			int	sock;

			if (m_socket >= 0) {
				close(m_socket);
				m_socket=-1;
			}

			std::memset(&hints, 0, sizeof(struct addrinfo));
			hints.ai_family=AF_UNSPEC;
			hints.ai_socktype=SOCK_STREAM;
			int s=getaddrinfo(m_host.data(), m_port.data(), &hints, &result);

			if (s < 0) {
				std::cout << "getaddrinfo failed to resolve " << m_host
					<< " returned " << s
					<< " errno " << errno
					<< std::endl;
				return;
			}

			for(struct addrinfo *rp=result;rp!=nullptr;rp=rp->ai_next) {
				sock=socket(rp->ai_family,
						rp->ai_socktype,
						rp->ai_protocol);

				if(sock < 0)
					continue;

				if (connect(sock, rp->ai_addr, rp->ai_addrlen) != -1) {
					m_socket=sock;
					m_connected=true;
					break;
				}

				std::cout << "connect to " << m_host
					<< " failed: "
					<< strerror(errno)
					<< std::endl;

				close(sock);
			}

			freeaddrinfo(result);

			if (!m_connected)
				return;

			setsockettimeout(sock, 5);
		}

		void ntripSendNullChunk(void ) {
			std::stringstream ss;
			ss << "0\r\n\r\n";
			auto sss=ss.str();
			send(m_socket, (char*)sss.data(), sss.size(), MSG_NOSIGNAL);
		}

		void ntripSendRtcm(std::shared_ptr<RtcmFrame> frame) {
			std::stringstream ss;
			int		r;

			/*
			 * FIXME Because of the TCP Nagle algorith we send out 3 frames with this
			 * as every "send" syscall generates a packet with PSH bit set
			 */

			/* Chunked transfer - send length of chump/frame */
			ss << std::hex << frame->size() << "\r\n";
			auto sss=ss.str();

			r=send(m_socket, (char*)sss.data(), sss.size(), MSG_NOSIGNAL);
			if (r<0 && errno == EPIPE) {
				dropConnection();
				return;
			}

			r=send(m_socket, (char*)frame->data(), frame->size(), MSG_NOSIGNAL);
			if (r<0 && errno == EPIPE) {
				dropConnection();
				return;
			}

			/* Send trailing cr lf */
			sss="\r\n";
			r=send(m_socket, (char*)sss.data(), sss.size(), MSG_NOSIGNAL);
			if (r<0 && errno == EPIPE) {
				dropConnection();
				return;
			}
		}

		void ntripThread() {
			while(!m_shutdown) {
				if (!m_connected) {
					connectCaster();
					if (m_connected) {
						loginCaster();
						continue;
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(2000));
					continue;
				} else {
					std::shared_ptr<RtcmFrame>	frame;
					if(m_rtcmQueue.tryPop(frame, std::chrono::milliseconds(200))) {
						ntripSendRtcm(frame);
					}
				}
			}
		}

};

#endif

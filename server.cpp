
#include "Connection.hpp"
#include "ServerState.hpp"

#include "hex_dump.hpp"

#include <glm/glm.hpp>
#include <chrono>
#include <stdexcept>
#include <iostream>
#include <cassert>
#include <unordered_map>

int main(int argc, char **argv) {
#ifdef _WIN32
	//when compiled on windows, unhandled exceptions don't have their message printed, which can make debugging simple issues difficult.
	try {
#endif

	//------------ argument parsing ------------

	if (argc != 2) {
		std::cerr << "Usage:\n\t./server <port>" << std::endl;
		return 1;
	}

	//------------ initialization ------------

	Server server(argv[1]);
  ServerState state;


	//------------ main loop ------------
	constexpr float ServerTick = 1.0f / 60.0f;

	while (true) {
		static auto next_tick = std::chrono::steady_clock::now() + std::chrono::duration< double >(ServerTick);
    auto tick_start = std::chrono::steady_clock::now();
		//process incoming data from clients until a tick has elapsed:
		while (true) {
			auto now = std::chrono::steady_clock::now();
			double remain = std::chrono::duration< double >(next_tick - now).count();
			if (remain < 0.0) {
				next_tick += std::chrono::duration< double >(ServerTick);
				break;
			}
			server.poll([&](Connection *c, Connection::Event evt){
				if (evt == Connection::OnOpen) {
					//client connected:
          state.connect(c);

				} else if (evt == Connection::OnClose) {
					//client disconnected:
          state.disconnect(c);

				} else { assert(evt == Connection::OnRecv);
					//got data from client:
					std::cout << "got bytes:\n" << hex_dump(c->recv_buffer); std::cout.flush();

					//handle messages from client:
					while (c->recv_buffer.size() >= 6) {
						//expecting 6-byte messages 'b' (left count) (right count) (down count) (up count)
						char type = c->recv_buffer[0];
						if (type != 'b') {
							std::cout << " message of non-'b' type received from client!" << std::endl;
							//shut down client connection:
							c->close();
							return;
						}
						uint8_t left = c->recv_buffer[1];
						uint8_t right = c->recv_buffer[2];
						uint8_t down = c->recv_buffer[3];
						uint8_t up = c->recv_buffer[4];
						uint8_t space = c->recv_buffer[5];

            state.received(c, left, right, down, up, space);

						c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 6);
					}
				}
			}, remain);
		}

		//update current game state
    std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - tick_start;
    state.update(elapsed.count());

		//send updated game state to all clients
    state.broadcast();
	}


	return 0;

#ifdef _WIN32
	} catch (std::exception const &e) {
		std::cerr << "Unhandled exception:\n" << e.what() << std::endl;uuuuu
		return 1;
	} catch (...) {
		std::cerr << "Unhandled exception (unknown type)." << std::endl;
		throw;
	}
#endif
}

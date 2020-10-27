#include "PlayMode.hpp"

#include "ClientState.hpp"
#include "DrawLines.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "hex_dump.hpp"
#include "Serialization.hpp"
#include "GameConsts.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

PlayMode::PlayMode(Client &client_) : client(client_) {
	//----- allocate OpenGL resources -----
	{ //vertex buffer:
		glGenBuffers(1, &vertex_buffer);
		//for now, buffer will be un-filled.

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //vertex array mapping buffer for color_texture_program:
		//ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);

		//set vertex_buffer_for_color_texture_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_texture_program);

		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		//set up the vertex array object to describe arrays of PongMode::Vertex:
		glVertexAttribPointer(
			color_texture_program.Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 0 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]
	   

		glVertexAttribPointer(
			color_texture_program.Color_vec4, //attribute
			4, //size
			GL_UNSIGNED_BYTE, //type
			GL_TRUE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Color_vec4);

		glVertexAttribPointer(
			color_texture_program.TexCoord_vec2, //attribute
			2, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 + 4*1 //offset
		);
		glEnableVertexAttribArray(color_texture_program.TexCoord_vec2);

		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //solid white texture:
		//ask OpenGL to fill white_tex with the name of an unused texture object:
		glGenTextures(1, &white_tex);

		//bind that texture object as a GL_TEXTURE_2D-type texture:
		glBindTexture(GL_TEXTURE_2D, white_tex);

		//upload a 1x1 image of solid white to the texture:
		glm::uvec2 size = glm::uvec2(1,1);
		std::vector< glm::u8vec4 > data(size.x*size.y, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

		//set filtering and wrapping parameters:
		//(it's a bit silly to mipmap a 1x1 texture, but I'm doing it because you may want to use this code to load different sizes of texture)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
		glGenerateMipmap(GL_TEXTURE_2D);

		//Okay, texture uploaded, can unbind it:
		glBindTexture(GL_TEXTURE_2D, 0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.repeat) {
			//ignore repeats
		} else if (evt.key.keysym.sym == SDLK_a) {
      if(!left.pressed) {
        left.changed = true;
      }

			left.pressed = 1;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
      if(!right.pressed) {
        right.changed = true;
      }

			right.pressed = 1;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
      if(!up.pressed) {
        up.changed = true;
      }

			up.pressed = 1;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
      if(!down.pressed) {
        down.changed = true;
      }

			down.pressed = 1;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
      if(!space.pressed) {
        space.changed = true;
      }

			space.pressed = 1;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
      if(left.pressed) {
        left.changed = true;
      }

			left.pressed = 0;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
      if(right.pressed) {
        right.changed = true;
      }

			right.pressed = 0;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
      if(up.pressed) {
        up.changed = true;
      }

			up.pressed = 0;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
      if(down.pressed) {
        down.changed = true;
      }

			down.pressed = 0;
      return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
      if(space.pressed) {
        space.changed = true;
      }

			space.pressed = 0;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//queue data for sending to server:
	if (left.changed || right.changed || down.changed || up.changed) {
		//send a 6-byte message of type 'b':
		client.connections.back().send('b');
		client.connections.back().send(left.pressed);
		client.connections.back().send(right.pressed);
		client.connections.back().send(down.pressed);
		client.connections.back().send(up.pressed);
		client.connections.back().send(space.pressed);
	}

	//send/receive data:
	client.poll([this](Connection *c, Connection::Event event){
		if (event == Connection::OnOpen) {
			std::cout << "[" << c->socket << "] opened" << std::endl;
		} else if (event == Connection::OnClose) {
			std::cout << "[" << c->socket << "] closed (!)" << std::endl;
			throw std::runtime_error("Lost connection to server!");
		} else { assert(event == Connection::OnRecv);
			std::cout << "[" << c->socket << "] recv'd data. Current buffer:\n" << hex_dump(c->recv_buffer); std::cout.flush();
      /* Message format:
       * Header 'm' (1 byte)
       * Number of players (2 bytes)
       * Whether self is stunned (1 byte)
       * Red zone health (4 bytes)
       * Blue zone health (4 bytes)
       * Ball position (8 bytes)
       * Player positions and whether stunned (9 * n bytes)
       */
			while (c->recv_buffer.size() >= 4) {
				std::cout << "[" << c->socket << "] recv'd data. Current buffer:\n" << hex_dump(c->recv_buffer); std::cout.flush();
				char type = c->recv_buffer[0];
				if (type != 'm') {
					throw std::runtime_error("Server sent unknown message type '" + std::to_string(type) + "'");
				}

				uint32_t num_players = (
					(uint32_t(c->recv_buffer[1]) << 8) | (uint32_t(c->recv_buffer[2]))
				);

				if (c->recv_buffer.size() < 3 + 17 + num_players * 10) break; //if whole message isn't here, can't process

        auto buf_it = c->recv_buffer.begin();
        std::advance(buf_it, 3);
        // Increase vector sizes if needed
        if(num_players != state.players.size()) {
          state.players.resize(num_players);
        }

        // Load whether we've been stunned
        state.just_stunned = buf_it[0];
        std::advance(buf_it, 1);

        // Load zone healths
        state.red_zone_health = deserialize_float(buf_it);
        std::advance(buf_it, 4);
        state.blue_zone_health = deserialize_float(buf_it);
        std::advance(buf_it, 4);

        // Load ball position
        state.ball_position.x = deserialize_float(buf_it);
        std::advance(buf_it, 4);
        state.ball_position.y = deserialize_float(buf_it);
        std::advance(buf_it, 4);

        // Load player positions
        for(size_t i = 0; i < num_players; i++) {
          state.players[i].pos.x = deserialize_float(buf_it);
          std::advance(buf_it, 4);
          state.players[i].pos.y = deserialize_float(buf_it);
          std::advance(buf_it, 4);
          state.players[i].team = buf_it[0];
          std::advance(buf_it, 1);
          state.players[i].stunned = buf_it[0];
          std::advance(buf_it, 1);
        }

				//and consume this part of the buffer:
				c->recv_buffer.erase(c->recv_buffer.begin(), buf_it);
			}
		}
	}, 0.0);
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//vertices will be accumulated into this list and then uploaded+drawn at the end of this function:
	std::vector< Vertex > vertices;

	// inline helper function for rectangle drawing:
	auto draw_rectangle = [&vertices](glm::vec2 const &center,
			glm::vec2 const &size, glm::u8vec4 const &color) {
		// draw rectangle as two CCW-oriented triangles:
		vertices.emplace_back(glm::vec3(center.x-size.x, center.y-size.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+size.x, center.y-size.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+size.x, center.y+size.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

		vertices.emplace_back(glm::vec3(center.x-size.x, center.y-size.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+size.x, center.y+size.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x-size.x, center.y+size.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	};

	//compute scale factor for court given that...
	float scale = std::min(
		(2.0f) / (court.x), //... x must fit in [-aspect,aspect] ...
		(2.0f) / (court.y) //... y must fit in [-1,1].
	);

	static std::mt19937 mt; //mersenne twister pseudo-random number generator

  // Screen shake if just stunned
  if(state.just_stunned) {
    shake_dist = max_shake_dist;
  }

  glm::vec2 shake_offset;
  if(shake_dist > 0) {
    shake_angle += mt() / float(mt.max()) * angle_var * 2 - angle_var;
    shake_offset = glm::vec2(cos(shake_angle) * shake_dist,
      sin(shake_angle) * shake_dist);

    shake_dist *= 0.9;
    if(shake_dist < min_shake) shake_dist = 0;
  } else {
    shake_offset = glm::vec2(0.0f, 0.0f);
  }

  glm::u8vec4 color;
  // Draw zones
  color = state.red_zone_health < state.last_red_health ?
    red_zone_damaged_color : red_zone_color;
  draw_rectangle(red_zone_position + shake_offset, zone_dimensions * state.red_zone_health, color);
  state.last_red_health = state.red_zone_health;

  color = state.blue_zone_health < state.last_blue_health ?
    blue_zone_damaged_color : blue_zone_color;
  draw_rectangle(blue_zone_position + shake_offset, zone_dimensions * state.blue_zone_health, color);
  state.last_blue_health = state.blue_zone_health;

  // Draw players
  for(auto& player : state.players) {
							std::cout << "a "<< (bool) player.team << std::endl; std::cout.flush();
    color = player.team ?
      player.stunned ?
        player_red_stunned_color :
        player_red_color
    : player.stunned?
        player_blue_stunned_color :
        player_blue_color;
    draw_rectangle(player.pos + shake_offset, glm::vec2(player_size, player_size) / scale, color);
  }

  // Draw ball
  draw_rectangle(state.ball_position + shake_offset, glm::vec2(ball_size, ball_size) / scale, ball_color);

	//------ compute court-to-window transform ------


	glm::vec2 center = 0.5f * court;

	//build matrix that scales and translates appropriately:
	glm::mat4 court_to_clip = glm::mat4(
		glm::vec4(scale, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, scale, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(-center.x * scale, -center.y * scale, 0.0f, 1.0f)
	);
	//NOTE: glm matrices are specified in *Column-Major* order,
	// so each line above is specifying a *column* of the matrix(!)

	//---- actual drawing ----

	//clear the color buffer:
	glClearColor(background_color.r, background_color.b, background_color.g, background_color.a);
	glClear(GL_COLOR_BUFFER_BIT);

	//use alpha blending:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//don't use the depth test:
	glDisable(GL_DEPTH_TEST);

	//upload vertices to vertex_buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STREAM_DRAW); //upload vertices array
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//set color_texture_program as current program:
	glUseProgram(color_texture_program.program);

	//upload OBJECT_TO_CLIP to the proper uniform location:
	glUniformMatrix4fv(color_texture_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(court_to_clip));

	//use the mapping vertex_buffer_for_color_texture_program to fetch vertex data:
	glBindVertexArray(vertex_buffer_for_color_texture_program);

	//bind the solid white texture to location zero so things will be drawn just with their colors:
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, white_tex);

	//run the OpenGL pipeline:
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));

	//unbind the solid white texture:
	glBindTexture(GL_TEXTURE_2D, 0);


	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		auto draw_text = [&](glm::vec2 const &at, std::string const &text, float H) {
			lines.draw_text(text,
				glm::vec3(at.x, at.y, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			float ofs = 2.0f / drawable_size.y;
			lines.draw_text(text,
				glm::vec3(at.x + ofs, at.y + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		};

		draw_text(glm::vec2(-aspect + 0.1f, 0.0f), "TEST SERVER MESSAGE", 0.09f);

		draw_text(glm::vec2(-aspect + 0.1f,-0.9f), "(press WASD to change your total)", 0.09f);
	}
	GL_ERRORS();
}

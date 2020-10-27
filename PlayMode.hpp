#include "Mode.hpp"

#include "ClientState.hpp"
#include "Connection.hpp"
#include "ColorTextureProgram.hpp"

#include "GL.hpp"
#include <glm/glm.hpp>

#define _USE_MATH_DEFINES
#include <math.h>
#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode(Client &client);
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t changed = 0;
		uint8_t pressed = 0;
	} left, right, down, up, space;

	//connection to server:
	Client &client;

  // State of last received server state
  ClientState state;

  static constexpr float max_shake_dist = 0.05f;
  static constexpr float shake_reduce = 0.8f;
  static constexpr float min_shake = 0.0005f;
  static constexpr float angle_var = M_PI * 0.4f; 
  float shake_angle = 0.0f;
  float shake_dist = 0.0f;

	//----- opengl assets / helpers ------

	//draw functions will work on vectors of vertices, defined as follows:
	struct Vertex {
		Vertex(glm::vec3 const &Position_, glm::u8vec4 const &Color_, glm::vec2 const &TexCoord_) :
			Position(Position_), Color(Color_), TexCoord(TexCoord_) { }
		glm::vec3 Position;
		glm::u8vec4 Color;
		glm::vec2 TexCoord;
	};
	static_assert(sizeof(Vertex) == 4*3 + 1*4 + 4*2, "PongMode::Vertex should be packed");

	//Shader program that draws transformed, vertices tinted with vertex colors:
	ColorTextureProgram color_texture_program;

	//Buffer used to hold vertex data during drawing:
	GLuint vertex_buffer = 0;

	//Vertex Array Object that maps buffer locations to color_texture_program attribute locations:
	GLuint vertex_buffer_for_color_texture_program = 0;

	//Solid white texture:
	GLuint white_tex = 0;

	//matrix that maps from clip coordinates to court-space coordinates:
	glm::mat3x2 clip_to_court = glm::mat3x2(1.0f);
	// computed in draw() as the inverse of OBJECT_TO_CLIP
	// (stored here so that the mouse handling code can use it to position the paddle)
  
	#define HEX_TO_U8VEC4( HX ) (glm::u8vec4( (HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff ))
  static constexpr glm::u8vec4 background_color = HEX_TO_U8VEC4(0xebf5dfff);
  static constexpr glm::u8vec4 player_red_color = HEX_TO_U8VEC4(0xe86f4aff);
  static constexpr glm::u8vec4 player_red_stunned_color = HEX_TO_U8VEC4(0xef9a81ff);
  static constexpr glm::u8vec4 player_blue_color = HEX_TO_U8VEC4(0x858ae3ff);
  static constexpr glm::u8vec4 player_blue_stunned_color = HEX_TO_U8VEC4(0xbcbff0ff);
  static constexpr glm::u8vec4 ball_color = HEX_TO_U8VEC4(0x000000ff);
  static constexpr glm::u8vec4 red_zone_color = HEX_TO_U8VEC4(0xe23c4aff);
  static constexpr glm::u8vec4 red_zone_damaged_color = HEX_TO_U8VEC4(0xc31d2bff);
  static constexpr glm::u8vec4 blue_zone_color = HEX_TO_U8VEC4(0x5d70c0ff);
  static constexpr glm::u8vec4 blue_zone_damaged_color = HEX_TO_U8VEC4(0x3f53a2ff);
};

#pragma once

#include "Connection.hpp"

#include <unordered_map>
#include <glm/glm.hpp>

struct ServerState {
  ServerState();
  ~ServerState();

  void setup();

  void connect(Connection* c);
  void disconnect(Connection* c);

  void received(Connection* c,
      int in_left, int in_right, int in_down, int in_up, int space);
  void update(float elapsed);
  void broadcast();

  // Inner structures
  struct Player {
    Player(glm::vec2 in_position, uint8_t team);

    glm::vec2 position;

    // Tracks the last movement of the player, for collision handling
    glm::vec2 last_move;

    float stunned;
    uint8_t just_stunned;

    bool ball;

    // Whether this player is on red team
    uint8_t team;

    float pass_charge;
    float shoot_ghosting;

    uint32_t left;
    uint32_t right;
    uint32_t down;
    uint32_t up;
    uint32_t space;
  };

  // Game consts
  static constexpr float speed = 1.5f;
  static constexpr float ball_speed_factor = 0.8f;
  static constexpr float ball_offset_factor = 8.0f;
  static constexpr float ball_cur_offset_smooth = 0.4f;
  static constexpr float ball_last_offset_smooth = 0.6f;
  static constexpr float stun_duration = 1.5f;
  static constexpr float zone_health_depletion = 0.05f;
  static constexpr float max_cooldown = 3.0f;
  static constexpr float max_pass_charge = 1.0f;
  static constexpr float min_launch_power = 1.0f;
  static constexpr float launch_factor = 3.0f;
  static constexpr float ball_friction = 0.98f;
  static constexpr float ball_min_speed = 0.01f;
  // Prevents a player from instantly picking up their shot ball
  static constexpr float shoot_ghosting_time = 1.0f;

  // Game state
  std::unordered_map<Connection*, Player> players;

  glm::vec2 ball_position;
  glm::vec2 ball_velocity;
  Player* ball_player;
  glm::vec2 last_ball_move;
  // Last ball offset, used to smooth out the ball offset when turning
  glm::vec2 last_ball_offset;

  float red_zone_health;
  float blue_zone_health;

  float cooldown;
};

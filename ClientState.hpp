#pragma once

#include <vector>

struct ClientState {
  struct Player {
    glm::vec2 pos;
    uint8_t team;
    uint8_t stunned;
  };

  std::vector<Player> players;

  glm::vec2 ball_position;

  float red_zone_health;
  float blue_zone_health;

  float last_red_health;
  float last_blue_health;

  uint8_t just_stunned;
};

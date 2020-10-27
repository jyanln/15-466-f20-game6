#include "ServerState.hpp"

#include "Serialization.hpp"
#include "GameConsts.hpp"

#include <limits.h>
#include <iostream>

ServerState::ServerState() {
  setup();
}

ServerState::~ServerState() {
}

void ServerState::setup() {
  ball_position = court / 2.0f;
  ball_player = nullptr;
  last_ball_move = glm::vec2(0.0f, 0.0f);
  last_ball_offset = glm::vec2(0.0f, 0.0f);

  red_zone_health = 1.0f;
  blue_zone_health = 1.0f;

  for(auto &[unused_c, player] : players) {
    (void) unused_c;
    player.stunned = 0.0f;
    player.pass_charge = 0.0f;
    player.shoot_ghosting = 0.0f;
    player.just_stunned = false;
    player.ball = false;
    player.last_move = glm::vec2(0.0f, 0.0f);
    player.position = glm::vec2(player.team ? red_zone_position : blue_zone_position);
  }
}

void ServerState::connect(Connection* c) {
  int reds = 0;
  int blues = 0;
  for(auto &[unused_c, player] : players) {
    (void) unused_c;
    if(player.team) {
      reds++;
    } else {
      blues++;
    }
  }
  uint8_t team = reds < blues;
  players.emplace(c, Player(glm::vec2(team ? red_zone_position : blue_zone_position), team));
}

void ServerState::disconnect(Connection* c) {
  auto f = players.find(c);
  assert(f != players.end());
  players.erase(f);
}

void ServerState::received(Connection* c,
    int in_left, int in_right, int in_down, int in_up, int in_space) {
  Player& player = players.at(c);
  player.left = in_left;
  player.right = in_right;
  player.down = in_down;
  player.up = in_up;
  player.space = in_space;
}

void ServerState::update(float elapsed) {
  // Cool down after a round
  if(cooldown > 0) {
    cooldown -= elapsed;
    if(cooldown <= 0) {
      setup();
    } else {
      return;
    }
  }

  for(auto &[c, player] : players) {
    (void)c;

    if(player.just_stunned) player.just_stunned = false;
    if(player.stunned > 0) {
      player.stunned -= elapsed;
      // Stunned players don't move
      continue;
    }

    if(player.shoot_ghosting > 0) {
      player.shoot_ghosting -= elapsed;
    }
    
    glm::vec2 velocity(0, 0);
    if(player.left > 0) {
      velocity.x -= 1;
    }

    if(player.right > 0) {
      velocity.x += 1;
    }

    if(player.down > 0) {
      velocity.y -= 1;
    }

    if(player.up > 0) {
      velocity.y += 1;
    }

    // Update position if moved
    glm::vec2 movement;
    if (glm::length(velocity) > 0.5f) {
      movement = glm::normalize(velocity) * speed * elapsed;
      if(player.ball) {
        movement *= ball_speed_factor;
      }
    } else {
      movement = glm::vec2(0.0f, 0.0f);
    }
    player.position += movement;
    player.last_move = movement;

    // Clip to court
    if(player.position.x + player_size / 2 > court.x) {
      player.position.x = court.x - player_size / 2;
    }

    if(player.position.y + player_size / 2 > court.y) {
      player.position.y = court.y - player_size / 2;
    }

    if(player.position.x < player_size / 2) {
      player.position.x = player_size / 2;
    }

    if(player.position.y < player_size / 2) {
      player.position.y = player_size / 2;
    }

    // Update shooting information
    if(player.ball) {
      if(player.space) {
        player.pass_charge += elapsed;
      } else if (player.pass_charge > 0) {
        // Shoot the ball using its offset
        float speed = std::max(player.pass_charge, max_pass_charge) * launch_factor + min_launch_power;
        glm::vec2 direction = glm::length(ball_position - player.position) < 1e-9 ?
          (glm::length(movement) < 1e-9 ?
          // If nothing works just shoot it straight up
          glm::vec2(0.0f, 1.0f) : movement)
          : (ball_position - player.position);

        ball_velocity = glm::normalize(direction) * speed;

        // Lose ball
        player.ball = false;
        ball_player = nullptr;
        player.shoot_ghosting = shoot_ghosting_time;
        player.pass_charge = 0;
      }
    }

    // Move the ball if needed
    if(ball_player == nullptr && (glm::length(ball_velocity) > 1e-9)) {
      ball_position += ball_velocity * elapsed;
      ball_velocity *= ball_friction;
      if(glm::length(ball_velocity) < ball_min_speed) ball_velocity = glm::vec2(0.0f, 0.0f);
    }

    // If the player still has the ball, update the ball position
    if(player.ball) {
      glm::vec2 ball_offset = ball_last_offset_smooth * last_ball_offset +
        ball_cur_offset_smooth * ball_offset_factor * player.last_move;
      last_ball_offset = ball_offset;

      glm::vec2 new_ball_position = ball_offset + player.position;

      last_ball_move = new_ball_position - ball_position;
      ball_position = new_ball_position;
    }

    // Clip ball to court
    if(ball_position.x + ball_size * 3.5f > court.x) {
      ball_position.x = court.x - ball_size * 3.5f;
    }

    if(ball_position.y + ball_size * 3.5f > court.y) {
      ball_position.y = court.y - ball_size * 3.5f;
    }

    if(ball_position.x < ball_size * 3.5f) {
      ball_position.x = ball_size * 3.5f;
    }

    if(ball_position.y < ball_size * 3.5f) {
      ball_position.y = ball_size * 3.5f;
    }
  }

  // Check each player for collision with the ball
  for(auto &[c, player] : players) {
    (void)c;

    // Stunned players can't grab the ball
    // Players on the same team won't intercept each other (this also prevents
    // the ball owner from intercepting themselves)
    // Players who have just shot a ball can't grab the ball
    if(player.stunned > 0 ||
        (ball_player != nullptr && player.team == ball_player->team) ||
        player.shoot_ghosting > 0) continue;

    // Simplified collision
    if((fabs(player.position.x - ball_position.x) < (player_size + ball_size) * 3.5f) &&
        (fabs(player.position.y - ball_position.y) < (player_size + ball_size) * 3.5f)) {
      if(ball_player != nullptr) {
        ball_player->ball = false;
        ball_player->stunned = stun_duration;
        ball_player->just_stunned = true;
        ball_player = nullptr;
      }

      player.ball = true;
      ball_player = &player;
    }
  }

  // Check if ball collides with either zone
  glm::vec2 red_dimensions = zone_dimensions * red_zone_health;
  if((abs(ball_position.x - red_zone_position.x) < (ball_size + red_dimensions.x) / 2) &&
      (abs(ball_position.y - red_zone_position.y) < (ball_size + red_dimensions.y) / 2)) {
    red_zone_health -= zone_health_depletion * elapsed;
  }

  glm::vec2 blue_dimensions = zone_dimensions * blue_zone_health;
  if((abs(ball_position.x - blue_zone_position.x) < (ball_size + blue_dimensions.x) / 2) &&
      (abs(ball_position.y - blue_zone_position.y) < (ball_size + blue_dimensions.y) / 2)) {
    blue_zone_health -= zone_health_depletion * elapsed;
  }

  if(red_zone_health <= 0 || blue_zone_health <= 0) {
    cooldown = max_cooldown;
  }
}

void ServerState::broadcast() {
  /* Message format:
   * Header 'm' (1 byte)
   * Number of players (2 bytes)
   * Whether self is stunned (1 byte)
   * ---- Shared: ----
   * Red zone health (4 bytes)
   * Blue zone health (4 bytes)
   * Ball position (8 bytes)
   * Player positions, teams, and whether stunned (10 * n bytes)
   */
  size_t num_players = players.size();
  std::vector<char> buf(16 + num_players * 10);
  auto buf_it = buf.begin();

  serialize_float(red_zone_health, buf_it);
  std::advance(buf_it, 4);
  serialize_float(blue_zone_health, buf_it);
  std::advance(buf_it, 4);

  serialize_float(ball_position.x, buf_it);
  std::advance(buf_it, 4);
  serialize_float(ball_position.y, buf_it);
  std::advance(buf_it, 4);

  for(auto &[unused_c, player] : players) {
    (void)unused_c;
    serialize_float(player.position.x, buf_it);
    std::advance(buf_it, 4);
    serialize_float(player.position.y, buf_it);
    std::advance(buf_it, 4);

    buf_it[0] = player.team;
    std::advance(buf_it, 1);
    buf_it[0] = player.stunned > 0;
    std::advance(buf_it, 1);
  }
  
  for(auto &[c, player] : players) {
    c->send('m');

    // 2 bytes can hold up to 2^16 players
    // If this game had 2^16 players I would probably have enough money to
    // rewrite this code here
    c->send(uint8_t((num_players >> 8) % 256));
    c->send(uint8_t(num_players % 256));

    c->send(player.just_stunned > 0);

    for(char& i : buf) {
      c->send(i);
    }
  }
}

ServerState::Player::Player(glm::vec2 in_position, uint8_t in_team) {
  position = in_position;
  last_move = glm::vec2(0.0f, 0.0f);
  team = in_team;
  stunned = 0.0f;
  shoot_ghosting = 0.0f;
  pass_charge = 0.0f;
  ball = false;
  left = 0;
  right = 0;
  down = 0;
  up = 0;
  space = 0;
}

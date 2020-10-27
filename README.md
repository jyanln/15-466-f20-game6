# Not Soccer

Author: Jonathan Yan

Design: Fight with friends over the coveted not-soccer ball, bring it to the enemy base to damage it, and destroy the enemy base before they can destroy yours!

Networking: Not-Soccer has the server handle all of the internal game logic. The client only sends input information, which is located in Playmode.hpp. The server waits for inputs inbetween ticks, and at every tick, it updates the game state and broadcasts only the information necessary to draw the game. The server logic can be found in ServerState.cpp.

Screen Shot:

![Screen Shot](screenshot.png)

How To Play:

Move with WASD, and space to pass. Run into the ball, or other players carrying the ball to take the ball. If you happened to steal the ball from someone, you will stun them as well, to add insult to injury. While carrying the ball, you will be slightly slower. However, your ball will be carried a short distance in the direction you are facing, so use that to your advantage! Bring the ball to the enemy base to damage it and reduce its size. One team wins upon destroying the enemy base. The game will automatically restart in a short while after a round finishes.

Sources: No external sources used.

This game was built with [NEST](NEST.md).


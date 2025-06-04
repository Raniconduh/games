# TD

A simple, randomly generated, tower defense game.

Click on a turret's name in the shop to begin purchasing, then click on a tile
to place it there. Press q to abort the purchase. Click on a turret (that has
already been placed) to open the upgrade menu. Click on the info button to see
the turret's stats. Buttons (in any menu) can be clicked on if they are
highlighted. Otherwise, they cannot be clicked on. Space to pause, any to
resume. q to quit.

Start by purchasing a turret and placing it on the map such that it is in range
of a path tile. Spikes may only be placed on path tiles and will run out after a
certain number of enemies run over them. When an enemy is killed, cash is
rewarded and the score increases. Once enough cash is earned, a turret may be
upgraded from its upgrade menu. Otherwise, another turret can be purchased and
placed on the map. Certain turrets may deal splash damage to enemies so that one
attack will damage multiple enemies at once. After killing enough enemies, the
round is progressed and a new wave of enemies is spawned. As the rounds
progress, enemies will gain more health and move faster.

# Config

* `X`: Board width (integer)
* `Y`: Board height (integer)
* `SEED`: PRNG seed (integer)
* `DELAY`: The delay (in milliseconds) between game ticks (integer)
* `PATH_BENDS`: The number of bends in the path (integer)
* `ATTACK_ANIMATION_DELAY`: The delay (ms) to animate attacks (integer)
* `MAX_ENEMIES`: The maximum number of enemies in a round (integer)
* `MAX_TURRETS`: The maximum number of turrets allowed on the map (integer)
* `STARTING_CASH`: Amount of cash at the start of the game (integer)
* `STARTING_LIVES`: Amount of lives at the start of the game (integer)
* `STARTING_ROUND`: The round at which the game starts (integer)

#ifndef INC_1OOM_GAME_FLEET_H
#define INC_1OOM_GAME_FLEET_H

#include "game_types.h"
#include "types.h"

struct game_s;
struct planet_s;

extern bool game_send_fleet_from_orbit(struct game_s *g, player_id_t owner, uint8_t from, uint8_t dest, const shipcount_t ships[NUM_SHIPDESIGNS], const uint8_t shiptypes[NUM_SHIPDESIGNS], uint8_t numtypes);
extern bool game_send_fleet_reloc(struct game_s *g, player_id_t owner, uint8_t from, uint8_t dest, uint8_t si, shipcount_t shipnum);
extern bool game_send_transport(struct game_s *g, struct planet_s *p);
extern void game_remove_empty_fleets(struct game_s *g);
extern void game_remove_player_fleets(struct game_s *g, player_id_t owner);
extern bool game_fleet_any_dest_player(const struct game_s *g, player_id_t owner, player_id_t target);
extern void game_fleet_unrefuel(struct game_s *g);

#endif

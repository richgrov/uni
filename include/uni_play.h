#ifndef UNI_PLAY_H_
#define UNI_PLAY_H_

#include <stdbool.h>
#include <stdint.h>

#include "uni.h"

UniPacketOut uni_pkt_join_game(
    int entity_id,
    bool hardcore,
    unsigned char gamemode,
    signed char prev_gamemode,
    int num_worlds,
    int *dim_name_lens,
    const char **dim_names,
    void *registry_codec,
    int registry_codec_len,
    const char *dim_type,
    int dim_type_len,
    const char *dim_name,
    int dim_name_len,
    int64_t seed_hash,
    int max_players,
    int view_distance,
    int sim_distance,
    bool reduced_debug_info,
    bool show_respawn_screen,
    bool debug_world,
    bool flat_world,
    bool has_death_loc,
    const char *death_loc_dim,
    int death_loc_dim_len,
    int64_t death_loc
);

#endif // !UNI_PLAY_H_

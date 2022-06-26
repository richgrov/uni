#include "uni_play.h"

#include "uni_packet.h"
#include "uni_log.h"

typedef enum {
    UNI_POUT_JOIN_GAME = 0x23,
} UniPlayOut;

// Warning: Note similar macro UNI_CHECK_ALLOC in uni_packet_handler.c
#define UNI_CHECK_PKT(pkt, type, size)                                 \
    if ((pkt).buf == NULL) {                                           \
        UNI_LOG("PACKET '%s' ALLOC(%d) FAILED", type, size);           \
        return (pkt);                                                  \
    }

UniPacketOut uni_pkt_join_game(
    int entity_id,
    bool hardcore,
    unsigned char gamemode,
    signed char prev_gamemode,
    int num_dimensions,
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
) {
    int dim_names_total_len = 0;
    for (int i = 0; i < num_dimensions; i++) {
        dim_names_total_len += uni_str_size(dim_name_lens[i]);
    }

    int pkt_size =
        uni_varint_size(UNI_POUT_JOIN_GAME) +
        sizeof(int32_t) + // entity id
        1 + // hardcore
        1 + // gamemode
        1 + // previous gamemode
        uni_varint_size(num_dimensions) +
        dim_names_total_len +
        registry_codec_len +
        uni_str_size(dim_type_len) +
        uni_str_size(dim_name_len) +
        sizeof(int64_t) + // seed hash
        uni_varint_size(max_players) +
        uni_varint_size(view_distance) +
        uni_varint_size(sim_distance) +
        1 + // reduced debug info
        1 + // show respawn screen
        1 + // debug world
        1 + // flat world
        1; // has death location

    if (has_death_loc) {
        pkt_size += uni_str_size(death_loc_dim_len) + sizeof(int64_t);
    }

    UniPacketOut pkt = uni_alloc_packet(pkt_size);
    UNI_CHECK_PKT(pkt, "join game", pkt_size);

    char *cursor = &pkt.buf[pkt.write_idx];
    cursor = uni_write_varint(cursor, UNI_POUT_JOIN_GAME);
    cursor = uni_write_int(cursor, entity_id);
    cursor = uni_write_byte(cursor, hardcore);
    cursor = uni_write_byte(cursor, gamemode);
    cursor = uni_write_byte(cursor, prev_gamemode);
    cursor = uni_write_varint(cursor, num_dimensions);
    for (int i = 0; i < num_dimensions; i++) {
        cursor = uni_write_str(cursor, dim_names[i], dim_name_lens[i]);
    }
    cursor = uni_write_bytes(cursor, registry_codec, registry_codec_len);
    cursor = uni_write_str(cursor, dim_type, dim_type_len);
    cursor = uni_write_str(cursor, dim_name, dim_name_len);
    cursor = uni_write_long(cursor, seed_hash);
    cursor = uni_write_varint(cursor, max_players);
    cursor = uni_write_varint(cursor, view_distance);
    cursor = uni_write_varint(cursor, sim_distance);
    cursor = uni_write_byte(cursor, reduced_debug_info);
    cursor = uni_write_byte(cursor, show_respawn_screen);
    cursor = uni_write_byte(cursor, debug_world);
    cursor = uni_write_byte(cursor, flat_world);
    cursor = uni_write_byte(cursor, has_death_loc);

    if (has_death_loc) {
        cursor = uni_write_str(cursor, death_loc_dim, death_loc_dim_len);
        cursor = uni_write_long(cursor, death_loc);
    }

    return pkt;
}

#include "config.h"

#include <stdio.h>

#include "uigmap.h"
#include "comp.h"
#include "game.h"
#include "game_misc.h"
#include "game_str.h"
#include "hw.h"
#include "kbd.h"
#include "lbx.h"
#include "lbxfont.h"
#include "lbxgfx.h"
#include "lbxpal.h"
#include "lib.h"
#include "log.h"
#include "rnd.h"
#include "types.h"
#include "ui.h"
#include "uidelay.h"
#include "uidefs.h"
#include "uidraw.h"
#include "uiobj.h"
#include "uipal.h"
#include "uisound.h"
#include "uistarmap_common.h"
#include "uiswitch.h"

/* -------------------------------------------------------------------------- */

struct gmap_data_s {
    struct game_s *g;
    player_id_t api;
    int16_t mode;
    uint8_t planet_i;
    int countdown;
    uint8_t *gfx_mapview;
    uint8_t *gfx_but_col;
    uint8_t *gfx_but_env;
    uint8_t *gfx_but_min;
    uint8_t *gfx_but_ok;
};

struct gmap_basic_data_s {
    struct game_s *g;
    uint8_t planet_i;
    int countdown;
    bool show_switch;
};

static void gmap_load_data(struct gmap_data_s *d)
{
    d->gfx_mapview = lbxfile_item_get(LBXFILE_STARMAP, 0x30, 0);
    d->gfx_but_col = lbxfile_item_get(LBXFILE_STARMAP, 0x31, 0);
    d->gfx_but_env = lbxfile_item_get(LBXFILE_STARMAP, 0x32, 0);
    d->gfx_but_min = lbxfile_item_get(LBXFILE_STARMAP, 0x33, 0);
    d->gfx_but_ok = lbxfile_item_get(LBXFILE_STARMAP, 0x34, 0);
}

static void gmap_free_data(struct gmap_data_s *d)
{
    lbxfile_item_release(LBXFILE_STARMAP, d->gfx_mapview);
    lbxfile_item_release(LBXFILE_STARMAP, d->gfx_but_col);
    lbxfile_item_release(LBXFILE_STARMAP, d->gfx_but_env);
    lbxfile_item_release(LBXFILE_STARMAP, d->gfx_but_min);
    lbxfile_item_release(LBXFILE_STARMAP, d->gfx_but_ok);
}

static void gmap_draw_cb(void *vptr)
{
    struct gmap_data_s *d = vptr;
    struct game_s *g = d->g;
    player_id_t tbl_havehome[PLAYER_NUM];
    int havehomenum = 1;

    tbl_havehome[0] = d->api;
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        if ((i != d->api) && (g->evn.home[i] != PLANET_NONE)) {
            tbl_havehome[havehomenum++] = i;
        }
    }

    uiobj_set_limits(7, 7, 230, 191);
    ui_draw_erase_buf();
    lbxgfx_draw_frame(0, 0, ui_data.gfx.starmap.sky, UI_SCREEN_W);
    lbxgfx_draw_frame(0, 0, d->gfx_mapview, UI_SCREEN_W);

    for (int i = 0; i < g->nebula_num; ++i) {
        int x, y;
        x = (g->nebula_x[i] * 224) / g->galaxy_maxx + 7;
        y = (g->nebula_y[i] * 185) / g->galaxy_maxy + 7;
        lbxgfx_draw_frame_offs(x, y, ui_data.gfx.starmap.smnebula[i], 7, 7, 230, 191, UI_SCREEN_W);
    }

    for (int i = 0; i < g->enroute_num; ++i) {
        const fleet_enroute_t *r = &(g->enroute[i]);
        if (BOOLVEC_IS1(r->visible, d->api)) {
            uint8_t *gfx;
            int x, y;
            x = (r->x * 224) / g->galaxy_maxx + 7;
            y = (r->y * 185) / g->galaxy_maxy + 7;
            gfx = ui_data.gfx.starmap.tinyship[g->eto[r->owner].banner];
            lbxgfx_draw_frame_offs(x, y, gfx, 7, 7, 230, 191, UI_SCREEN_W);
        }
    }

    for (int i = 0; i < g->transport_num; ++i) {
        const transport_t *r = &(g->transport[i]);
        if (BOOLVEC_IS1(r->visible, d->api)) {
            uint8_t *gfx;
            int x, y;
            x = (r->x * 224) / g->galaxy_maxx + 7;
            y = (r->y * 185) / g->galaxy_maxy + 7;
            gfx = ui_data.gfx.starmap.tinytran[g->eto[r->owner].banner];
            lbxgfx_draw_frame_offs(x, y, gfx, 7, 7, 230, 191, UI_SCREEN_W);
        }
    }

    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        if (BOOLVEC_IS1(p->within_srange, d->api)) {
            player_id_t tbl_have_orbit_owner[PLAYER_NUM];
            int have_orbit_num;

            have_orbit_num = 0;
            for (player_id_t j = PLAYER_0; j < g->players; ++j) {
                const fleet_orbit_t *r = &(g->eto[j].orbit[i]);
                for (int k = 0; k < g->eto[j].shipdesigns_num; ++k) {
                    if (r->ships[k] != 0) {
                        tbl_have_orbit_owner[have_orbit_num++] = j;
                        break;
                    }
                }
            }

            for (int j = 0; j < have_orbit_num; ++j) {
                uint8_t *gfx;
                int x, y;
                /* FIXME all drawn to same position? */
                x = (p->x * 224) / g->galaxy_maxx + 14;
                y = (p->y * 185) / g->galaxy_maxy + 8;
                gfx = ui_data.gfx.starmap.tinyship[g->eto[tbl_have_orbit_owner[j]].banner];
                lbxgfx_draw_frame_offs(x, y, gfx, 7, 7, 230, 191, UI_SCREEN_W);
            }
        }
    }

    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        uint8_t *gfx;
        int x, y;
        x = (p->x * 224) / g->galaxy_maxx + 7;
        y = (p->y * 185) / g->galaxy_maxy + 7;
        if (BOOLVEC_IS1(p->explored, d->api) || (d->mode == 0)) {
            gfx = ui_data.gfx.starmap.smstars[p->star_type];
            if ((d->planet_i == i) && (d->countdown > 0)) {
                lbxgfx_set_new_frame(gfx, d->countdown);
            } else {
                lbxgfx_set_frame_0(gfx);
            }
            lbxgfx_draw_frame(x, y, gfx, UI_SCREEN_W);
        } else {
            gfx = ui_data.gfx.starmap.smallstr;
            lbxgfx_set_frame_0(gfx);
            lbxgfx_draw_frame(x + 1, y + 1, gfx, UI_SCREEN_W);
        }
    }

    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        uint8_t *gfx;
        player_id_t owner;
        int x, y;
        x = (p->x * 224) / g->galaxy_maxx + 7;
        y = (p->y * 185) / g->galaxy_maxy + 7;
        owner = (BOOLVEC_IS1(p->within_srange, d->api)) ? p->owner : g->seen[d->api][i].owner;
        if (owner != PLAYER_NONE) {
            gfx = ui_data.gfx.starmap.smalflag[g->eto[owner].banner];
            lbxgfx_draw_frame(x + 2, y - 3, gfx, UI_SCREEN_W);
        }
        switch (d->mode) {
            case 1:
                if (BOOLVEC_IS1(p->explored, d->api)) {
                    char buf[2] = { 0, 0 };
                    int pt;
                    lbxfont_select_set_12_1(2, (p->type < g->eto[PLAYER_0].have_colony_for) ? 5 : 0xe, 0, 0);
                    pt = (PLANET_TYPE_TERRAN - p->type);
                    SETMAX(pt, 0);
                    buf[0] = game_str_gm_tchar[pt];
                    lbxfont_print_str_normal_limit(x + 7, y, buf, 7, 7, 230, 191, UI_SCREEN_W);
                }
                break;
            case 2:
                if (BOOLVEC_IS1(p->explored, d->api) && (p->special != PLANET_SPECIAL_NORMAL)) {
                    if (p->special > PLANET_SPECIAL_ARTIFACTS) {
                        lbxfont_select(2, 1, 0, 0);
                    } else {
                        lbxfont_select_set_12_1(2, (p->special != PLANET_SPECIAL_ARTIFACTS) ? 5 : 0xe, 0, 0);
                    }
                    if (p->special == PLANET_SPECIAL_4XTECH) {
                        lbxfont_select(2, 0xe, 0, 0);
                    }
                    lbxfont_print_str_center_limit(x + 2, y + 7, game_str_tbl_gm_spec[p->special], 7, 7, 230, 191, UI_SCREEN_W);
                }
                break;
            default:
                break;
        }
    }
    switch (d->mode) {
        case 0:
            lbxfont_select(0, 6, 0, 0);
            for (int i = 0; i < havehomenum; ++i) {
                empiretechorbit_t *e;
                e = &(g->eto[tbl_havehome[i]]);
                lbxgfx_draw_frame(245, 105 + 10 * i, ui_data.gfx.starmap.smalflag[e->banner], UI_SCREEN_W);
                lbxfont_print_str_normal(260, 105 + 10 * i, game_str_tbl_race[e->race], UI_SCREEN_W);
            }
            break;
        case 1:
            for (int i = 0; i < 7; ++i) {
                char buf[2] = { 0, 0 };
                buf[0] = game_str_gm_tchar[i];
                lbxfont_select(2, ((PLANET_TYPE_TERRAN - i) < g->eto[d->api].have_colony_for) ? 5 : 0xe, 0, 0);
                lbxfont_print_str_normal(241, 105 + 7 * i, buf, UI_SCREEN_W);
                lbxfont_select(2, 6, 0, 0);
                lbxfont_print_str_normal(247, 105 + 7 * i, game_str_tbl_sm_pltype[13 - i], UI_SCREEN_W);
            }
            for (int i = 7; i < 14; ++i) {
                char buf[2] = { 0, 0 };
                const char *str;
                buf[0] = game_str_gm_tchar[i];
                lbxfont_select(2, ((PLANET_TYPE_TERRAN - i) < g->eto[d->api].have_colony_for) ? 5 : 0xe, 0, 0);
                lbxfont_print_str_normal(276, 105 + 7 * (i - 7), buf, UI_SCREEN_W);
                lbxfont_select(2, 6, 0, 0);
                if (i == 13) {
                    str = game_str_st_none2;
                } else {
                    str = game_str_tbl_sm_pltype[13 - i];
                }
                lbxfont_print_str_normal(282, 105 + 7 * (i - 7), str, UI_SCREEN_W);
            }
            lbxfont_print_str_normal(247, 160, game_str_gm_unable, UI_SCREEN_W);
            ui_draw_pixel(241, 161, 0x44);
            ui_draw_pixel(241, 162, 0x44);
            ui_draw_pixel(241, 163, 0x44);
            ui_draw_pixel(242, 161, 0x44);
            ui_draw_pixel(242, 162, 0x44);
            ui_draw_pixel(242, 163, 0x44);
            ui_draw_pixel(243, 161, 0x44);
            ui_draw_pixel(243, 162, 0x44);
            ui_draw_pixel(243, 163, 0x44);
            break;
        case 2:
            lbxfont_select_set_12_1(2, 5, 0, 0);
            lbxfont_print_str_normal(240, 103, game_str_tbl_sm_pspecial[0], UI_SCREEN_W);
            lbxfont_print_str_normal(240, 111, game_str_tbl_sm_pspecial[1], UI_SCREEN_W);
            lbxfont_select_set_12_1(2, 1, 0, 0);
            lbxfont_print_str_normal(240, 119, game_str_tbl_sm_pspecial[4], UI_SCREEN_W);
            lbxfont_print_str_normal(240, 127, game_str_tbl_sm_pspecial[5], UI_SCREEN_W);
            lbxfont_select_set_12_1(2, 0xe, 0, 0);
            lbxfont_print_str_normal(240, 135, game_str_tbl_sm_pspecial[1], UI_SCREEN_W);
            lbxfont_print_str_normal(240, 143, game_str_tbl_gm_spec[6], UI_SCREEN_W);
            lbxfont_select(2, 6, 0, 0);
            lbxfont_print_str_normal(295, 103, game_str_gm_prod, UI_SCREEN_W);
            lbxfont_print_str_normal(295, 111, game_str_gm_prod, UI_SCREEN_W);
            lbxfont_print_str_normal(295, 119, game_str_gm_prod, UI_SCREEN_W);
            lbxfont_print_str_normal(295, 127, game_str_gm_prod, UI_SCREEN_W);
            lbxfont_print_str_normal(295, 135, game_str_gm_tech, UI_SCREEN_W);
            lbxfont_print_str_normal(295, 143, game_str_gm_tech, UI_SCREEN_W);
            lbxfont_print_str_right(295, 103, game_str_gm_1_3, UI_SCREEN_W);
            lbxfont_print_str_right(295, 111, game_str_gm_1_2, UI_SCREEN_W);
            lbxfont_print_str_right(295, 119, game_str_gm_2x, UI_SCREEN_W);
            lbxfont_print_str_right(295, 127, game_str_gm_3x, UI_SCREEN_W);
            lbxfont_print_str_right(295, 135, game_str_gm_2x, UI_SCREEN_W);
            lbxfont_print_str_right(295, 143, game_str_gm_4x, UI_SCREEN_W);
            lbxfont_print_str_center(272, 152, game_str_gm_prodb1, UI_SCREEN_W);
            lbxfont_print_str_center(272, 159, game_str_gm_prodb2, UI_SCREEN_W);
            lbxfont_print_str_center(272, 166, game_str_gm_prodb3, UI_SCREEN_W);
            break;
        default:
            break;
    }

    lbxfont_set_temp_color(0x2b);
    lbxfont_select_set_12_4(4, 0xf, 0, 0);
    lbxfont_print_str_normal(242, 8, game_str_gm_gmap, UI_SCREEN_W);
    lbxfont_print_str_normal(250, 88, game_str_gm_mapkey, UI_SCREEN_W);
    lbxfont_set_temp_color(0x00);

    if (d->countdown < 0) {
        d->planet_i = rnd_0_nm1(g->galaxy_stars, &g->seed);
        d->countdown = 3;
    } else {
        --d->countdown;
    }
    {
        int x, y;
        x = (ui_data.starmap.x * 224) / g->galaxy_maxx + 7;
        y = (ui_data.starmap.y * 185) / g->galaxy_maxy + 7;
        lbxgfx_draw_frame(x, y, ui_data.gfx.starmap.bmap, UI_SCREEN_W);
    }
}

static void ui_gmap_basic_draw_galaxy(struct gmap_basic_data_s *d)
{
    struct game_s *g = d->g;
    ui_draw_filled_rect(6, 6, 221, 177, 0);
    /*uiobj_set_limits(6, 6, 221, 177);*/
    lbxgfx_draw_frame_offs(0, 0, ui_data.gfx.starmap.sky, 6, 6, 221, 177, UI_SCREEN_W);
    for (int i = 0; i < g->nebula_num; ++i) {
        int x, y;
        x = (g->nebula_x[i] * 215) / g->galaxy_maxx + 6;
        y = (g->nebula_y[i] * 171) / g->galaxy_maxy + 6;
        lbxgfx_draw_frame_offs(x, y, ui_data.gfx.starmap.smnebula[i], 6, 6, 221, 177, UI_SCREEN_W);
    }
    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        uint8_t *gfx;
        int x, y;
        x = (p->x * 215) / g->galaxy_maxx + 6;
        y = (p->y * 171) / g->galaxy_maxy + 6;
        gfx = ui_data.gfx.starmap.smstars[p->star_type];
        if ((d->planet_i == i) && (d->countdown > 0)) {
            lbxgfx_set_new_frame(gfx, d->countdown);
        } else {
            lbxgfx_set_frame_0(gfx);
        }
        lbxgfx_draw_frame(x, y, gfx, UI_SCREEN_W);
    }
}

/* -------------------------------------------------------------------------- */

bool ui_gmap(struct game_s *g, player_id_t active_player)
{
    struct gmap_data_s d;
    bool flag_done = false, flag_do_focus = false;
    int16_t /*oi_col, oi_env, oi_min,*/ oi_ok, oi_tbl_planet[PLANETS_MAX];

    gmap_load_data(&d);
    d.g = g;
    d.api = active_player;
    d.mode = 0;
    d.countdown = -1;
    d.planet_i = 0;

    uiobj_table_clear();

    oi_ok = uiobj_add_t0(246, 181, "", d.gfx_but_ok, MOO_KEY_SPACE, -1);
    /*oi_col =*/ uiobj_add_t3(246, 27, "", d.gfx_but_col, &(d.mode), 0, MOO_KEY_c, -1);
    /*oi_env =*/ uiobj_add_t3(246, 47, "", d.gfx_but_env, &(d.mode), 1, MOO_KEY_e, -1);
    /*oi_min =*/ uiobj_add_t3(246, 67, "", d.gfx_but_min, &(d.mode), 2, MOO_KEY_m, -1);
    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        int x, y;
        x = (p->x * 224) / g->galaxy_maxx + 7;
        y = (p->y * 185) / g->galaxy_maxy + 7;
        /* FIXME limits not set! ... but no need to limit anyway */
        oi_tbl_planet[i] = uiobj_add_mousearea/*_limited*/(x, y, x + 4, y + 4, MOO_KEY_UNKNOWN, -1);
    }

    uiobj_set_help_id(9);
    uiobj_set_callback_and_delay(gmap_draw_cb, &d, 4);
    uiobj_set_downcount(1);

    while (!flag_done) {
        int16_t oi;
        oi = uiobj_handle_input_cond();
        ui_delay_prepare();
        if ((oi == oi_ok) || (oi == UIOBJI_ESC)) {
            flag_done = true;
        }
        if (oi != 0) {
            ui_sound_play_sfx_24();
        }
        for (int i = 0; i < g->galaxy_stars; ++i) {
            if (oi == oi_tbl_planet[i]) {
                g->planet_focus_i[active_player] = i;
                flag_do_focus = true;
                flag_done = true;
                break;
            }
        }
        if (!flag_done) {
            gmap_draw_cb(&d);
            ui_draw_finish();
            ui_delay_ticks_or_click(4);
        }
    }

    uiobj_unset_callback();
    uiobj_set_help_id(-1);
    gmap_free_data(&d);
    return flag_do_focus;
}

void *ui_gmap_basic_init(struct game_s *g, bool show_player_switch)
{
    static struct gmap_basic_data_s ctx; /* HACK */
    ctx.g = g;
    ctx.countdown = -1;
    ctx.planet_i = 0;
    ctx.show_switch = show_player_switch;
    if (!show_player_switch) {
        ui_draw_copy_buf();
        ui_starmap_draw_button_text(0/*unused*/, false);
        hw_video_copy_back_to_page2();
    }
    return &ctx;
}

void ui_gmap_basic_shutdown(void *ctx)
{
}

void ui_gmap_basic_start_player(void *ctx, int pi)
{
    struct gmap_basic_data_s *d = ctx;
    if (d->show_switch) {
        player_id_t pil;
        pil = pi;
        ui_switch(d->g, &pil, 1, false);
        ui_draw_copy_buf();
        hw_video_copy_back_to_page2();
    }
}

void ui_gmap_basic_start_frame(void *ctx, int pi)
{
    /*struct gmap_basic_data_s *d = ctx;*/
    ui_delay_prepare();
    hw_video_copy_back_from_page2();
}

void ui_gmap_basic_draw_frame(void *ctx, int pi/*player_i*/)
{
    struct gmap_basic_data_s *d = ctx;
    struct game_s *g = d->g;
    ui_gmap_basic_draw_galaxy(d);
    if (pi >= 0) {
        for (int i = 0; i < g->enroute_num; ++i) {
            const fleet_enroute_t *r = &(g->enroute[i]);
            if (BOOLVEC_IS1(r->visible, pi)) {
                uint8_t *gfx;
                int x, y;
                x = (r->x * 215) / g->galaxy_maxx + 6;
                y = (r->y * 171) / g->galaxy_maxy + 6;
                gfx = ui_data.gfx.starmap.tinyship[g->eto[r->owner].banner];
                lbxgfx_draw_frame_offs(x, y, gfx, 6, 6, 221, 177, UI_SCREEN_W);
            }
        }
        for (int i = 0; i < g->transport_num; ++i) {
            const transport_t *r = &(g->transport[i]);
            if (BOOLVEC_IS1(r->visible, pi)) {
                uint8_t *gfx;
                int x, y;
                x = (r->x * 215) / g->galaxy_maxx + 6;
                y = (r->y * 171) / g->galaxy_maxy + 6;
                gfx = ui_data.gfx.starmap.tinytran[g->eto[r->owner].banner];
                lbxgfx_draw_frame_offs(x, y, gfx, 6, 6, 221, 177, UI_SCREEN_W);
            }
        }
        for (int i = 0; i < g->galaxy_stars; ++i) {
            const planet_t *p = &(g->planet[i]);
            if (BOOLVEC_IS1(p->within_srange, pi)) {
                player_id_t tbl_have_orbit_owner[PLAYER_NUM];
                int have_orbit_num;
                have_orbit_num = 0;
                for (player_id_t j = PLAYER_0; j < g->players; ++j) {
                    const fleet_orbit_t *r = &(g->eto[j].orbit[i]);
                    for (int k = 0; k < g->eto[j].shipdesigns_num; ++k) {
                        if (r->ships[k] != 0) {
                            tbl_have_orbit_owner[have_orbit_num++] = j;
                            break;
                        }
                    }
                }
                for (int j = 0; j < have_orbit_num; ++j) {
                    uint8_t *gfx;
                    int x, y;
                    /* FIXME all drawn to same position? */
                    x = (p->x * 215) / g->galaxy_maxx + 13;
                    y = (p->y * 171) / g->galaxy_maxy + 7;
                    gfx = ui_data.gfx.starmap.tinyship[g->eto[tbl_have_orbit_owner[j]].banner];
                    lbxgfx_draw_frame_offs(x, y, gfx, 6, 6, 221, 177, UI_SCREEN_W);
                }
            }
        }
        for (int i = 0; i < 2; ++i) {
            const monster_t *r;
            r = (i == 0) ? &(g->evn.crystal) : &(g->evn.amoeba);
            if (r->exists && (r->killer == PLAYER_NONE)) {
                uint8_t *gfx;
                int x, y;
                x = (r->x * 215) / g->galaxy_maxx + 6;
                y = (r->y * 171) / g->galaxy_maxy + 6;
                gfx = ui_data.gfx.planets.tmonster;
                lbxgfx_draw_frame_offs(x, y, gfx, 6, 6, 221, 177, UI_SCREEN_W);
            }
        }
    }
    if (d->countdown < 0) {
        d->planet_i = rnd_0_nm1(g->galaxy_stars, &g->seed);
        d->countdown = 3;
    } else {
        --d->countdown;
    }
}

void ui_gmap_basic_draw_only(void *ctx, int pi/*planet_i*/)
{
    struct gmap_basic_data_s *d = ctx;
    struct game_s *g = d->g;
    ui_gmap_basic_draw_galaxy(d);
    {
        const planet_t *p = &(g->planet[pi]);
        player_id_t tbl_have_orbit_owner[PLAYER_NUM];
        int have_orbit_num;
        have_orbit_num = 0;
        for (player_id_t j = PLAYER_0; j < g->players; ++j) {
            const fleet_orbit_t *r = &(g->eto[j].orbit[pi]);
            for (int k = 0; k < g->eto[j].shipdesigns_num; ++k) {
                if (r->ships[k] != 0) {
                    tbl_have_orbit_owner[have_orbit_num++] = j;
                    break;
                }
            }
        }
        for (int j = 0; j < have_orbit_num; ++j) {
            uint8_t *gfx;
            int x, y;
            /* FIXME all drawn to same position? */
            x = (p->x * 215) / g->galaxy_maxx + 13;
            y = (p->y * 171) / g->galaxy_maxy + 7;
            gfx = ui_data.gfx.starmap.tinyship[g->eto[tbl_have_orbit_owner[j]].banner];
            lbxgfx_draw_frame_offs(x, y, gfx, 6, 6, 221, 177, UI_SCREEN_W);
        }
        for (int i = 0; i < 2; ++i) {
            const monster_t *r;
            r = (i == 0) ? &(g->evn.crystal) : &(g->evn.amoeba);
            if (r->exists && (r->killer == PLAYER_NONE) && (r->x == p->x) && (r->y == p->y)) {
                uint8_t *gfx;
                int x, y;
                x = (r->x * 215) / g->galaxy_maxx + 6;
                y = (r->y * 171) / g->galaxy_maxy + 6;
                gfx = ui_data.gfx.planets.tmonster;
                lbxgfx_draw_frame_offs(x, y, gfx, 6, 6, 221, 177, UI_SCREEN_W);
            }
        }
    }
    if (d->countdown < 0) {
        d->planet_i = rnd_0_nm1(g->galaxy_stars, &g->seed);
        d->countdown = 3;
    } else {
        --d->countdown;
    }
}

void ui_gmap_basic_finish_frame(void *ctx, int pi)
{
    /*struct gmap_basic_data_s *d = ctx;*/
    ui_draw_finish();
    ui_delay_ticks_or_click(2);
}

void ui_gmap_draw_planet_border(const struct game_s *g, uint8_t planet_i)
{
    const planet_t *p = &(g->planet[planet_i]);
    int x, y;
    x = (p->x * 215) / g->galaxy_maxx + 5;
    y = (p->y * 171) / g->galaxy_maxy + 5;
    lbxgfx_draw_frame_offs(x, y, ui_data.gfx.starmap.slanbord, 6, 6, 221, 177, UI_SCREEN_W);
}

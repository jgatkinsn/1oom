#include "config.h"

#include <stdio.h>

#include "uistarmap.h"
#include "comp.h"
#include "game.h"
#include "game_misc.h"
#include "game_save.h"
#include "game_str.h"
#include "kbd.h"
#include "lbxgfx.h"
#include "lbxfont.h"
#include "log.h"
#include "types.h"
#include "uidraw.h"
#include "uidefs.h"
#include "uidelay.h"
#include "uiobj.h"
#include "uisound.h"
#include "uistarmap_common.h"

/* -------------------------------------------------------------------------- */

static void ui_starmap_reloc_draw_cb(void *vptr)
{
    struct starmap_data_s *d = vptr;
    struct game_s *g = d->g;
    planet_t *pf = &g->planet[d->rl.from];
    planet_t *pt = &g->planet[g->planet_focus_i[d->api]];
    char buf[0x40];
    int x0, y0;
    ui_starmap_draw_starmap(d);
    ui_starmap_draw_button_text(d, true);
    x0 = (pf->x - ui_data.starmap.x) * 2 + 8;
    y0 = (pf->y - ui_data.starmap.y) * 2 + 8;
    if (pf->reloc != d->rl.from) {
        int x1, y1;
        x1 = (pt->x - ui_data.starmap.x) * 2 + 14;
        y1 = (pt->y - ui_data.starmap.y) * 2 + 14;
        ui_draw_line_limit_ctbl(x0 + 6, y0 + 6, x1, y1, colortbl_line_hmm1, 5, ui_data.starmap.line_anim_phase);
    }
    lbxgfx_draw_frame_offs(x0, y0, ui_data.gfx.starmap.planbord, 6, 6, 221, 177, UI_SCREEN_W);
    lbxgfx_draw_frame(222, 80, ui_data.gfx.starmap.relocate, UI_SCREEN_W);
    lbxfont_select_set_12_1(5, 5, 0, 0);
    lbxfont_print_str_center(269, 90, game_str_sm_sreloc, UI_SCREEN_W);
    lbxfont_select(0, 6, 0, 0);
    lbxfont_print_str_split(229, 105, 80, game_str_sm_sreloc2, 2, UI_SCREEN_W, UI_SCREEN_H);
    if (g->planet_focus_i[d->api] != d->rl.from) {
        if (pf->have_stargate && pt->have_stargate) {
            strcpy(buf, game_str_sm_stargate);
        } else {
            int eta;
            eta = game_calc_eta(g, g->srd[d->api].design[pf->buildship].engine, pf->x, pf->y, pt->x, pt->y);
            sprintf(buf, "%s %i %s", game_str_sm_delay, eta, (eta == 1) ? game_str_sm_turn : game_str_sm_turns);
        }
        lbxfont_select(0, 0, 0, 0);
        lbxfont_print_str_center(268, 149, buf, UI_SCREEN_W);
    }
    lbxgfx_set_new_frame(ui_data.gfx.starmap.reloc_bu_accept, 1);
    lbxgfx_draw_frame(271, 163, ui_data.gfx.starmap.reloc_bu_accept, UI_SCREEN_W);
}

/* -------------------------------------------------------------------------- */

void ui_starmap_reloc(struct game_s *g, player_id_t active_player)
{
    bool flag_done = false;
    int16_t oi_scroll, oi_cancel, oi_accept,
            oi_f2, oi_f3, oi_f4, oi_f5, oi_f6, oi_f7, oi_f8, oi_f9, oi_f10
            ;
    uint16_t scrollx = 0, scrolly = 0;
    struct starmap_data_s d;
    uint8_t oldreloc;
    d.g = g;
    d.api = active_player;
    d.anim_delay = 0;
    {
        uint8_t pi = g->planet_focus_i[active_player];
        d.rl.from = pi;
        oldreloc = g->planet[pi].reloc;
        g->planet_focus_i[active_player] = oldreloc;
        if (g->planet[oldreloc].owner != active_player) {
            g->planet_focus_i[active_player] = pi;
        }
    }

    uiobj_table_clear();

#define UIOBJ_CLEAR_LOCAL() \
    do { \
        STARMAP_UIOBJ_CLEAR_COMMON(); \
        STARMAP_UIOBJ_CLEAR_FX(); \
        oi_accept = UIOBJI_INVALID; \
        oi_cancel = UIOBJI_INVALID; \
    } while (0)

    UIOBJ_CLEAR_LOCAL();

    uiobj_set_help_id(2);
    uiobj_set_callback_and_delay(ui_starmap_reloc_draw_cb, &d, STARMAP_DELAY);

    while (!flag_done) {
        int16_t oi1, oi2;
        oi1 = uiobj_handle_input_cond();
        oi2 = uiobj_at_cursor();
        ui_delay_prepare();
        g->planet[d.rl.from].reloc = g->planet_focus_i[active_player];
        if (oi1 == d.oi_gameopts) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_GAMEOPTS;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == d.oi_design) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_DESIGN;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == d.oi_fleet) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_FLEET;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == d.oi_map) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_MAP;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == d.oi_races) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_RACES;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == d.oi_planets) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_PLANETS;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == d.oi_tech) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_TECH;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == d.oi_next_turn) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_NEXT_TURN;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == oi_f2) {
            int i;
            i = g->planet_focus_i[active_player];
            do {
                if (--i < 0) { i = g->galaxy_stars - 1; }
            } while (g->planet[i].owner != active_player);
            g->planet_focus_i[active_player] = i;
            ui_starmap_set_pos_focus(g, active_player);
            ui_sound_play_sfx_24();
        } else if (oi1 == oi_f3) {
            int i;
            i = g->planet_focus_i[active_player];
            do {
                i = (i + 1) % g->galaxy_stars;
            } while (g->planet[i].owner != active_player);
            g->planet_focus_i[active_player] = i;
            ui_starmap_set_pos_focus(g, active_player);
            ui_sound_play_sfx_24();
        } else if ((oi1 == oi_f8) && g->eto[active_player].have_ia_scanner) {
            bool found;
            int i, pi;
            i = pi = g->planet_focus_i[active_player];
            found = false;
            ui_sound_play_sfx_24();
            do {
                i = (i + 1) % g->galaxy_stars;
                if (g->planet[i].owner == active_player) {
                    for (int j = 0; !found && (j < g->enroute_num); ++j) {
                        fleet_enroute_t *r = &(g->enroute[i]);
                        if (BOOLVEC_IS1(r->visible, active_player) && (r->owner != active_player) && (r->dest == pi)) {
                            found = true;
                        }
                    }
                    for (int j = 0; !found && (j < g->transport_num); ++j) {
                        transport_t *r = &(g->transport[i]);
                        if (BOOLVEC_IS1(r->visible, active_player) && (r->owner != active_player) && (r->dest == pi)) {
                            found = true;
                        }
                    }
                }
            } while (!found && (i != pi));
            if (found) {
                g->planet_focus_i[active_player] = i;
                ui_starmap_set_pos_focus(g, active_player);
            }
        } else if ((oi1 == oi_f9) && g->eto[active_player].have_ia_scanner) {
            bool found;
            int i, pi;
            i = pi = g->planet_focus_i[active_player];
            found = false;
            ui_sound_play_sfx_24();
            do {
                if (--i < 0) { i = g->galaxy_stars - 1; }
                if (g->planet[i].owner == active_player) {
                    for (int j = 0; !found && (j < g->enroute_num); ++j) {
                        fleet_enroute_t *r = &(g->enroute[i]);
                        if (BOOLVEC_IS1(r->visible, active_player) && (r->owner != active_player) && (r->dest == pi)) {
                            found = true;
                        }
                    }
                    for (int j = 0; !found && (j < g->transport_num); ++j) {
                        transport_t *r = &(g->transport[i]);
                        if (BOOLVEC_IS1(r->visible, active_player) && (r->owner != active_player) && (r->dest == pi)) {
                            found = true;
                        }
                    }
                }
            } while (!found && (i != pi));
            if (found) {
                g->planet_focus_i[active_player] = i;
                ui_starmap_set_pos_focus(g, active_player);
            }
        } else if (oi1 == oi_f10) {
            game_save_do_save_i(GAME_SAVE_I_CONTINUE, "Continue", g);
        } else if (oi1 == oi_f4) {
            bool found;
            int i, pi;
            i = pi = g->planet_focus_i[active_player];
            found = false;
            do {
                i = (i + 1) % g->galaxy_stars;
                for (int j = 0; j < g->eto[active_player].shipdesigns_num; ++j) {
                    if (g->eto[active_player].orbit[i].ships[j]) {
                        found = true;
                        break;
                    }
                }
            } while ((!found) && (i != pi));
            if (found) {
                g->planet_focus_i[active_player] = i;
                ui_starmap_set_pos_focus(g, active_player);
                ui_sound_play_sfx_24();
            }
        } else if (oi1 == oi_f5) {
            bool found;
            int i, pi;
            i = pi = g->planet_focus_i[active_player];
            found = false;
            do {
                if (--i < 0) { i = g->galaxy_stars - 1; }
                for (int j = 0; j < g->eto[active_player].shipdesigns_num; ++j) {
                    if (g->eto[active_player].orbit[i].ships[j]) {
                        found = true;
                        break;
                    }
                }
            } while ((!found) && (i != pi));
            if (found) {
                g->planet_focus_i[active_player] = i;
                ui_starmap_set_pos_focus(g, active_player);
                ui_sound_play_sfx_24();
            }
        } else if (oi1 == oi_f6) {
            int i;
            i = ui_starmap_newship_next(g, active_player, g->planet_focus_i[active_player]);
            g->planet_focus_i[active_player] = i;
            ui_starmap_set_pos_focus(g, active_player);
            ui_sound_play_sfx_24();
        } else if (oi1 == oi_f7) {
            int i;
            i = ui_starmap_newship_prev(g, active_player, g->planet_focus_i[active_player]);
            g->planet_focus_i[active_player] = i;
            ui_starmap_set_pos_focus(g, active_player);
            ui_sound_play_sfx_24();
        }
        if ((oi1 == oi_cancel) || (oi1 == UIOBJI_ESC)) {
            ui_sound_play_sfx_06();
            flag_done = true;
            g->planet[d.rl.from].reloc = oldreloc;
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
        } else if (oi1 == oi_accept) {
            ui_sound_play_sfx_24();
            flag_done = true;
            g->planet[d.rl.from].reloc = g->planet_focus_i[active_player];
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
        } else if (oi1 == oi_scroll) {
            int x, y;
            x = ui_data.starmap.x + scrollx - 54;
            y = ui_data.starmap.y + scrolly - 43;
            SETRANGE(x, 0, g->galaxy_maxx - 108);
            SETRANGE(y, 0, g->galaxy_maxy - 86);
            ui_data.starmap.x2 = x;
            ui_data.starmap.y2 = y;
        }
        ui_starmap_handle_oi_ctrl(&d, oi1);
        for (int i = 0; i < g->galaxy_stars; ++i) {
            if (oi1 == d.oi_tbl_stars[i]) {
                g->planet_focus_i[active_player] = i;
                ui_starmap_set_pos_focus(g, active_player);
                ui_sound_play_sfx_24();
                break;
            }
        }
        if (!flag_done) {
            d.bottom_highlight = -1;
            if (oi2 == d.oi_gameopts) {
                d.bottom_highlight = 0;
            } else if (oi2 == d.oi_design) {
                d.bottom_highlight = 1;
            } else if (oi2 == d.oi_fleet) {
                d.bottom_highlight = 2;
            } else if (oi2 == d.oi_map) {
                d.bottom_highlight = 3;
            } else if (oi2 == d.oi_races) {
                d.bottom_highlight = 4;
            } else if (oi2 == d.oi_planets) {
                d.bottom_highlight = 5;
            } else if (oi2 == d.oi_tech) {
                d.bottom_highlight = 6;
            } else if (oi2 == d.oi_next_turn) {
                d.bottom_highlight = 7;
            }
            ui_starmap_reloc_draw_cb(&d);
            uiobj_table_clear();
            UIOBJ_CLEAR_LOCAL();
            oi_f2 = uiobj_add_inputkey(MOO_KEY_F2);
            oi_f3 = uiobj_add_inputkey(MOO_KEY_F3);
            oi_f4 = uiobj_add_inputkey(MOO_KEY_F4);
            oi_f5 = uiobj_add_inputkey(MOO_KEY_F5);
            oi_f6 = uiobj_add_inputkey(MOO_KEY_F6);
            oi_f7 = uiobj_add_inputkey(MOO_KEY_F7);
            oi_f8 = uiobj_add_inputkey(MOO_KEY_F8);
            oi_f9 = uiobj_add_inputkey(MOO_KEY_F9);
            oi_f10 = uiobj_add_inputkey(MOO_KEY_F10);
            ui_starmap_fill_oi_tbl_stars_own(&d, active_player);
            oi_cancel = uiobj_add_t0(227, 163, "", ui_data.gfx.starmap.reloc_bu_cancel, MOO_KEY_ESCAPE, -1);
            if (g->planet[d.rl.from].buildship != BUILDSHIP_STARGATE) {
                oi_accept = uiobj_add_t0(271, 163, "", ui_data.gfx.starmap.reloc_bu_accept, MOO_KEY_SPACE, -1);
            }
            oi_scroll = uiobj_add_tb(6, 6, 2, 2, 108, 86, &scrollx, &scrolly, -1);
            ui_starmap_fill_oi_ctrl(&d);
            ui_starmap_add_oi_bottom_buttons(&d);
            ui_draw_finish();
            ui_delay_ticks_or_click(STARMAP_DELAY);
        }
    }
    uiobj_unset_callback();
    uiobj_set_help_id(-1);
    g->planet_focus_i[active_player] = d.rl.from;
}

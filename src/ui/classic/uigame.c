#include "config.h"

#include "ui.h"
#include "game.h"
#include "game_audience.h"
#include "game_design.h"
#include "game_aux.h"
#include "game_misc.h"
#include "hw.h"
#include "lbx.h"
#include "lbxfont.h"
#include "lbxgfx.h"
#include "lbxpal.h"
#include "lib.h"
#include "log.h"
#include "types.h"
#include "uibasescrap.h"
#include "uicaught.h"
#include "uicursor.h"
#include "uidefs.h"
#include "uidesign.h"
#include "uidraw.h"
#include "uiempirereport.h"
#include "uiempirestatus.h"
#include "uifleet.h"
#include "uigmap.h"
#include "uigameopts.h"
#include "uiobj.h"
#include "uipal.h"
#include "uiplanets.h"
#include "uiraces.h"
#include "uispecs.h"
#include "uistarmap.h"
#include "uistarview.h"
#include "uiswitch.h"
#include "uitech.h"

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */

ui_turn_action_t ui_game_turn(struct game_s *g, int *load_game_i_ptr, int pi)
{
    int scrapi = -1;
    int opponi = -1;
    if (g->gaux->local_players > 1) {
        player_id_t pil;
        pil = pi;
        while (ui_switch(g, &pil, 1, true)) {
            switch (ui_gameopts(g, load_game_i_ptr)) {
                case GAMEOPTS_DONE:
                    break;
                 case GAMEOPTS_LOAD:
                    return UI_TURN_ACT_LOAD_GAME;
                 case GAMEOPTS_QUIT:
                    return UI_TURN_ACT_QUIT_GAME;
            }
        }
        ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
        ui_starmap_set_pos_focus(g, pi);
    }
    while (1) {
        ui_cursor_setup_area(1, &ui_cursor_area_tbl[0]);
        if (g->evn.build_finished_num[pi] > 0) {
            uint8_t pli;
            for (pli = 0; pli < g->galaxy_stars; ++pli) {
                if (g->planet[pli].finished[0] & (~(1 << FINISHED_SHIP))) {
                    break;
                }
            }
            g->planet_focus_i[pi] = pli;
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
            ui_starmap_set_pos_focus(g, pi);
        }
        game_update_have_reserve_fuel(g); /* TODO move to game_* ? */
        ui_data.flag_main_hmm1 = false;
        switch (ui_data.ui_main_loop_action) {
            case UI_MAIN_LOOP_STARMAP:
                ui_cursor_setup_area(2, &ui_cursor_area_tbl[3]);
                ui_starmap_do(g, pi);
                break;
            case UI_MAIN_LOOP_RELOC:
                ui_cursor_setup_area(2, &ui_cursor_area_tbl[1]);
                ui_starmap_reloc(g, pi);
                break;
            case UI_MAIN_LOOP_TRANS:
                ui_cursor_setup_area(2, &ui_cursor_area_tbl[1]);
                ui_starmap_trans(g, pi);
                break;
            case UI_MAIN_LOOP_ORBIT_OWN_SEL:
                ui_cursor_setup_area(2, &ui_cursor_area_tbl[3]);
                ui_starmap_orbit_own(g, pi);
                break;
            case UI_MAIN_LOOP_ORBIT_EN_SEL:
                ui_cursor_setup_area(2, &ui_cursor_area_tbl[3]);
                ui_starmap_orbit_en(g, pi);
                break;
            case UI_MAIN_LOOP_TRANSPORT_SEL:
                ui_cursor_setup_area(2, &ui_cursor_area_tbl[3]);
                ui_starmap_transport(g, pi);
                break;
            case UI_MAIN_LOOP_ENROUTE_SEL:
                ui_cursor_setup_area(2, &ui_cursor_area_tbl[3]);
                ui_starmap_enroute(g, pi);
                break;
            case UI_MAIN_LOOP_GAMEOPTS:
                switch (ui_gameopts(g, load_game_i_ptr)) {
                    case GAMEOPTS_DONE:
                        ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
                        break;
                    case GAMEOPTS_LOAD:
                        return UI_TURN_ACT_LOAD_GAME;
                    case GAMEOPTS_QUIT:
                        return UI_TURN_ACT_QUIT_GAME;
                }
                break;
            case UI_MAIN_LOOP_DESIGN:
                {
                    struct game_design_s gd;
                    bool ok;
                    int sd_num;
                    sd_num = g->eto[pi].shipdesigns_num;
                    game_design_prepare(g, &gd, pi, &g->current_design[pi]);
                    ok = ui_design(g, &gd, pi);
                    if (ok && (sd_num == NUM_SHIPDESIGNS)) {
                        ui_specs_before(g, pi);
                        ui_data.ui_main_loop_action = UI_MAIN_LOOP_SPECS;
                        ui_data.ui_main_loop_action_next = UI_MAIN_LOOP_SPECS;
                        ui_data.ui_main_loop_action_prev = UI_MAIN_LOOP_DESIGN;
                        ui_data.flag_main_hmm1 = true;
                        scrapi = ui_specs(g, pi);
                        sd_num = g->eto[pi].shipdesigns_num;
                        ok = (sd_num < NUM_SHIPDESIGNS);
                    }
                    if (ok) {
                        g->srd[pi].design[sd_num] = gd.sd;
                        g->eto[pi].shipdesigns_num = ++sd_num;
                    }
                    g->current_design[pi] = gd.sd;
                }
                ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
                break;
            case UI_MAIN_LOOP_SPECS:
                scrapi = ui_specs(g, pi);
                break;
            case UI_MAIN_LOOP_MUSTSCRAP:
                if (scrapi >= 0) {
                    ui_specs_mustscrap(g, pi, scrapi);
                } else {
                    LOG_DEBUG((3, "%s: invalid scrapi %i on MUSTSCRAP\n", __func__, scrapi));
                    ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
                }
                break;
            case UI_MAIN_LOOP_PLANETS:
                ui_planets(g, pi);
                ui_starmap_set_pos_focus(g, pi);
                ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
                break;
            case UI_MAIN_LOOP_FLEET:
                scrapi = ui_fleet(g, pi);
                if (ui_data.ui_main_loop_action == UI_MAIN_LOOP_ORBIT_OWN_SEL) {
                    ui_starmap_set_pos_focus(g, pi);
                } else if (ui_data.ui_main_loop_action == UI_MAIN_LOOP_ENROUTE_SEL) {
                    fleet_enroute_t *r;
                    r = &(g->enroute[ui_data.starmap.fleet_selected]);
                    ui_starmap_set_pos(g, r->x, r->y);
                }
                break;
            case UI_MAIN_LOOP_MAP:
                if (ui_gmap(g, pi)) {
                    ui_starmap_set_pos_focus(g, pi);
                }
                ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
                break;
            case UI_MAIN_LOOP_RACES:
                opponi = ui_races(g, pi);
                break;
            case UI_MAIN_LOOP_EMPIRESTATUS:
                ui_empirestatus(g, pi);
                ui_data.ui_main_loop_action = UI_MAIN_LOOP_RACES;
                break;
            case UI_MAIN_LOOP_EMPIREREPORT:
                if ((opponi > 0) && (opponi < g->players)) {
                    ui_empirereport(g, pi, opponi);
                } else {
                    LOG_DEBUG((3, "%s: invalid opponi %i on EMPIREREPORT\n", __func__, opponi));
                }
                ui_data.ui_main_loop_action = UI_MAIN_LOOP_RACES;
                break;
            case UI_MAIN_LOOP_AUDIENCE:
                if ((opponi > 0) && (opponi < g->players)) {
                    game_audience(g, pi, opponi);
                } else {
                    LOG_DEBUG((3, "%s: invalid opponi %i on AUDIENCE\n", __func__, opponi));
                }
                ui_data.ui_main_loop_action = UI_MAIN_LOOP_RACES;
                break;
            case UI_MAIN_LOOP_STARVIEW:
                ui_starview(g, pi);
                ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
                break;
            case UI_MAIN_LOOP_TECH:
                ui_tech(g, pi);
                ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
                break;
            case UI_MAIN_LOOP_SCRAP_BASES:
                ui_basescrap(g, pi);
                ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
                break;
            case UI_MAIN_LOOP_SPIES_CAUGHT:
                ui_caught(g, pi);
                ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
                break;
            case UI_MAIN_LOOP_NEXT_TURN:
                ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
                ui_data.news.flag_also = false;
                return UI_TURN_ACT_NEXT_TURN;
            default:
                LOG_DEBUG((3, "%s: unimpl 0x%x\n", __func__, ui_data.ui_main_loop_action));
                ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
                break;
        }
    }
    return UI_TURN_ACT_QUIT_GAME;
}

void ui_game_start(struct game_s *g)
{
    for (int i = 0; i < g->nebula_num; ++i) {
        ui_data.gfx.starmap.nebula[i] = lbxfile_item_get(LBXFILE_STARMAP, 0xf + g->nebula_type[i], 0);
        ui_data.gfx.starmap.smnebula[i] = ui_data.gfx.starmap.smneb[g->nebula_type[i] + g->galaxy_size * 10];
    }
    ui_data.gfx.starmap.bmap = lbxfile_item_get(LBXFILE_V11, 1 + g->galaxy_size, 0);

    /* HACK remove visual glitch on load game */
    ui_draw_erase_buf();
    hw_video_draw_buf();

    lbxpal_select(0, -1, 0);
    lbxpal_build_colortables();
    /* HACK fix wrong palette after new game */
    lbxpal_set_update_range(248, 255);
    ui_palette_set_n();
    ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
    for (int i = 0; i < g->players; ++i) {
        if (BOOLVEC_IS0(g->is_ai, i)) {
            ui_starmap_set_pos_focus(g, i);
            break;
        }
    }
}

void ui_game_end(struct game_s *g)
{
    for (int i = 0; i < NEBULA_MAX; ++i) {
        if (ui_data.gfx.starmap.nebula[i]) {
             lbxfile_item_release(LBXFILE_STARMAP, ui_data.gfx.starmap.nebula[i]);
             ui_data.gfx.starmap.nebula[i] = NULL;
             ui_data.gfx.starmap.smnebula[i] = NULL;
        }
    }
    lbxfile_item_release(LBXFILE_V11, ui_data.gfx.starmap.bmap);
    ui_data.gfx.starmap.bmap = NULL;
}
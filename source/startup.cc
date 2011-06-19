/**
 * @file
 * @brief Collection of startup related functions and objects
**/

#include "AppHdr.h"

#include "arena.h"
#include "cio.h"
#include "command.h"
#include "ctest.h"
#include "database.h"
#include "dbg-maps.h"
#include "defines.h"
#include "dlua.h"
#include "dungeon.h"
#include "env.h"
#include "exclude.h"
#include "files.h"
#include "food.h"
#include "godpassive.h"
#include "hints.h"
#include "initfile.h"
#include "itemname.h"
#include "items.h"
#include "lev-pand.h"
#include "macro.h"
#include "maps.h"
#include "menu.h"
#include "message.h"
#include "misc.h"
#include "mon-cast.h"
#include "mon-util.h"
#include "mutation.h"
#include "newgame.h"
#include "ng-input.h"
#include "ng-setup.h"
#include "notes.h"
#include "options.h"
#include "output.h"
#include "skills2.h"
#include "spl-book.h"
#include "spl-util.h"
#include "stairs.h"
#include "startup.h"
#include "state.h"
#include "status.h"
#include "stuff.h"
#include "terrain.h"
#include "view.h"
#include "viewchar.h"

#ifdef USE_TILE
 #include "tilepick.h"
 #include "tilereg-crt.h"
 #include "tileview.h"
#endif

// Initialise a whole lot of stuff...
static void _initialize()
{
    Options.fixup_options();

    you.symbol = MONS_PLAYER;

    if (Options.seed)
        seed_rng(Options.seed);
    else
        seed_rng();
    get_typeid_array().init(ID_UNKNOWN_TYPE);
    init_char_table(Options.char_set);
    init_show_table();
    init_monster_symbols();
    init_spell_descs();        // This needs to be way up top. {dlb}
    init_zap_index();
    init_mut_index();
    init_duration_index();
    init_mon_name_cache();
    init_mons_spells();

    // init_item_name_cache() needs to be redone after init_char_table()
    // and init_show_table() have been called, so that the glyphs will
    // be set to use with item_names_by_glyph_cache.
    init_item_name_cache();

    msg::initialise_mpr_streams();

    // Init item array.
    for (int i = 0; i < MAX_ITEMS; ++i)
        init_item(i);

    // Empty messaging string.
    info[0] = 0;

    for (int i = 0; i < MAX_MONSTERS; ++i)
        menv[i].reset();

    igrd.init(NON_ITEM);
    mgrd.init(NON_MONSTER);
    env.map_knowledge.init(map_cell());
    env.pgrid.init(0);

    you.unique_creatures.init(false);
    you.unique_items.init(UNIQ_NOT_EXISTS);

    // Set up the Lua interpreter for the dungeon builder.
    init_dungeon_lua();

#ifdef USE_TILE
    // Draw the splash screen before the database gets initialised as that
    // may take awhile and it's better if the player can look at a pretty
    // screen while this happens.
    if (!crawl_state.map_stat_gen && !crawl_state.test
        && crawl_state.title_screen)
    {
        tiles.draw_title();
        tiles.update_title_msg("Loading Databases...");
    }
#endif

    // Initialise internal databases.
    databaseSystemInit();
#ifdef USE_TILE
    if (crawl_state.title_screen)
        tiles.update_title_msg("Loading Spells and Features...");
#endif

    init_feat_desc_cache();
    init_spell_name_cache();
    init_spell_rarities();
#ifdef USE_TILE
    if (crawl_state.title_screen)
        tiles.update_title_msg("Loading maps...");
#endif

    // Read special levels and vaults.
    read_maps();
    run_map_global_preludes();

    if (crawl_state.build_db)
        end(0);

    if (!crawl_state.io_inited)
        cio_init();

    // System initialisation stuff.
    textbackground(0);
#ifdef USE_TILE
    if (!Options.tile_skip_title && crawl_state.title_screen)
    {
        tiles.update_title_msg("Loading complete, press any key to start.");
        tiles.hide_title();
    }
#endif

    clrscr();

#ifdef DEBUG_DIAGNOSTICS
    if (crawl_state.map_stat_gen)
    {
        generate_map_stats();
        end(0, false);
    }
#endif

    if (crawl_state.test)
    {
#if defined(DEBUG_TESTS) && !defined(DEBUG)
#error "DEBUG must be defined if DEBUG_TESTS is defined"
#endif

#if defined(DEBUG_DIAGNOSTICS) || defined(DEBUG_TESTS)
#ifdef USE_TILE
        init_player_doll();
#endif
        crawl_state.show_more_prompt = false;
        crawl_tests::run_tests(true);
        // Superfluous, just to make it clear that this is the end of
        // the line.
        end(0, false);
#else
        end(1, false, "Non-debug Crawl cannot run tests. "
            "Please use a debug build (defined FULLDEBUG, DEBUG_DIAGNOSTIC "
            "or DEBUG_TESTS)");
#endif
    }
}

static void _post_init(bool newc)
{
    ASSERT(strwidth(you.your_name) <= kNameLen);

    // Fix the mutation definitions for the species we're playing.
    fixup_mutations();

    // Load macros
    macro_init();

    crawl_state.need_save = true;
    crawl_state.last_type = crawl_state.type;
    crawl_state.last_game_won = false;

    calc_hp();
    calc_mp();
    food_change(true);

    run_map_local_preludes();

    if (newc && you.char_direction == GDT_GAME_START)
    {
        // Chaos Knights of Lugonu start out in the Abyss.
        you.level_type  = LEVEL_ABYSS;
        you.entry_cause = EC_UNKNOWN;
    }

    // XXX: Any invalid level_id should do.
    level_id old_level;
    old_level.level_type = NUM_LEVEL_AREA_TYPES;

    load(you.entering_level ? you.transit_stair : DNGN_STONE_STAIRS_DOWN_I,
         you.entering_level ? LOAD_ENTER_LEVEL :
         newc               ? LOAD_START_GAME : LOAD_RESTART_GAME,
         old_level);

    if (newc && you.char_direction == GDT_GAME_START)
    {
        // Randomise colours properly for the Abyss.
        init_pandemonium();
    }

#ifdef DEBUG_DIAGNOSTICS
    // Debug compiles display a lot of "hidden" information, so we auto-wiz.
    you.wizard = true;
#endif

    init_properties();
    burden_change();

    you.redraw_stats.init(true);
    you.redraw_armour_class = true;
    you.redraw_evasion      = true;
    you.redraw_experience   = true;
    you.redraw_quiver       = true;
    you.wield_change        = true;

    // Start timer on session.
    you.last_keypress_time = time(NULL);

#ifdef CLUA_BINDINGS
    clua.runhook("chk_startgame", "b", newc);
    std::string yname = you.your_name; // XXX: what's this for?
    read_init_file(true);
    Options.fixup_options();
    you.your_name = yname;

    // In case Lua changed the character set.
    init_char_table(Options.char_set);
    init_show_table();
    init_monster_symbols();
#endif

#ifdef USE_TILE
    // Override inventory weights options for tiled menus.
    if (Options.tile_menu_icons && Options.show_inventory_weights)
        Options.show_inventory_weights = false;

    init_player_doll();

    tiles.resize();
#endif
    update_player_symbol();

    draw_border();
    new_level(!newc);
    update_turn_count();
    update_vision_range();
    you.xray_vision = !!you.duration[DUR_SCRYING];
    init_exclusion_los();
    you.bondage_level = ash_bondage_level();

    trackers_init_new_level(false);

    if (newc) // start a new game
    {
        you.friendly_pickup = Options.default_friendly_pickup;

        // Mark items in inventory as of unknown origin.
        origin_set_inventory(origin_set_unknown);

        // For a new game, wipe out monsters in LOS, and
        // for new hints mode games also the items.
        zap_los_monsters(Hints.hints_events[HINT_SEEN_FIRST_OBJECT]);

        // For a newly started hints mode, turn secret doors into normal ones.
        if (crawl_state.game_is_hints())
            hints_zap_secret_doors();

        if (crawl_state.game_is_zotdef())
            fully_map_level();
    }

#ifdef USE_TILE
    tile_new_level(newc);
#endif

    // This just puts the view up for the first turn.
    viewwindow();

    activate_notes(true);

    // XXX: And run Lua map postludes for D:1. Kinda hacky, it shouldn't really
    // be here.
    if (newc)
        run_map_epilogues();
}

/**
 * Helper for show_startup_menu()
 * constructs the game modes section
 */
static void _construct_game_modes_menu(MenuScroller* menu)
{
#ifdef USE_TILE
    TextTileItem* tmp = NULL;
#else
    TextItem* tmp = NULL;
#endif
    std::string text;

#ifdef USE_TILE
    tmp = new TextTileItem();
    tmp->add_tile(tile_def(tileidx_gametype(GAME_TYPE_NORMAL), TEX_GUI));
#else
    tmp = new TextItem();
#endif
    text = "Dungeon Crawl";
    tmp->set_text(text);
    tmp->set_fg_colour(WHITE);
    tmp->set_highlight_colour(WHITE);
    tmp->set_id(GAME_TYPE_NORMAL);
    // Scroller does not care about x-coordinates and only cares about
    // item height obtained from max.y - min.y
    tmp->set_bounds(coord_def(1, 1), coord_def(1, 2));
    tmp->set_description_text("Dungeon Crawl: The main game: full of monsters, "
                              "items, gods and danger!");
    menu->attach_item(tmp);
    tmp->set_visible(true);

#ifdef USE_TILE
    tmp = new TextTileItem();
    tmp->add_tile(tile_def(tileidx_gametype(GAME_TYPE_TUTORIAL), TEX_GUI));
#else
    tmp = new TextItem();
#endif
    text = "Tutorial for Dungeon Crawl";
    tmp->set_text(text);
    tmp->set_fg_colour(WHITE);
    tmp->set_highlight_colour(WHITE);
    tmp->set_id(GAME_TYPE_TUTORIAL);
    // Scroller does not care about x-coordinates and only cares about
    // item height obtained from max.y - min.y
    tmp->set_bounds(coord_def(1, 1), coord_def(1, 2));
    tmp->set_description_text("Tutorial that covers the basics of "
                              "Dungeon Crawl survival.");
    menu->attach_item(tmp);
    tmp->set_visible(true);

#ifdef USE_TILE
    tmp = new TextTileItem();
    tmp->add_tile(tile_def(tileidx_gametype(GAME_TYPE_HINTS), TEX_GUI));
#else
    tmp = new TextItem();
#endif
    text = "Hints mode for Dungeon Crawl";
    tmp->set_text(text);
    tmp->set_fg_colour(WHITE);
    tmp->set_highlight_colour(WHITE);
    tmp->set_id(GAME_TYPE_HINTS);
    // Scroller does not care about x-coordinates and only cares about
    // item height obtained from max.y - min.y
    tmp->set_bounds(coord_def(1, 1), coord_def(1, 2));
    tmp->set_description_text("A mostly normal game that provides more "
                              "advanced hints than the tutorial.");
    menu->attach_item(tmp);
    tmp->set_visible(true);

#ifdef USE_TILE
    tmp = new TextTileItem();
    tmp->add_tile(tile_def(tileidx_gametype(GAME_TYPE_SPRINT), TEX_GUI));
#else
    tmp = new TextItem();
#endif
    text = "Dungeon Sprint";
    tmp->set_text(text);
    tmp->set_fg_colour(WHITE);
    tmp->set_highlight_colour(WHITE);
    tmp->set_id(GAME_TYPE_SPRINT);
    // Scroller does not care about x-coordinates and only cares about
    // item height obtained from max.y - min.y
    tmp->set_bounds(coord_def(1, 1), coord_def(1, 2));
    tmp->set_description_text("Hard, fixed single level game mode.");
    menu->attach_item(tmp);
    tmp->set_visible(true);

#ifdef ENABLE_ZOTDEF
#ifdef USE_TILE
    tmp = new TextTileItem();
    tmp->add_tile(tile_def(tileidx_gametype(GAME_TYPE_ZOTDEF), TEX_GUI));
#else
    tmp = new TextItem();
#endif
    text = "Zot Defence";
    tmp->set_text(text);
    tmp->set_fg_colour(WHITE);
    tmp->set_highlight_colour(WHITE);
    tmp->set_id(GAME_TYPE_ZOTDEF);
    // Scroller does not care about x-coordinates and only cares about
    // item height obtained from max.y - min.y
    tmp->set_bounds(coord_def(1, 1), coord_def(1, 2));
    tmp->set_description_text("Defend the Orb of Zot against waves of critters.");
    menu->attach_item(tmp);
    tmp->set_visible(true);
#endif

#ifdef USE_TILE
    tmp = new TextTileItem();
    tmp->add_tile(tile_def(tileidx_gametype(GAME_TYPE_INSTRUCTIONS), TEX_GUI));
#else
    tmp = new TextItem();
#endif
    text = "Instructions";
    tmp->set_text(text);
    tmp->set_fg_colour(WHITE);
    tmp->set_highlight_colour(WHITE);
    tmp->set_id('?');
    // Scroller does not care about x-coordinates and only cares about
    // item height obtained from max.y - min.y
    tmp->set_bounds(coord_def(1, 1), coord_def(1, 2));
    tmp->set_description_text("Help menu.");
    menu->attach_item(tmp);
    tmp->set_visible(true);

#ifdef USE_TILE
    tmp = new TextTileItem();
    tmp->add_tile(tile_def(tileidx_gametype(GAME_TYPE_ARENA), TEX_GUI));
#else
    tmp = new TextItem();
#endif
    text = "The Arena";
    tmp->set_text(text);
    tmp->set_fg_colour(WHITE);
    tmp->set_highlight_colour(WHITE);
    tmp->set_id(GAME_TYPE_ARENA);
    // Scroller does not care about x-coordinates and only cares about
    // item height obtained from max.y - min.y
    tmp->set_bounds(coord_def(1, 1), coord_def(1, 2));
    tmp->set_description_text("Pit computer controlled teams versus each other!");
    menu->attach_item(tmp);
    tmp->set_visible(true);
}

static void _construct_save_games_menu(MenuScroller* menu,
                       const std::vector<player_save_info>& chars)
{
    if (chars.size() == 0)
    {
        // no saves
        return;
    }

    std::string text;

    std::vector<player_save_info>::iterator it;
    for (unsigned int i = 0; i < chars.size(); ++i)
    {
#ifdef USE_TILE
        SaveMenuItem* tmp = new SaveMenuItem();
#else
        TextItem* tmp = new TextItem();
#endif
        tmp->set_text(chars.at(i).short_desc());
        tmp->set_bounds(coord_def(1, 1), coord_def(1, 2));
        tmp->set_fg_colour(WHITE);
        tmp->set_highlight_colour(WHITE);
        // unique id
        tmp->set_id(NUM_GAME_TYPE + i);
#ifdef USE_TILE
        tmp->set_doll(chars.at(i).doll);
#endif
        //tmp->set_description_text("...");
        menu->attach_item(tmp);
        tmp->set_visible(true);
    }
}

// Should probably use some std::find invocation instead.
static int _find_save(const std::vector<player_save_info>& chars,
                      const std::string& name)
{
    for (int i = 0; i < static_cast<int>(chars.size()); ++i)
        if (chars[i].name == name)
            return (i);
    return (-1);
}

static bool _game_defined(const newgame_def& ng)
{
    return (ng.type != NUM_GAME_TYPE
            && ng.species != SP_UNKNOWN
            && ng.job != JOB_UNKNOWN);
}

static const int SCROLLER_MARGIN_X  = 18;
static const int NAME_START_Y       = 5;
static const int GAME_MODES_START_Y = 7;
static const int SAVE_GAMES_START_Y = GAME_MODES_START_Y + 1 + NUM_GAME_TYPE;
static const int MISC_TEXT_START_Y  = 19;
static const int GAME_MODES_WIDTH   = 60;
static const int NUM_HELP_LINES     = 3;
static const int NUM_MISC_LINES     = 5;

// Display more than just two saved characters if more are available
// and there's enough space.
static int _misc_text_start_y(int num)
{
    const int max_lines = get_number_of_lines() - NUM_MISC_LINES;

#ifdef USE_TILE
    return (max_lines);
#else
    if (num <= 2)
        return (MISC_TEXT_START_Y);

    return (std::min(MISC_TEXT_START_Y + num - 1, max_lines));
#endif
}

/**
 * Saves game mode and player name to ng_choice.
 */
static void _show_startup_menu(newgame_def* ng_choice,
                               const newgame_def& defaults)
{
    std::vector<player_save_info> chars = find_all_saved_characters();
    const int num_saves = chars.size();
    static int type = GAME_TYPE_UNSPECIFIED;

#ifdef USE_TILE
    const int max_col    = tiles.get_crt()->mx;
#else
    const int max_col    = get_number_of_cols() - 1;
#endif
    const int max_line   = get_number_of_lines() - 1;
    const int help_start = _misc_text_start_y(num_saves);
    const int help_end   = help_start + NUM_HELP_LINES + 1;
    const int desc_y     = help_end;
#ifdef USE_TILE
    const int game_mode_bottom = GAME_MODES_START_Y + tiles.to_lines(NUM_GAME_TYPE);
    const int game_save_top = help_start - 2 - tiles.to_lines(std::min(2, num_saves));
    const int save_games_start_y = std::min<int>(game_mode_bottom, game_save_top);
#else
    const int save_games_start_y = SAVE_GAMES_START_Y;
#endif

    clrscr();
    PrecisionMenu menu;
    menu.set_select_type(PrecisionMenu::PRECISION_SINGLESELECT);
    MenuFreeform* freeform = new MenuFreeform();
    freeform->init(coord_def(1, 1), coord_def(max_col, max_line), "freeform");
    // This freeform will only containt unfocusable texts
    freeform->allow_focus(false);
    MenuScroller* game_modes = new MenuScroller();
    game_modes->init(coord_def(SCROLLER_MARGIN_X, GAME_MODES_START_Y),
                     coord_def(GAME_MODES_WIDTH, save_games_start_y),
                     "game modes");

    MenuScroller* save_games = new MenuScroller();
    save_games->init(coord_def(SCROLLER_MARGIN_X, save_games_start_y),
                     coord_def(max_col, help_start - 1),
                     "save games");
    _construct_game_modes_menu(game_modes);
    _construct_save_games_menu(save_games, chars);

    NoSelectTextItem* tmp = new NoSelectTextItem();
    tmp->set_text("Enter your name:");
    tmp->set_bounds(coord_def(1, NAME_START_Y),
                    coord_def(SCROLLER_MARGIN_X, NAME_START_Y + 1));
    freeform->attach_item(tmp);
    tmp->set_visible(true);

    tmp = new NoSelectTextItem();
    tmp->set_text("Choices:");
    tmp->set_bounds(coord_def(1, GAME_MODES_START_Y),
                    coord_def(SCROLLER_MARGIN_X, GAME_MODES_START_Y + 1));
    freeform->attach_item(tmp);
    tmp->set_visible(true);

    if (num_saves)
    {
        tmp = new NoSelectTextItem();
        tmp->set_text("Saved games:");
        tmp->set_bounds(coord_def(1, save_games_start_y),
                        coord_def(SCROLLER_MARGIN_X, save_games_start_y + 1));
        freeform->attach_item(tmp);
        tmp->set_visible(true);
    }

    tmp = new NoSelectTextItem();
    std::string text = "Use the up/down keys to select the type of game "
                       "or load a character.\n"
                       "You can type your name; if you leave it blank "
                       "you will be asked later.\n"
                       "Press Enter to start";
    // TODO: this should include a description of that character.
    if (_game_defined(defaults))
        text += ", Tab to repeat the last game's choice";
    text += ".\n";
    tmp->set_text(text);
    tmp->set_bounds(coord_def(1, help_start), coord_def(max_col - 1, help_end));
    freeform->attach_item(tmp);
    tmp->set_visible(true);

    menu.attach_object(freeform);
    menu.attach_object(game_modes);
    menu.attach_object(save_games);

    MenuDescriptor* descriptor = new MenuDescriptor(&menu);
    descriptor->init(coord_def(1, desc_y), coord_def(max_col, desc_y + 1),
                     "descriptor");
    menu.attach_object(descriptor);

#ifdef USE_TILE
    // Black and White highlighter looks kinda bad on tiles
    BoxMenuHighlighter* highlighter = new BoxMenuHighlighter(&menu);
#else
    BlackWhiteHighlighter* highlighter = new BlackWhiteHighlighter(&menu);
#endif
    highlighter->init(coord_def(-1, -1), coord_def(-1, -1), "highlighter");
    menu.attach_object(highlighter);

#ifdef USE_TILE
    tiles.get_crt()->attach_menu(&menu);
#endif

    freeform->set_visible(true);
    game_modes->set_visible(true);
    save_games->set_visible(true);
    descriptor->set_visible(true);
    highlighter->set_visible(true);

    // Draw legal info etc
    opening_screen();

    std::string input_string = defaults.name;

    // If the game filled in a complete name, the user will
    // usually want to enter a new name instead of adding
    // to the current one.
    bool full_name = !input_string.empty();

    int save = _find_save(chars, input_string);
    if (type != GAME_TYPE_UNSPECIFIED)
    {
        menu.set_active_object(game_modes);
        game_modes->set_active_item(type);
    }
    else if (save != -1)
    {
        menu.set_active_object(save_games);
        // save game id is offset by NUM_GAME_TYPE
        save_games->set_active_item(NUM_GAME_TYPE + save);
    }
    else if (defaults.type != NUM_GAME_TYPE)
    {
        menu.set_active_object(game_modes);
        game_modes->set_active_item(defaults.type);
    }
    else if (!chars.empty())
    {
        menu.set_active_object(save_games);
        save_games->activate_first_item();
    }
    else
    {
        menu.set_active_object(game_modes);
        game_modes->activate_first_item();
    }

    while (true)
    {
        menu.draw_menu();
        textcolor(WHITE);
        cgotoxy(SCROLLER_MARGIN_X, NAME_START_Y);
        clear_to_end_of_line();
        cgotoxy(SCROLLER_MARGIN_X, NAME_START_Y);
        cprintf("%s", input_string.c_str());

        const int keyn = getch_ck();

        if (key_is_escape(keyn))
        {
            // End the game
            end(0);
        }
        else if (keyn == '\t' && _game_defined(defaults))
        {
            *ng_choice = defaults;
            return;
        }

        if (!menu.process_key(keyn))
        {
            // handle the non-action keys by hand to poll input
            // Only consider alphanumeric keys and -_ .
            bool changed_name = false;
            if (std::iswalnum(keyn) || keyn == '-' || keyn == '.'
                || keyn == '_' || keyn == ' ')
            {
                if (full_name)
                {
                    full_name = false;
                    input_string = "";
                }
                if (strwidth(input_string) < kNameLen)
                {
                    input_string += stringize_glyph(keyn);
                    changed_name = true;
                }
            }
            else if (keyn == CK_BKSP)
            {
                if (!input_string.empty())
                {
                    if (full_name)
                        input_string = "";
                    else
                        input_string.erase(input_string.size() - 1);
                    changed_name = true;
                    full_name = false;
                }
            }
            // clear the "That's a silly name line"
            cgotoxy(SCROLLER_MARGIN_X, GAME_MODES_START_Y - 1);
            clear_to_end_of_line();

            // Depending on whether the current name occurs
            // in the saved games, update the active object.
            // We want enter to start a new game if no character
            // with the given name exists, or load the corresponding
            // game.
            if (changed_name)
            {
                int i = _find_save(chars, input_string);
                if (i == -1)
                {
                    menu.set_active_object(game_modes);
                }
                else
                {
                    menu.set_active_object(save_games);
                    // save game ID is offset by NUM_GAME_TYPE
                    save_games->set_active_item(NUM_GAME_TYPE + i);
                }
            }
            else
            {
                // Menu might have changed selection -- sync name.
                type = menu.get_active_item()->get_id();
                switch (type)
                {
                case GAME_TYPE_ARENA:
                    input_string = "";
                    break;
                case GAME_TYPE_NORMAL:
                case GAME_TYPE_TUTORIAL:
                case GAME_TYPE_SPRINT:
                case GAME_TYPE_ZOTDEF:
                case GAME_TYPE_HINTS:
                    // If a game type is chosen, the user expects
                    // to start a new game. Just blanking the name
                    // it it clashes for now.
                    if (_find_save(chars, input_string) != -1)
                        input_string = "";
                    break;

                case '?':
                    break;

                default:
                    int save_number = type - NUM_GAME_TYPE;
                    input_string = chars.at(save_number).name;
                    full_name = true;
                    break;
                }
            }
        }
        // we had a significant action!
        std::vector<MenuItem*> selected = menu.get_selected_items();
        if (selected.size() == 0)
        {
            // Uninteresting action, poll a new key
            continue;
        }

        int id = selected.at(0)->get_id();
        switch (id)
        {
        case GAME_TYPE_NORMAL:
        case GAME_TYPE_TUTORIAL:
        case GAME_TYPE_SPRINT:
        case GAME_TYPE_ZOTDEF:
        case GAME_TYPE_HINTS:
            trim_string(input_string);
            if (is_good_name(input_string, true, false))
            {
                ng_choice->type = static_cast<game_type>(id);
                ng_choice->name = input_string;
                return;
            }
            else
            {
                // bad name
                textcolor(RED);
                cgotoxy(SCROLLER_MARGIN_X ,GAME_MODES_START_Y - 1);
                clear_to_end_of_line();
                cprintf("That's a silly name");
            }
            continue;

        case GAME_TYPE_ARENA:
            ng_choice->type = GAME_TYPE_ARENA;
            return;

        case '?':
            list_commands();
            // recursive escape because help messes up CRTRegion
            _show_startup_menu(ng_choice, defaults);
            return;

        default:
            // It was a savegame instead
            const int save_number = id - NUM_GAME_TYPE;
            // Save the savegame character name
            ng_choice->name = chars.at(save_number).name;
            ng_choice->type = chars.at(save_number).saved_game_type;
            return;
        }
    }
}

static void _choose_arena_teams(newgame_def* choice,
                                const newgame_def& defaults)
{
    if (!choice->arena_teams.empty())
        return;

    clear_message_store();
    clrscr();

    cprintf("Enter your choice of teams:\n");

    cgotoxy(1, 4);
    if (!defaults.arena_teams.empty())
        cprintf("Enter - %s\n", defaults.arena_teams.c_str());
    cprintf("\n");
    cprintf("Examples:\n");
    cprintf("  Sigmund v Jessica\n");
    cprintf("  99 orc v the royal jelly\n");
    cprintf("  20-headed hydra v 10 kobold ; scimitar ego:flaming\n");
    cgotoxy(1, 2);

    char buf[80];
    if (cancelable_get_line(buf, sizeof(buf)))
        game_ended();
    choice->arena_teams = buf;
    if (choice->arena_teams.empty())
        choice->arena_teams = defaults.arena_teams;
}

bool startup_step()
{
    std::string name;

    _initialize();

    newgame_def choice   = Options.game;

    // Setup base game type *before* reading startup prefs -- the prefs file
    // may be in a game-specific subdirectory.
    crawl_state.type = choice.type;

    newgame_def defaults = read_startup_prefs();

    // Set the crawl_state gametype to the requested game type. This must
    // be done before looking for the savegame or the startup prefs file.
    if (crawl_state.type == GAME_TYPE_UNSPECIFIED
        && defaults.type != GAME_TYPE_UNSPECIFIED)
    {
        crawl_state.type = defaults.type;
    }

    // Name from environment overwrites the one from command line.
    if (!SysEnv.crawl_name.empty())
        choice.name = SysEnv.crawl_name;


#ifndef DGAMELAUNCH
    if (crawl_state.last_type == GAME_TYPE_TUTORIAL
        || crawl_state.last_type == GAME_TYPE_SPRINT)
    {
        choice.type = crawl_state.last_type;
        crawl_state.type = crawl_state.last_type;
        crawl_state.last_type = GAME_TYPE_UNSPECIFIED;
        choice.name = defaults.name;
        if (choice.type == GAME_TYPE_TUTORIAL)
            choose_tutorial_character(&choice);
    }
    // We could also check whether game type has been set here,
    // but it's probably not necessary to choose non-default game
    // types while specifying a name externally.
    else if (!is_good_name(choice.name, false, false)
        && choice.type != GAME_TYPE_ARENA)
    {
        _show_startup_menu(&choice, defaults);
        // [ds] Must set game type here, or we won't be able to load
        // Sprint saves.
        crawl_state.type = choice.type;
    }
#endif

    // TODO: integrate arena better with
    //       choose_game and setup_game
    if (choice.type == GAME_TYPE_ARENA)
    {
        _choose_arena_teams(&choice, defaults);
        write_newgame_options_file(choice);
        run_arena(choice.arena_teams);
        end(0, false);
    }

    bool newchar = false;
    if (save_exists(choice.name))
    {
        restore_game(choice.name);
        save_player_name();
    }
    else
    {
        newgame_def ng;
        bool restore = choose_game(&ng, &choice, defaults);
        if (restore)
        {
            restore_game(ng.name);
            save_player_name();
        }
        else
        {
            setup_game(ng);
            newchar = true;
        }
    }

    _post_init(newchar);

    return (newchar);
}

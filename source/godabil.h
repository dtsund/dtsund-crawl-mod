/**
 * @file
 * @brief God-granted abilities.
**/

#ifndef GODABIL_H
#define GODABIL_H

#include "enum.h"
#include "externs.h"
#include "mon-info.h"

struct bolt;

std::string zin_recite_text(int* trits, size_t len, int prayertype, int step);
bool zin_sustenance(bool actual = true);
bool zin_check_able_to_recite();
int zin_check_recite_to_monsters(recite_type *prayertype);
bool zin_recite_to_single_monster(const coord_def& where,
                                  recite_type prayertype);
void zin_saltify(monster* mon);
bool zin_vitalisation();
void zin_remove_divine_stamina();
bool zin_remove_all_mutations();
bool zin_sanctuary();
bool zin_recite();
bool zin_imprison();

void tso_divine_shield();
void tso_remove_divine_shield();
void tso_cleansing_flame();
void tso_summon_divine_warrior();

void elyvilon_purification();
bool elyvilon_divine_vigour();
void elyvilon_remove_divine_vigour();
void elyvilon_request_lifesaving();
bool elyvilon_lesser_healing(const bool self);
bool elyvilon_greater_healing(const ability_type abil);
void elyvilon_restoration();

bool vehumet_supports_spell(spell_type spell);

bool trog_burn_spellbooks();
void trog_berserk();
void trog_trogs_hand();
void trog_brothers_in_arms();

bool jiyva_can_paralyse_jellies();
void jiyva_paralyse_jellies();
bool jiyva_remove_bad_mutation();
bool jiyva_call_jelly();
void jiyva_slimify();

bool beogh_water_walk();
bool beogh_smiting();
void beogh_recall_orcish_followers();

bool yred_injury_mirror();
void yred_request_injury_mirror();
bool yred_can_animate_dead();
void yred_animate_remains_or_dead();
void yred_drain_life();
void yred_make_enslaved_soul(monster* mon, bool force_hostile = false);
void yred_recall_undead_slaves();
bool yred_enslave_soul();

bool kiku_receive_corpses(int pow, coord_def where);
bool kiku_take_corpse();
bool kiku_torment();

bool fedhas_passthrough_class(const monster_type mc);
bool fedhas_passthrough(const monster* target);
bool fedhas_passthrough(const monster_info* target);
bool fedhas_shoot_through(const bolt & beam, const monster* victim);
int fedhas_fungal_bloom();
bool fedhas_sunlight();
bool prioritise_adjacent(const coord_def &target,
                         std::vector<coord_def> &candidates);
bool fedhas_plant_ring_from_fruit();
int fedhas_rain(const coord_def &target);
int fedhas_corpse_spores(beh_type behavior = BEH_FRIENDLY,
                         bool interactive = true);
bool mons_is_evolvable(const monster* mon);
bool fedhas_evolve_flora();
bool fedhas_spawn_spores();

void lugonu_bend_space();
void lugonu_abyss_exit();
bool lugonu_banish();
bool lugonu_corrupt();
void lugonu_abyss_enter();

bool is_ponderousifiable(const item_def& item);
bool ponderousify_armour();
void cheibriados_time_bend(int pow);
int cheibriados_slouch(int pow);
void cheibriados_time_step(int pow);

bool ashenzari_transfer_knowledge();
bool ashenzari_end_transfer(bool finished = false, bool force = false);
void ashenzari_scrying();

void okawaru_might();
void okawaru_haste();
//The below two currently aren't used, but I do hate throwing away code outright.
void okawaru_heroism();
bool okawaru_finesse();

void sif_muna_channel_energy();
bool sif_muna_forget_spell();

bool makhleb_minor_destruction();
void makhleb_lesser_servant_of_makhleb();
bool makhleb_major_destruction();
void makhleb_greater_servant_of_makhleb();

#endif

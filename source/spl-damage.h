#ifndef SPL_DAMAGE_H
#define SPL_DAMAGE_H

#include "enum.h"

struct bolt;
class dist;

bool fireball(int pow, bolt &beam);
void setup_fire_storm(const actor *source, int pow, bolt &beam);
bool cast_fire_storm(int pow, bolt &beam);
bool cast_hellfire_burst(int pow, bolt &beam);
void cast_chain_lightning(int pow, const actor *caster);

void cast_toxic_radiance(bool non_player = false);
void cast_refrigeration(int pow, bool non_player = false,
                        bool freeze_potions = true);
bool vampiric_drain(int pow, monster* mons);
bool cast_freeze(int pow, monster* mons);

int airstrike(int pow, const dist &beam);

void cast_shatter(int pow);
void cast_ignite_poison(int pow);
void cast_discharge(int pow);
int disperse_monsters(coord_def where, int pow);
void cast_dispersal(int pow);
bool cast_fragmentation(int powc, const dist& spd);
int wielding_rocks();
bool cast_sandblast(int powc, bolt &beam);
bool cast_tornado(int powc);
void tornado_damage(actor *caster, int dur);
void cancel_tornado();

#endif

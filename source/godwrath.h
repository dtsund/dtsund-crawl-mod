/**
 * @file
 * @brief Divine retribution.
**/


#ifndef GODWRATH_H
#define GODWRATH_H

bool divine_retribution(god_type god, bool no_bonus = false, bool force = false);
bool do_god_revenge(conduct_type thing_done);
int ash_reduce_xp(int amount);
#endif

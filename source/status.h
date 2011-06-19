/**
 * @file
 * @brief Collects information for output of status effects.
**/

#ifndef STATUS_H
#define STATUS_H

enum status_type
{
    STATUS_AIRBORNE = NUM_DURATIONS + 1,
    STATUS_BEHELD,
    STATUS_BURDEN,
    STATUS_GLOW,
    STATUS_NET,
    STATUS_HUNGER,
    STATUS_REGENERATION,
    STATUS_ROT,
    STATUS_SICK,
    STATUS_SPEED,
    STATUS_CLINGING,
    STATUS_STR_ZERO,
    STATUS_INT_ZERO,
    STATUS_DEX_ZERO,
};

struct status_info
{
    int light_colour;
    std::string light_text; // status light
    std::string short_text; // @: line
    std::string long_text;  // @ message
};

int dur_colour(int exp_colour, bool expiring);

// status should be a duration or status_type
// *info will be filled in as appropriate for current
// character state
void fill_status_info(int status, status_info* info);

void init_duration_index();

#endif

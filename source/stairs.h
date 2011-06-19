/**
 * @file
 * @brief Moving between levels.
**/

#ifndef STAIRS_H
#define STAIRS_H

bool check_annotation_exclusion_warning();
void down_stairs(dungeon_feature_type force_stair = DNGN_UNSEEN,
                 entry_cause_type entry_cause = EC_UNKNOWN,
                 const level_id* force_dest = NULL);
void up_stairs(dungeon_feature_type force_stair = DNGN_UNSEEN,
               entry_cause_type entry_cause = EC_UNKNOWN);
void new_level(bool restore = false);
int runes_in_pack(std::vector<int> &runes);

dungeon_feature_type random_stair(bool do_place_check = true);
#endif

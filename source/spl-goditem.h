#ifndef SPL_GODITEM_H
#define SPL_GODITEM_H

int identify(int power, int item_slot = -1, std::string *pre_msg = NULL);
int cast_healing(int pow, bool divine_ability = false,
                 const coord_def& where = coord_def(0, 0),
                 bool not_self = false, targ_mode_type mode = TARG_NUM_MODES);
bool cast_revivification(int pow);

void antimagic();

void cast_detect_secret_doors(int pow);
int detect_traps(int pow);
int detect_items(int pow);
int detect_creatures(int pow, bool telepathic = false);
bool remove_curse(bool alreadyknown = true, std::string *pre_msg = NULL);
bool curse_item(bool armour, bool alreadyknown, std::string *pre_msg = NULL);
bool detect_curse(int scroll, bool suppress_msg);

bool entomb(int pow);
bool cast_imprison(int pow, monster* mons, int source);

bool cast_smiting(int pow, monster* mons);

#endif

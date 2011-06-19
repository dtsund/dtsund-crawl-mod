/**
 * @file
 * @brief Functions used when starting a new game.
**/


#ifndef NEWGAME_H
#define NEWGAME_H

#include "newgame_def.h"
#include "menu.h"

undead_state_type get_undead_state(const species_type sp);
void choose_tutorial_character(newgame_def* ng_choice);

bool choose_game(newgame_def *ng, newgame_def* choice,
                 const newgame_def& defaults);

void make_rod(item_def &item, stave_type rod_type, int ncharges);
int start_to_wand(int wandtype, bool& is_rod);
int start_to_book(int firstbook, int booktype);

#define JOB_GROUP_SIZE 9
/*
 * A structure for grouping backgrounds by category.
 */
struct job_group
{
    const char* name;   //<! Name of the group.
    coord_def position; //<! Relative coordinates of the title.
    int width;          //<! Column width.
    job_type jobs[JOB_GROUP_SIZE]; //<! List of jobs in the group.

    //! A method to attach the group to a freeform.
    void attach(const newgame_def* ng, const newgame_def& defaults,
                MenuFreeform* menu, menu_letter &letter);
};

#endif

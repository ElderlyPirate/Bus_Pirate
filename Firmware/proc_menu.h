/*
 * This file is part of the Bus Pirate project
 * (https://github.com/BusPirate/Bus_Pirate/).
 *
 * Written and maintained by the Bus Pirate project.
 *
 * To the extent possible under law, the project has waived all copyright and
 * related or neighboring rights to Bus Pirate. This work is published from
 * United States.
 *
 * For details see: http://creativecommons.org/publicdomain/zero/1.0/
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#ifndef BP_PROC_MENU_H
#define BP_PROC_MENU_H

#define BELL 0x07

#define CMDLENMSK (BP_COMMAND_BUFFER_SIZE - 1)

extern char cmdbuf[BP_COMMAND_BUFFER_SIZE];
extern unsigned int cmdend;
extern unsigned int cmdstart;
extern bool command_error;

void print_version_info(void);

void serviceuser(void);

int getint(void);
int getrepeat(void);
int getnumbits(void);

void consumewhitechars(void);
void changemode(void);

/**
 * @brief Switches to the given bus mode, applying each protocol's default
 * settings, without any user interaction.
 *
 * This is what a BASIC program needs to configure the bus on its own: it runs
 * the same setup callbacks that an interactive mode change uses, but the
 * setup prompts answer their defaults instead of waiting for input. The pins
 * of the new mode are configured right away, because a script never issues
 * the power-on request that engages the bus in the interactive flow.
 *
 * @param[in] mode the mode to switch to, one-based as the mode menu lists it.
 *
 * @return 0 if the switch succeeded, -1 if the mode does not exist.
 */
int bp_change_mode(int mode);
#ifdef BP_ENABLE_COMMAND_HISTORY
int cmdhistory(void);
#endif /* BP_ENABLE_COMMAND_HISTORY */

int getnumber(int def, int min, int max, int x);
#ifdef BUSPIRATEV4
long getlong(long def, int min, long max, int x);
#endif /* BUSPIRATEV4 */

#endif /* !BP_PROC_MENU_H */
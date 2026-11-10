/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Wait bounding for the interaction CPU boost
 * (specs/0020-ui-interaction-boost.md). No firmware includes, so
 * tests/test_ui_boost.c can exercise it on the host.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef UI_BOOST_H
#define UI_BOOST_H

#include <stdbool.h>

#define UI_BOOST_TIMEOUT_BLOCK (-1)   /* same value as TIMEOUT_BLOCK */

/* Ticks to wait for input before re-checking the boost: the caller's
 * timeout, cut short at the boost deadline while a boost is held. */
static inline long ui_boost_wait(long timeout, bool boosted, long left)
{
    if (!boosted)
        return timeout;
    if (left <= 0)
        return 0;
    if (timeout == UI_BOOST_TIMEOUT_BLOCK || timeout > left)
        return left;
    return timeout;
}

struct ui_boost_state
{
    bool boosted;
    long until;     /* tick at which the boost is dropped */
};

struct ui_boost_ops
{
    long (*get)(int ticks);         /* button_get_w_tmo() */
    long (*now)(void);              /* current_tick */
    void (*boost)(bool on);         /* cpu_boost() */
    bool (*is_input)(long button);  /* real user input? */
    bool enabled;                   /* setting */
    long hold;                      /* ticks to stay boosted after input */
};

static inline void ui_boost_release(struct ui_boost_state *st,
                                    const struct ui_boost_ops *ops)
{
    if (st->boosted)
    {
        st->boosted = false;
        ops->boost(false);
    }
}

/* Wait for a button like ops->get(timeout), boosting on input and dropping
 * the boost once it has been idle for ops->hold ticks. */
static inline long ui_boost_get_button(struct ui_boost_state *st,
                                       const struct ui_boost_ops *ops,
                                       int timeout)
{
    long deadline = ops->now() + timeout;
    while (true)
    {
        long now = ops->now();
        long remaining = timeout;
        if (timeout > 0)
            remaining = deadline - now > 0 ? deadline - now : 0;
        long button = ops->get((int)ui_boost_wait(remaining, st->boosted,
                                                  st->until - now));
        if (button != 0)
        {
            if (ops->is_input(button))
            {
                if (!ops->enabled)
                    ui_boost_release(st, ops);
                else
                {
                    if (!st->boosted)
                    {
                        st->boosted = true;
                        ops->boost(true);
                    }
                    st->until = ops->now() + ops->hold;
                }
            }
            return button;
        }
        now = ops->now();
        if (st->boosted && now - st->until >= 0)
            ui_boost_release(st, ops);
        if (timeout == 0 || (timeout > 0 && now - deadline >= 0))
            return 0;
    }
}

#endif /* UI_BOOST_H */

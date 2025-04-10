/*
 * Created by v1tr10l7 on 18.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/termios.h>

/*
 * Defaults on "first" open.
 */
constexpr usize TTYDEF_IFLAG = ICRNL | IXON;
constexpr usize TTYDEF_OFLAG = OPOST | ONLCR;
constexpr usize TTYDEF_LFLAG
    = ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE | IEXTEN;
constexpr usize TTYDEF_CFLAG = B38400 | CS8 | CREAD | HUPCL;
constexpr usize TTYDEF_SPEED = 38400;
// constexpr usize TTYDEF_IFLAG = BRKINT | ISTRIP | ICRNL | IMAXBEL | IXON |
// IXANY; constexpr usize TTYDEF_OFLAG = OPOST | ONLCR | XTABS; constexpr usize
// TTYDEF_LFLAG
//     = ECHO | ICANON | ISIG | IEXTEN | ECHOE | ECHOK | ECHOCTL;
// constexpr usize TTYDEF_CFLAG = CREAD | CS8 | PARENB | HUPCL;
// constexpr usize TTYDEF_SPEED = B9600;

/*
 * Control Character Defaults
 */
constexpr usize CTRL(u64 c) { return c & 037; }
constexpr usize CINTR    = CTRL('C');
constexpr usize CQUIT    = 034;
constexpr usize CERASE   = 0177;
constexpr usize CKILL    = CTRL('U');
constexpr usize CEOF     = CTRL('D');
constexpr usize CTIME    = 0;
constexpr usize CMIN     = 1;
constexpr usize CSWTC    = 0;
constexpr usize CSTART   = CTRL('Q');
constexpr usize CSTOP    = CTRL('S');
constexpr usize CSUSP    = CTRL('Z');
constexpr usize CSTATUS  = 0;
constexpr usize CDSUSP   = CTRL('Y');
constexpr usize CEOL     = 0;
constexpr usize CREPRINT = CTRL('R');
constexpr usize CDISCARD = CTRL('O');
constexpr usize CWERASE  = CTRL('W');
constexpr usize CLNEXT   = CTRL('V');
constexpr usize CEOL2    = CEOL;

constexpr usize CEOT     = CEOF;
constexpr usize CBRK     = CEOL;
constexpr usize CRPRNT   = CREPRINT;
constexpr usize CFLUSH   = CDISCARD;

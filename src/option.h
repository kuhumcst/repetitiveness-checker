/*
Repetitiveness checker

Copyright (C) 2020  Center for Sprogteknologi, University of Copenhagen

This file is part of CST's Language Technology Tools.

REPETITIVENESS CHECKER is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

REPETITIVENESS CHECKER is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with REPETITIVENESS CHECKER; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef OPTION_H
#define OPTION_H

typedef enum {GoOn = 0,Leave = 1,Error = 2} OptReturnTp;

struct optionStruct
    {
    const char * w; // weight
    const char * o; // output
    const char * p; // passes
    bool letters; // morpheme analysis
    optionStruct();
    ~optionStruct();
    OptReturnTp doSwitch(int c,char * locoptarg,char * progname);
    OptReturnTp readOptsFromFile(char * locoptarg,char * progname);
    OptReturnTp readArgs(int argc, char * argv[]);
    };
#endif

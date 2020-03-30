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

#include "option.h"
#include "argopt.h"
#include "repetitions.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <cstddef>


//        printf("usage: makeaffixrules -w <word list> -c <cutoff> -o <flexrules> -e <extra> -n <columns> -f <compfunc> [<word list> [<cutoff> [<flexrules> [<extra> [<columns> [<compfunc>]]]]]]\n");

bool VERBOSE = false;
static char opts[] = "?h@:w:o:p:l" /* GNU: */ "WR";
static char *** Ppoptions = NULL;
static char ** Poptions = NULL;
static int optionSets = 0;

char * dupl(const char * s)
    {
    char * d = new char[strlen(s) + 1];
    strcpy(d,s);
    return d;
    }

optionStruct::optionStruct()
    {
    w = NULL;
    o = NULL;
    p = NULL;
    letters = false;
    }

optionStruct::~optionStruct()
    {
    for(int I = 0;I < optionSets;++I)
        {
        delete [] Poptions[I];
        delete [] Ppoptions[I];
        }
    delete [] w;
    delete [] o;
    delete [] p;
    }

OptReturnTp optionStruct::doSwitch(int optchar,char * locoptarg,char * progname)
    {
    switch (optchar)
        {
        case '@':
            readOptsFromFile(locoptarg,progname);
            break;
        case 'w':
            w = dupl(locoptarg);
            break;
        case 'o':
            o = dupl(locoptarg);
            break;
        case 'p':
            p = dupl(locoptarg);
            break;
        case 'l':
            letters = true;
            break;
        case 'h':
        case '?':
            printf("usage:\n"
                "repver [-@ <option file>] [-w <weight>] [-o <output>] [-p <passes>] [-l] file1 file2 file3 ..."
                "\n");
            printf("-@: Options are read from file with lines formatted as: -<option letter> <value>\n"
                   "    A semicolon comments out the rest of the line.\n"
                );
            printf("-w: weight function 1-10\n");
            printf("-o: (output) list of found prases. Default is standard output\n");
            printf("-p: passes: 1 or 2 (default). (2 to eliminate overlap).\n");
            printf("-l: morpheme analysis on all types in input.\n");
            return Leave;
// GNU >>
        case 'R':
            printf("12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING\n");
            printf("WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR\n");
            printf("REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,\n");
            printf("INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING\n");
            printf("OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED\n");
            printf("TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY\n");
            printf("YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER\n");
            printf("PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE\n");
            printf("POSSIBILITY OF SUCH DAMAGES.\n");
            return Leave;
        case 'W':
            printf("11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY\n");
            printf("FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN\n");
            printf("OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES\n");
            printf("PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED\n");
            printf("OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n");
            printf("MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS\n");
            printf("TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE\n");
            printf("PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,\n");
            printf("REPAIR OR CORRECTION.\n");
            return Leave;
// << GNU
        }
    return GoOn;
    }



OptReturnTp optionStruct::readOptsFromFile(char * locoptarg,char * progname)
    {
    char ** poptions;
    char * options;
    FILE * fpopt = fopen(locoptarg,"r");
    OptReturnTp result = GoOn;
    if(fpopt)
        {
        char * p;
        char line[1000];
        int lineno = 0;
        size_t bufsize = 0;
        while(fgets(line,sizeof(line) - 1,fpopt))
            {
            lineno++;
            size_t off = strspn(line," \t");
            if(line[off] == ';')
                continue; // comment line
            if(line[off] == '-')
                {
                off++;
                if(line[off])
                    {
                    char * optarg2 = line + off + 1;
                    size_t off2 = strspn(optarg2," \t");
                    if(!optarg2[off2])
                        optarg2 = NULL;
                    else
                        optarg2 += off2;
                    if(optarg2)
                        {
                        for(p = optarg2 + strlen(optarg2) - 1;p >= optarg2;--p)
                            {
                            if(!isspace(*p))
                                break;
                            *p = '\0';
                            }
                        bool string = false;
                        if(*optarg2 == '\'' || *optarg2 == '"')
                            {
                            for(p = optarg2 + strlen(optarg2) - 1;p > optarg2;--p)
                                {
                                if(*p == *optarg2)
                                    {
                                    string = true;
                                    for(char * q = p + 1;*q;++q)
                                        {
                                        if(*q == ';')
                                            break;
                                        if(!isspace(*q))
                                            {
                                            string = false;
                                            }
                                        }
                                    if(string)
                                        {
                                        *p = '\0';
                                        ++optarg2;
                                        }
                                    break;
                                    }
                                }
                            }
                        if(!*optarg2 && !string)
                            optarg2 = NULL;
                        }
                    if(optarg2)
                        {
                        bufsize += strlen(optarg2) + 1;
                        }
                    char * optpos = strchr(opts,line[off]);
                    if(optpos)
                        {
                        if(optpos[1] != ':')
                            {
                            if(optarg2)
                                {
                                printf("Option argument %s provided for option letter %c that doesn't use it on line %d in option file \"%s\"\n",optarg2,line[off],lineno,locoptarg);
                                exit(1);
                                }
                            }
                        }
                    }
                else
                    {
                    printf("Missing option letter on line %d in option file \"%s\"\n",lineno,locoptarg);
                    exit(1);
                    }
                }
            }
        rewind(fpopt);

        poptions = new char * [lineno];
        options = new char[bufsize];
        // update stacks that keep pointers to the allocated arrays.
        optionSets++;
        char *** tmpPpoptions = new char **[optionSets];
        char ** tmpPoptions = new char *[optionSets];
        int g;
        for(g = 0;g < optionSets - 1;++g)
            {
            tmpPpoptions[g] = Ppoptions[g];
            tmpPoptions[g] = Poptions[g];
            }
        tmpPpoptions[g] = poptions;
        tmpPoptions[g] = options;
        delete [] Ppoptions;
        Ppoptions = tmpPpoptions;
        delete [] Poptions;
        Poptions = tmpPoptions;

        lineno = 0;
        bufsize = 0;
        while(fgets(line,sizeof(line) - 1,fpopt))
            {
            poptions[lineno] = options+bufsize;
            size_t off = strspn(line," \t");
            if(line[off] == ';')
                continue; // comment line
            if(line[off] == '-')
                {
                off++;
                if(line[off])
                    {
                    char * optarg2 = line + off + 1;
                    size_t off2 = strspn(optarg2," \t");
                    if(!optarg2[off2])
                        optarg2 = NULL;
                    else
                        optarg2 += off2;
                    if(optarg2)
                        {
                        for(p = optarg2 + strlen(optarg2) - 1;p >= optarg2;--p)
                            {
                            if(!isspace(*p))
                                break;
                            *p = '\0';
                            }
                        bool string = false;
                        if(*optarg2 == '\'' || *optarg2 == '"')
                            {
                            for(p = optarg2 + strlen(optarg2) - 1;p > optarg2;--p)
                                {
                                if(*p == *optarg2)
                                    {
                                    string = true;
                                    for(char * q = p + 1;*q;++q)
                                        {
                                        if(*q == ';')
                                            break;
                                        if(!isspace(*q))
                                            {
                                            string = false;
                                            }
                                        }
                                    if(string)
                                        {
                                        *p = '\0';
                                        ++optarg2;
                                        }
                                    break;
                                    }
                                }
                            }
                        if(!*optarg2 && !string)
                            optarg2 = NULL;
                        }
                    if(optarg2)
                        {
                        strcpy(poptions[lineno],optarg2);
                        bufsize += strlen(optarg2) + 1;
                        }
                    OptReturnTp res = doSwitch(line[off],poptions[lineno],progname);
                    if(res > result)
                        result = res;
                    }
                }
            lineno++;
            }
        fclose(fpopt);
        }
    else
        {
        printf("Cannot open option file %s\n",locoptarg);
        }
    return result;
    }

OptReturnTp optionStruct::readArgs(int argc, char * argv[])
    {
    int optchar;
    OptReturnTp result = GoOn;
    while((optchar = getopt(argc,argv, opts)) != -1)
        {
        OptReturnTp res = doSwitch(optchar,optarg,argv[0]);
        if(res > result)
            result = res;
        }
    if(optind < argc)
        {
        }
    return result;
    }


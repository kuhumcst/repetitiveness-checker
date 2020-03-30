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
#ifndef repetitionsH
#define repetitionsH
//---------------------------------------------------------------------------

#define DOUNICODE 1
#include <ctype.h>
#include <stdio.h>
#include <cstddef>

enum charprop
    {firstcharprop
    ,string_constituent
    ,ignored_symbol
    ,atomic_string
    ,string_constituent_to_be_replaced
    ,token_delimiter
    ,sentence_delimiter
#ifdef UNNECESSARYSTUFF
    ,control_character_to_be_kept
    ,character_only_occurring_at_start_of_string
    ,character_only_occurring_at_end_of_string
#endif
    ,lastcharprop
    };

typedef union
    {
    struct
        {
        bool b_task:1;
        bool b_case_sensitive:1;
        bool b_fuzzy:1;
        bool b_limit:1;
        bool b_passes:1;
        bool b_weight:1;
        bool b_repetitiveness:1;
        bool b_formula:1;
        } bools;
    char allflags;
    } flagspreambule;

extern charprop character[256];
extern bool case_sensitive;
extern bool versioncomparison;
extern bool recursive;
extern bool unlimited;
extern int maxlimit;
extern int minlimit;

void fill_character_properties(void);
void ShiftToNextProp(int i);
double ComputeRepetitiveness(char ** sis,double * versionalikeness,bool morphemes);

char ** WriteTextWithMarkings();
//char * WritePhrases();
void WritePhrasesArgHTML
        (char ** names
        ,double * versionalikeness
        ,bool morphemes
        ,FILE * fp
        ,flagspreambule preambule
        ,bool b_phraseno
        ,bool b_realCount
        ,bool b_weight
        ,bool b_getAccumulatedRepetitiveness
        );

unsigned long GetLowestfreq();
unsigned long GetHighestfreq();
ptrdiff_t GetNumberOfTokens();
ptrdiff_t GetNumberOfTokensFile(int fileno);
unsigned long GetNumberOfSentenceSeparatorsFile(int fileno);
unsigned long GetRealUnmatchedFile(int fileno);
unsigned long GetNumberOfTypes();
unsigned long GetLowestfreq(const char ** type);
unsigned long GetHighestfreq(const char ** type);
//unsigned long GetNumberOfUnmatchedWords();
size_t GetFiducialTextLength();
size_t GetReducedTextLength();
unsigned long GetRealUnmatched();
void chooseWeightAsFrequency();
void chooseWeightAsLength();
void chooseWeightAsFrequencyTimesLength();
void chooseWeightAsFrequencyTimesLengthTimesAverageOfInverseOfWordFrequency();
void chooseWeightAsFrequencyTimesLengthTimesAllRatiosOfAverageWordFrequencyByRealWordFequency();
void chooseWeightAsFrequencyTimesLengthTimesAverageOfEntropy();
void chooseWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLength();
//void chooseWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLengthTimesPhraseCountReduction();
void chooseWeightAsFrequencyTimesLengthTimesAverageOfInverseWordFrequencyTimesPhraseCountReduction();
void chooseWeightAs2005();
void chooseWeightAs2005b();
void setUnlimited(bool flag,int editMaxLimit);
void setMaxLimit(int limit);
void setMinLimit(int limit);
void setRecursion(int npasses);
bool weightIsFrequency();
bool weightIsLength();
bool weightIsFrequencyTimesLength();
bool weightIsFrequencyTimesLengthTimesAverageOfInverseOfWordFrequency();
bool weightIsFrequencyTimesLengthTimesAllRatiosOfAverageWordFrequencyByRealWordFequency();
bool weightIsFrequencyTimesLengthTimesAverageOfEntropy();
bool weightIsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLength();
//bool weightIsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLengthTimesPhraseCountReduction();
bool weightIsFrequencyTimesLengthTimesAverageOfInverseWordFrequencyTimesPhraseCountReduction();
bool weightIs2005();
bool weightIs2005b();
void selectFuzzynessBoundary(int perc);
int currentFuzzynessBoundary();

#define ALLOWONEWORDPHRASES
#define ALLOWOVERLAP
#endif

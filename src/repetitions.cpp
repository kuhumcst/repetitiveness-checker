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
/*
  Found the following problematic text (schematically):


A B A B A. B. B A.

There must be at least as many B's as A's.

Finds A B A twice and BA three times.
Reduces A B A to one time, due to overlap, and leaves B A two times.
BUT, A B A one time -> A B A zero times, which gives B A three occurrences!
*/

//#include "stdafx.h"

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>
#include <cstddef>
//#if defined _CONSOLE
#include "argopt.h"
#include "option.h"
//#endif
#include "repetitions.h"
#include "utf8func.h"
#ifdef __BORLANDC__
#include "addtochart.h"
#endif
static unsigned long lowestfreq = ULONG_MAX,highestfreq = 0L;
static unsigned long tokens = 0L,gtokens;
static unsigned long types = 0L;
static unsigned long lowtype = 0L,hightype = 0L;
static unsigned long realUnmatched = 0L;
static size_t fiducialTextLength = 0L;
static size_t reducedTextLength = 0L;
static unsigned long numberOfPhrases = 0;
static unsigned long numberOfSentenceSeparators = 0;
static double repetitiveness = 1.0;
static double averageTypeFrequency = 0.0;
// phrase length = b + m * log(phrase #)
static double m; // gradient
static double b; // offset
static char * textBuffer = NULL;
bool versioncomparison = false;
bool recursive = false;
bool unlimited = true;
static int npasses = 1;
int maxlimit = -1;
int minlimit = 2;
static double logfac[1000];

class leastSquareFitter
    {
    double _x,_y,_xx,_xy;
    unsigned long n;
public:
    leastSquareFitter(): _x(0.0),_y(0.0),_xx(0.0),_xy(0.0),n(0L){}
    void add(double x,double y)
        {
        ++n;
        _x += x;
        _y += y;
        _xx += x*x;
        _xy += x*y;
        }
    double m()
        {
        double denominator = n*_xx - _x*_x;
        if(denominator != 0.0)
            return (n*_xy - _x*_y)/denominator;
        else
            return 10e30;
        }
    double b()
        {
        if(n > 0)
            return (_y - m()*_x)/n;
        else
            return 10e30;
        }
    };


class type;

#define _f_ (1 << 0)
#define _b_ (1 << 1)
#define _B_ (1 << 2)
#define _t_ (1 << 3)
#define _e_ (1 << 4)


typedef struct word
    {
    type * tp;
    long fileStartPos;
    long fileEndPos;
    char marked;
    } word;

static word * lastword = NULL;
static word * afterlastword = NULL;

typedef struct filedata
    {
    filedata()
        {
        filename = NULL;
        }
    ~filedata()
        {
        delete filename;
        }
    char * filename;
    unsigned long realUnmatched;
    unsigned long numberOfSentenceSeparators;
    word * boundary;
    } filedata;

static filedata * filedatalist = NULL;

class phrase
    {
    word * wording;
    phrase * next;
    ptrdiff_t offset;
    size_t length;
    unsigned long count;
    unsigned long realCount;
    double weight;
    size_t LengthOfWorsePhrases;
    double AccumulatedRepetitiveness;
public:
    void setLengthOfWorsePhrases(size_t len)
        {
        LengthOfWorsePhrases = len;
        }
    size_t getLengthOfWorsePhrases()
        {
        return LengthOfWorsePhrases;
        }
    void setAccumulatedRepetitiveness(double repetitiveness)
        {
        AccumulatedRepetitiveness = repetitiveness;
        }
    double getAccumulatedRepetitiveness()
        {
        return AccumulatedRepetitiveness;
        }
    phrase  (word * wording
            ,ptrdiff_t offset
            ,size_t length
            ):wording(wording),
            next(NULL),
            offset(offset),
            length(length),
            count(0L),
            realCount(0L),
            weight(1.0)
        {
        }
    ~phrase()
        {
        wording = NULL;
        delete next;
        next = NULL;
        }
    void add(word * wording
            ,ptrdiff_t offset
            ,size_t length
            )
        {
        if(this->offset == offset
        && this->length == length)
            {
            size_t i;
            for ( i = 0
                ; i < length && this->wording[i].tp == wording[i].tp
                ; ++i
                )
                ;
            if(i == length)
                return; // phrase already found
            }
        if(next)
            next->add(wording,offset,length);
        else
            next = new phrase(wording,offset,length);
        }
    phrase * Next() const
        {
        return next;
        }
    word * Wording() const
        {
        return wording;
        }
    ptrdiff_t Offset() const
        {
        return offset;
        }
    size_t Length() const
        {
        return length;
        }
    void setCount(unsigned long count)
        {
        this->count = count;
        this->realCount = count;//20050616
        }
    unsigned long Count() const
        {
        return count;
        }
    unsigned long RealCount() const
        {
        return realCount;
        }
    void setWeightAsFrequency()
        {
        weight = count;
        if(next)
            next->setWeightAsFrequency();
        }
    void setWeightAsLength()
        {
        weight = length;
        if(next)
            next->setWeightAsLength();
        }
    void setWeightAsFrequencyTimesLength()
        {
        weight = length * count;
        if(next)
            next->setWeightAsFrequencyTimesLength();
        }
    void setWeightAsFrequencyTimesLengthTimesAverageOfInverseOfWordFrequency();
    void setWeightAsFrequencyTimesLengthTimesAllRatiosOfAverageWordFrequencyByRealWordFequency();
    void setWeightAsFrequencyTimesLengthTimesAverageOfEntropy();
    void setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLength();
//    void setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLengthTimesPhraseCountReduction();
    void setWeightAsFrequencyTimesLengthTimesAverageOfInverseWordFrequencyTimesPhraseCountReduction();
    void setWeight2005();
    void setWeight2005b();
    void print(FILE * fp);
//    bool printsimple(FILE * fp, unsigned long phraseno);
    bool printsimpleARG(bool morphemes,FILE * fp, unsigned long phraseno,bool b_phraseno,bool b_realCount,bool b_weight, bool b_getAccumulatedRepetitiveness,const char * A,const char * Z);
//#ifdef __BORLANDC__
    bool stat(unsigned long phraseno,leastSquareFitter * line);
//#endif
    double Weight() const
        {
        return weight;
        }
    unsigned long countPhrase(word * words, bool recount);
    unsigned long countPhraseInText(/*word * words, */bool recount,
            word * textFirst, word * nextTextFirst);
        // changes 'f' to 'B', 't' and 'e'
    void confirmPhraseInText(word * textFirst, word * nextTextFirst);
        // changes 'B' to 'b'
    unsigned long uncountPhraseInText(/*word * words,*/
            word * textFirst, word * nextTextFirst);
        // changes 'B' 't' 'e' to 'f'
    };

static type * typeArray = NULL;

static word * words1 = NULL;
static word ** pwordlist = NULL;

static phrase ** phrases = NULL;

static bool goodAllPhraseSize
    (
    word * phraseStart,
    size_t length,
    word * firstOfText,
    word * firstOfNextText
    )
    {
    return true;
    }

static bool isSentenceDelimiter(type * Type); // forward declaration

static long distanceToStartOfSentence(word * phraseStart,word * firstOfText,long maxToTest)
    {
    long dist = 0;
    while(  maxToTest >= 0
         && --phraseStart > firstOfText
         && !isSentenceDelimiter(phraseStart->tp)
         )
        {
        --maxToTest;
        ++dist;
        }
    return dist; // 0 <= return value <= maxToTest + 1
    }

static long distanceToStartOfNextSentence(word * afterPhraseEnd,word * firstOfNextText,long maxToTest)
    {
    long dist = 0;
    while(  maxToTest >= 0
         && afterPhraseEnd < firstOfNextText
         && !isSentenceDelimiter(afterPhraseEnd->tp)
         )
        {
        --maxToTest;
        ++dist;
        ++afterPhraseEnd;
        }
    return dist; // 0 <= return value <= maxToTest + 1
    }

static double MaxUncovered = 20;

static bool goodPhraseSize
    (
    word * phraseStart,
    size_t length,
    word * firstOfText,
    word * firstOfNextText
    )
    {
    long maxUncoveredLength = (long)(MaxUncovered * ((double)length + 0.5));
    maxUncoveredLength -= distanceToStartOfSentence(phraseStart,firstOfText,maxUncoveredLength);
    if(maxUncoveredLength >= 0)
        maxUncoveredLength -= distanceToStartOfNextSentence(phraseStart + length,firstOfNextText,maxUncoveredLength);
    return maxUncoveredLength >= 0;
    }

static bool goodSentenceSize
    (
    word * phraseStart,
    size_t length,
    word * firstOfText,
    word * firstOfNextText
    )
    {
    if(  phraseStart == firstOfText
      || (  phraseStart > firstOfText
         && isSentenceDelimiter((phraseStart-1)->tp)
         )
      )
        {
        word * FirstAfterPhrase = phraseStart + length;
        return FirstAfterPhrase == firstOfNextText
            || isSentenceDelimiter(FirstAfterPhrase->tp);
        }
    else
        return false;
    }

static bool (*goodSize)
    (
    word * phraseStart,
    size_t length,
    word * firstOfText,
    word * firstOfNextText
    ) = goodAllPhraseSize;


class type
    {
    char * typestring;
    bool is_word;
    word ** index;
    unsigned long frequency;
    phrase * phraseP;
public:
    type(): typestring(NULL),is_word(true),index(NULL),frequency(0L),phraseP(NULL)
        {
        }
    ~type()
        {
        delete [] typestring;
        delete phraseP;
        }
    void create(char * typestring,word ** index,unsigned long frequency)
        {
        setName(typestring);
        this->index = index;
        this->frequency = frequency;
        }
    void setName(const char * typestring)
        {
        if(this->typestring)
            delete [] this->typestring;
        if(typestring)
            {
            this->typestring = new char[strlen(typestring) + 1];
            strcpy(this->typestring,typestring);
            int kar;
            const char * s = typestring;
            while((kar = getUTF8char(s,UTF8)) != 0)
                {
                if(!isAlpha(kar))
                    {
                    is_word = false;
                    break;
                    }
                }
            }
        else
            {
            this->typestring = NULL;
            }
        }
    bool isWord() const
        {
        return is_word;
        }
    const char * name() const
        {
        return typestring ? typestring : "(NULL)";
        }
    unsigned long getFrequency() const
        {
        return frequency;
        }
    word * const * getIndex() const
        {
        return index;
        }
    void addPhrase  (word * wording
                    ,ptrdiff_t offset
                    ,size_t length
                    )
        {
        if(phraseP)
            phraseP->add(wording,offset,length);
        else
            phraseP = new phrase(wording,offset,length);
        }
    unsigned long countNumberOfPhrases()
        {
        unsigned long res = 0L;
        for ( phrase * Phrase = phraseP
            ; Phrase
            ; Phrase = Phrase->Next()
            )
            ++res;
        return res;
        }
    unsigned long countPhrases(word * words)
        {
        unsigned long allcount = 0L;
        for ( phrase * Phrase = phraseP
            ; Phrase
            ; Phrase = Phrase->Next()
            )
            {
            word * wording = Phrase->Wording();
            ptrdiff_t offset = Phrase->Offset();
            size_t length = Phrase->Length();
            unsigned long count = 0;
            for ( word * const * q = index
                ; q < index + frequency
                ; q++
                )
                {
                word * cand = *q - offset;
                if  (  cand >= words // candidates can not start before begin of text
                    && cand + length <= afterlastword
                                    // candidates can not end after end of text
                    && goodSize(cand,length,words,afterlastword )
                    )
                    {
                    word * r, * s;
                    for ( r = cand, s = wording
                        ; s < wording + length && r->tp == s->tp && s->tp != NULL
                        ; ++r,++s
                        )
                        ;
                    if(s == wording + length)
                        {
                        ++count;
                        }
                    }
                }
            Phrase->setCount(count);
            allcount += count;
            }
        return allcount;
        }
    size_t SumOfPhraseLengths() const
        {
        size_t sumOfPhraseLengths = 0L;
        for ( phrase * Phrase = phraseP
            ; Phrase
            ; Phrase = Phrase->Next()
            )
            {
            sumOfPhraseLengths += Phrase->Length();
            }
        return sumOfPhraseLengths;
        }
    size_t SumOfPhraseLengthsTimesFrequencies() const
        {
        size_t sumOfPhraseLengthsTimesFrequencies = 0L;
        for ( phrase * Phrase = phraseP
            ; Phrase
            ; Phrase = Phrase->Next()
            )
            {
            sumOfPhraseLengthsTimesFrequencies += Phrase->Length() * Phrase->Count();
            }
        return sumOfPhraseLengthsTimesFrequencies;
        }
    void setWeightAsFrequency()
        {
        if(phraseP)
            phraseP->setWeightAsFrequency();
        }
    void setWeightAsLength()
        {
        if(phraseP)
            phraseP->setWeightAsLength();
        }
    void setWeightAsFrequencyTimesLength()
        {
        if(phraseP)
            phraseP->setWeightAsFrequencyTimesLength();
        }
    void setWeightAsFrequencyTimesLengthTimesAverageOfInverseOfWordFrequency()
        {
        if(phraseP)
            phraseP->setWeightAsFrequencyTimesLengthTimesAverageOfInverseOfWordFrequency();
        }
    void setWeightAsFrequencyTimesLengthTimesAllRatiosOfAverageWordFrequencyByRealWordFequency()
        {
        if(phraseP)
            phraseP->setWeightAsFrequencyTimesLengthTimesAllRatiosOfAverageWordFrequencyByRealWordFequency();
        }
    void setWeightAsFrequencyTimesLengthTimesAverageOfEntropy()
        {
        if(phraseP)
            phraseP->setWeightAsFrequencyTimesLengthTimesAverageOfEntropy();
        }
    void setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLength()
        {
        if(phraseP)
            phraseP->setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLength();
        }
/*  void setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLengthTimesPhraseCountReduction()
        {
        if(phraseP)
            phraseP->setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLengthTimesPhraseCountReduction();
        }
*/
    void setWeightAsFrequencyTimesLengthTimesAverageOfInverseWordFrequencyTimesPhraseCountReduction()
        {
        if(phraseP)
            phraseP->setWeightAsFrequencyTimesLengthTimesAverageOfInverseWordFrequencyTimesPhraseCountReduction();
        }
    void setWeight2005()
        {
        if(phraseP)
            phraseP->setWeight2005();
        }
    void setWeight2005b()
        {
        if(phraseP)
            phraseP->setWeight2005b();
        }
    void print(FILE * fp)
        {
        fprintf(fp,"<%s>x%ld\n",typestring,frequency);
        for ( phrase * Phrase = phraseP
            ; Phrase
            ; Phrase = Phrase->Next()
            )
            Phrase->print(fp);
        }
    void setPhrasesInArray(phrase *** curPhrase)
        {
        for ( phrase * Phrase = phraseP
            ; Phrase
            ; Phrase = Phrase->Next()
            )
            {
            **curPhrase = Phrase;
            ++*curPhrase;
            }
        }
    };

charprop character[256];

static bool isSentenceDelimiter(type * Type)
    {
    unsigned char x = (unsigned char)Type->name()[0];
    return character[x] == sentence_delimiter;
//    return x == '.' || x == '?' || x == ';' || x == '!';
    }

static char * doubleslash(char * s)
    {
    static char buf[500];
    char *d = buf;
    while(*s)
        {
        if(*s == '\\')
            *d++ = '\\';
        *d++ = *s++;
        }
    *d = '\0';
    return buf;
    }

static long inpos = 0L;
#if 0
static char * writePhraseRTF(word * start,word * end)
    {
    static char markedfile[L_tmpnam];
    word * i;
    bool endofcode = false;
    if(!filedatalist)
        return NULL;
    tmpnam(markedfile);
    filedata * pfile = filedatalist;
    FILE * fpi = NULL;
    FILE * fpo = fopen(markedfile/*"marked.txt"*/,"wb");
// WordPad header:
/*
    fprintf(fpo,"{\\rtf1\\ansi\\deff0\\deftab720{\\fonttbl{\\f0\\fswiss MS ");
    fprintf(fpo,"Sans Serif;}{\\f1\\froman\\fcharset2 Symbol;}{\\f2\\fmodern ");
    fprintf(fpo,"Courier New;}}");
    fprintf(fpo,"{\\colortbl\\red0\\green0\\blue0;}");
    fprintf(fpo,"\\deflang1033\\pard\\plain\\f2\\fs20 ");
*/
    fprintf(fpo,"{\\rtf1\\ansi\\deff0\\deftab720{\\fonttbl{\\f0\\fswiss MS ");
    fprintf(fpo,"Sans Serif;}{\\f1\\froman\\fcharset2 Symbol;}{\\f2\\froman\\");
    fprintf(fpo,"fprq2 Times New Roman;}{\\f3\\froman Times New Roman;}}\n");
    fprintf(fpo,"{\\colortbl\\red0\\green0\\blue0;\\red255\\green0\\blue0;\\");
    fprintf(fpo,"red0\\green128\\blue0;\\red255\\green255\\blue0;\\red0\\");
    fprintf(fpo,"green0\\blue255;}\n");
    fprintf(fpo,"\\deflang1033\\pard\\plain\\f3\\fs20 ");
    int count = 0;
    if(!fpo)
        return NULL;
    while(pfile->boundary < start)
        ++pfile;
    for(i = start;i <= end;++i)
        {
        if(i == pfile->boundary)
            {
            fprintf(fpo,
    "\\par \\par \\plain\\f4\\fs20\\cf2 FILE %s \\plain\\f2\\fs20 \\par \\par ",
                    doubleslash(pfile->filename));
            if(fpi)
                fclose(fpi);
            fpi = fopen(pfile->filename,"rb");
            inpos = 0L;
            ++pfile;
            }
        assert(fpi != NULL);
        if(i->marked & _b_)
            {
            if(endofcode)
                {
                endofcode = false;
                fprintf(fpo," |");
                }
            while(inpos < i->fileStartPos)
                    {
                    int kar = fgetc(fpi);
                    inpos++;
                    if(kar == '\n')
                            count += fprintf(fpo,"\n\\par ");
                    else
                            {
                            ++count;
                            fputc(kar,fpo);
                            }
                    }
            fprintf(fpo,"{\\cf1 ");
            }
        else
            endofcode = false;
        if(i->tp)
            {
#ifdef UNNECESSARYSTUFF
            const char * s = words[i]->name();
            if(!strcmp(s,"\\a") && character[(unsigned int)'\a'] == control_character_to_be_kept)
                count += fprintf(fpo,"%c",'\a');
            else
            if(!strcmp(s,"\\b") && character[(unsigned int)'\b'] == control_character_to_be_kept)
                count += fprintf(fpo,"%c",'\b');
            else
            if(!strcmp(s,"\\f") && character[(unsigned int)'\f'] == control_character_to_be_kept)
                {
                fprintf(fpo,"%c",'\f');
                count = 0;
                }
            else
            if(!strcmp(s,"\\n") && character[(unsigned int)'\n'] == control_character_to_be_kept)
                {
                fprintf(fpo,"%c",'\\par ');
                count = 0;
                }
            else
            if(!strcmp(s,"\\r") && character[(unsigned int)'\r'] == control_character_to_be_kept)
                {
                fprintf(fpo,"%c",'\r');
                count = 0;
                }
            else
            if(!strcmp(s,"\\t") && character[(unsigned int)'\t'] == control_character_to_be_kept)
                {
                fprintf(fpo,"%c",'\t');
                count += 8;
                }
            else
            if(!strcmp(s,"\\v") && character[(unsigned int)'\v'] == control_character_to_be_kept)
                fprintf(fpo,"%c",'\v');
            else
            if(!strcmp(s,"\\\\") && character[(unsigned int)'\\'] == control_character_to_be_kept)
                count += fprintf(fpo,"%c",'\\\\');
            else
#endif
                {
                while(inpos < i->fileEndPos)
                        {
                        int kar = fgetc(fpi);
                        inpos++;
                        if(kar == '\n')
                                count += fprintf(fpo,"\n\\par ");
                        else
                                {
                                ++count;
                                fputc(kar,fpo);
                                }
                        }
                }
            }
        else
            count += fprintf(fpo,"\\plain\\f4\\fs20\\cf3 XXX\\plain\\f2\\fs20 ");
        if (i->marked & _e_)
            {
            fprintf(fpo,"}");
            endofcode = true;
            }
        if(count >= 72)
            {
            fprintf(fpo,"\n");
            count = 0;
            }
        }
    fprintf(fpo,"\\par }");
    fclose(fpo);
    return markedfile;
    }
#endif

void header(FILE * fpo,const char * title)
    {
    fprintf(fpo,
        "<?xml version=\"1.0\" encoding=\"%s\" ?>\n"
        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" "
        "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
        "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
        "<head>\n"
        "<title></title>\n"
        "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=%s\"/>\n"
        "<style type=\"text/css\">"
        "span { background-color: Silver; } </style>"
        "</head>\n"
        "<body>\n",
        UTF8 ? "UTF-8" : "ISO-8859-1",
        UTF8 ? "UTF-8" : "ISO-8859-1"
        );
    }


static char ** writePhraseHTML(word * start,word * end)
    {
    char ** ret = new char * [2];
    ret[1] = NULL;
    int nfiles = 1;
    static char markedfile[1000];
    word * i;
    bool endofcode = false;
    if(!filedatalist)
        return NULL;
    filedata * pfile = filedatalist;
    FILE * fpi = NULL;

    int count = 0;
    while(pfile->boundary < start)
        ++pfile;
    FILE * fpo = NULL;
    for(i = start;i <= end;++i)
        {
        if(i == pfile->boundary)
            {
            if(fpo)
                {
                ++nfiles;
                char ** nret = new char * [nfiles+1];
                for(int i = 0;i < nfiles;++i)
                    {
                    nret[i+1] = ret[i];
                    }
                delete [] ret;
                ret = nret;
                fprintf(fpo,"</p>");
                fprintf(fpo,"%s\n",
                    "</body>\n"
                    "</html>\n");
                fclose(fpo);
                }
            sprintf(markedfile,"%s.html",pfile->filename);
            ret[0] = new char[strlen(markedfile)+1];
            strcpy(ret[0],markedfile);
            fpo = fopen(markedfile/*"marked.txt"*/,"wb");
            if(!fpo)
                return NULL;
            header(fpo,pfile->filename);
            fprintf(fpo,"<h1>%s</h1><p>\n",doubleslash(pfile->filename));
            if(fpi)
                fclose(fpi);
            fpi = fopen(pfile->filename,"rb");
            inpos = 0L;
            ++pfile;
            }
        assert(fpi != NULL);
        if(i->marked & _b_)
            {
            if(endofcode)
                {
                endofcode = false;
                fprintf(fpo," |");
                }
            while(inpos < i->fileStartPos)
                {
                int kar = fgetc(fpi);
                inpos++;
                if(kar == '\n')
                    count += fprintf(fpo,"<br />\n");
                else
                    {
                    ++count;
                    fputc(kar,fpo);
                    }
                }
            fprintf(fpo,"<span>");
            }
        else
            {
            endofcode = false;
            }
        if(i->tp)
            {
            while(inpos < i->fileEndPos)
                {
                int kar = fgetc(fpi);
                inpos++;
                if(kar == '\n')
                    count += fprintf(fpo,"<br />\n");
                else
                    {
                    ++count;
                    fputc(kar,fpo);
                    }
                }
            }
        else
            count += fprintf(fpo,"<h2> XXX</h2>\n");
        if (i->marked & _e_)
            {
            fprintf(fpo,"</span>");
            endofcode = true;
            }
        }
    if(fpo)
        {
        fprintf(fpo,"</p>");
        fprintf(fpo,"%s\n",
            "</body>\n"
            "</html>\n");
        fclose(fpo);
        }
    return ret;
    }

static bool FindRepOfPhrase(word * startofphrase,word * endofphrase,/*bool & UniqueWord,*/ word *& PosOfUniqueWord)
    {
    word *  i, * lowi = NULL;
    unsigned long lowfreq = ULONG_MAX;
    for(i = startofphrase;i <= endofphrase;++i)
        {
        if(i->tp && lowfreq > i->tp->getFrequency())
            {
            lowfreq = i->tp->getFrequency();
            lowi = i;
            }
        }
    if(lowfreq == 1)
        {
        //UniqueWord = true;
        PosOfUniqueWord = lowi;
        return false; // no repetitions of this phrase exist.
        }
    ptrdiff_t offset = lowi - startofphrase;
    type * LeastFrequentType = lowi->tp;
    word * const * plong = LeastFrequentType->getIndex();
    bool found = false;
    for ( word * const * q = plong
        ; q < plong + lowfreq && !found
        ; q++
        )
        {
        word * cand = *q - offset;
        if  (  *q != lowi // phrase does not repeat with itself
            && cand >= words1 // candidates can not start before begin of text
                           // text starts at 1, not at 0, so if offset = 0
                           // then *q must be at least 1.
            && cand + (endofphrase - startofphrase) <= lastword
                            // candidates can not end after end of text
            )
            {
            word * r, * s;
            for ( r = cand, s = startofphrase
                ; s <= endofphrase && r->tp == s->tp && s->tp != NULL
                ; ++r,++s
                )
                ;
            if(s == endofphrase + 1)
                {
                found = true;
                LeastFrequentType->addPhrase(*q - offset
                    ,offset
                    ,(endofphrase + 1) - startofphrase
                    );
                }
            }
        }
    return found;
    }

static void FindRepsWithinSentence(word * startofsentence,word * endofsentence,bool startOK,bool endOK)
    {
    word *  start, * end, * PosOfUniqueWord = NULL;
    //bool UniqueWord = false;
    for( start = startofsentence
//       ;    start < endofsentence
       ;    start + minlimit - 1 <= endofsentence
         && PosOfUniqueWord == NULL //!UniqueWord
       ;
       )
        {
        if(start->tp
        && (  (start == startofsentence && startOK)
           || start->tp->isWord()
           )
          )
            {
            word * theEnd = endofsentence;
            if(maxlimit > 0 && theEnd - start > maxlimit - 1)
               theEnd = start + maxlimit - 1;
            for(end = theEnd // endofsentence
#ifdef ALLOWONEWORDPHRASES
               ;    end >= start + minlimit - 1   // Bart 20010515: Quick hack to include
                                   // phrases (terms) of only one word
#else
               ;    end > start
#endif
#ifdef ALLOWOVERLAP
#else
                 && !(  end->tp
                     && (end == endofsentence && endOK || end->tp->isWord())
                     && FindRepOfPhrase(start,end,/*UniqueWord,*/PosOfUniqueWord)
                     )
#endif
                 && PosOfUniqueWord == NULL //!UniqueWord
               ; --end
               )
#ifdef ALLOWOVERLAP
                {
                if(end->tp && (((end == endofsentence) && endOK) || end->tp->isWord()))
                    FindRepOfPhrase(start,end,/*UniqueWord,*/PosOfUniqueWord);
                }
#endif
#ifdef ALLOWOVERLAP
#else
            if(end > start) // found!
                {
                start = end + 1; // jump over words within repetition.
                }
            else
#endif
            if(PosOfUniqueWord == NULL /*!UniqueWord*/)
                {
                ++start;
//                ++unmatched; // no phrases starting at this word
                }
            }
        else
            {
            ++start;
//            ++unmatched; // no phrases starting at this word
            }
        }
//    if(start == endofsentence)
//        ++unmatched; // no phrases starting at the last (or only) word
    if(PosOfUniqueWord != NULL /*UniqueWord*/)
        {
//        ++unmatched; // the unique word
        if(PosOfUniqueWord > startofsentence)
            {
            FindRepsWithinSentence(startofsentence,PosOfUniqueWord - 1,startOK,false);
            }
        if(PosOfUniqueWord < endofsentence)
            FindRepsWithinSentence(PosOfUniqueWord + 1,endofsentence,false,endOK);
        }
    }

static void FindRepsAsSentence(word * startofsentence,word * endofsentence,bool start,bool end)
    {
//    bool success;
    if(  startofsentence->tp
//      && startofsentence->tp->isWord()
      && endofsentence > startofsentence
      && endofsentence->tp
//      && endofsentence->tp->isWord()
      )
        {
        word *  i, * lowi = NULL;
        unsigned long lowfreq = ULONG_MAX;
        for(i = startofsentence;i <= endofsentence;++i)
            {
            if(i->tp && lowfreq > i->tp->getFrequency())
                {
                lowfreq = i->tp->getFrequency();
                lowi = i;
                }
            }
        if(lowfreq == 1)
            ;//success = false; // no repetitions of this phrase exist.
        else
            {
            ptrdiff_t offset = lowi - startofsentence;
            type * LeastFrequentType = lowi->tp;
            word * const * plong = LeastFrequentType->getIndex();
            bool found = false;

            for ( word * const * q = plong
                ; q < plong + lowfreq && !found
                ; q++
                )
                {
                word * cand = *q - offset;
                if  (  *q != lowi // phrase does not repeat with itself
                    && cand >= words1 // candidates can not start before begin of text
                                   // text starts at 1, not at 0, so if offset = 0
                                   // then *q must be at least 1.
                    && cand + (endofsentence - startofsentence) <= lastword
                                    // candidates can not end after end of text
                    && goodSize(cand,(endofsentence - startofsentence) + 1,words1/*firstOfText*/,afterlastword /*firstOfNextText*/)
                    )
                    {
                    word * r, * s;
                    for ( r = cand, s = startofsentence
                        ; s <= endofsentence && r->tp == s->tp && s->tp != NULL
                        ; ++r,++s
                        )
                        ;
                    if(s == endofsentence + 1)
                        {
                        found = true;
                        LeastFrequentType->addPhrase(*q - offset
                            ,offset
                            ,(endofsentence + 1) - startofsentence
                            );
                        }
                    }
                }
//            success = found;
            }
        }
//    if(!success)
//        unmatched += endofsentence - startofsentence + 1;
    }

static void (*FindReps)(word * startofsentence,word * endofsentence,bool startOK,bool endOK) =
    FindRepsWithinSentence;


static unsigned long Repetitions()
    {
    if(typeArray && words1 && pwordlist)
        {
        word * wordindex;
        word * startofsentence = words1;
        unsigned long i;
//        int fileno = 0;
        filedata * pfile = filedatalist;
//        unmatched = 0L;
        numberOfPhrases = 0L;
        numberOfSentenceSeparators = 0L;
        pfile->numberOfSentenceSeparators = 0L;
        for(wordindex = words1;wordindex <= lastword;wordindex++)
            {
            type * Type;
            if((Type = wordindex->tp) != NULL)
				{
                if(isSentenceDelimiter(Type))
                    {
                    FindReps(startofsentence,wordindex - 1,true,true); // -1 because we do not include the separator
                    startofsentence = wordindex + 1;
                    ++numberOfSentenceSeparators;
                    ++(pfile->numberOfSentenceSeparators);
                    }
                else if(wordindex >= (pfile+1)->boundary)
                    {
                    (++pfile)->numberOfSentenceSeparators = 0L;
                    if(startofsentence < wordindex-1)
                        {
                        FindReps(startofsentence,wordindex-1,true,true);
                         // -1 because we do not include the first word of the new text
                        //++numberOfSentenceSeparators;
                        }
                    startofsentence = wordindex;// first word of the new text
                    }
				}
            }
        for ( i = 0
            ; i < types
            ; ++i
            )
            numberOfPhrases += typeArray[i].countNumberOfPhrases();
        if(phrases)
            delete [] phrases;
        phrases = new phrase * [numberOfPhrases];
        phrase ** curPhrase = phrases;
        for ( i = 0
            ; i < types
            ; ++i
            )
            typeArray[i].setPhrasesInArray(&curPhrase);
        return numberOfPhrases;
        }
    return 0L;
    }

static unsigned long CountRepetitions()
    {
    unsigned long result = 0L;
    unsigned long i;
    if(typeArray && words1 && pwordlist)
        {
        for ( i = 0
            ; i < types
            ; ++i
            )
            result += typeArray[i].countPhrases(words1);
        }
    return result;
    }

static void CountRealUnMatched()
    {
    word * i;
    filedata * pfile = filedatalist;
    realUnmatched = 0L;
    pfile->realUnmatched = 0;
    for ( i = words1
        ; i <= lastword
        ;
        )
        {
        if(i->marked == _f_)
            {
            ++realUnmatched;
            ++(pfile->realUnmatched);
            }
        ++i;
        if(i == (pfile+1)->boundary)
            {
            pfile->realUnmatched -= pfile->numberOfSentenceSeparators;
            (++pfile)->realUnmatched = 0;
            }
        }
    realUnmatched -= numberOfSentenceSeparators;
    pfile->realUnmatched -= pfile->numberOfSentenceSeparators;
    }

static unsigned long CountRepetitionsBetter()
    {
    unsigned long result = 0L;
    unsigned long i;
    for(i = 0;words1 + i <= lastword;++i)
        words1[i].marked = _f_;
    if(phrases && words1)
        {
        for ( i = 0
            ; i < numberOfPhrases
            ; ++i
            )
            result += phrases[i]->countPhrase(words1,true);
        }
    CountRealUnMatched();
    return result;
    }

static unsigned long CountRepetitionsInTextsBetter()
    {
    unsigned long result = 0L
#ifdef  NDEBUG
#else
		, saveresult
#endif		
		;// = 0L;
    unsigned long i;
    for(i = 0;words1 + i <= lastword;++i)
        words1[i].marked = _f_;
    if(phrases && words1)
        {
        for ( i = 0
            ; i < numberOfPhrases
            ; ++i
            )
            {
            int count;
            filedata * pfile = filedatalist;
#ifdef  NDEBUG
#else
            saveresult = result;
#endif
            for(
               ;    ( pfile->boundary != afterlastword /*0xffffffff*/ )
                 && (  ( count
                       = phrases[i]->countPhraseInText
                        (pfile == filedatalist,pfile->boundary,(pfile+1)->boundary)
                       )
                    != 0
                    )
               ;pfile++
               )
                {
                result += count;
                }
            if(pfile->boundary == afterlastword /*0xffffffff*/)
                for(;--pfile >= filedatalist;)
                    phrases[i]->confirmPhraseInText(pfile->boundary,(pfile+1)->boundary);
            else
                {
                for(;--pfile >= filedatalist;)
                    {
                    result -= phrases[i]->uncountPhraseInText
                       (pfile->boundary,(pfile+1)->boundary);
/*                    FILE * fp = fopen("\\LOG","a");
                    phrases[i]->print(fp);
                    fclose(fp);
*/
                    }
                assert(result == saveresult);// something wrong?
/*              if(result != saveresult)
                    result = saveresult;   // put breakpoint here, this is WRONG
*/
                }
            }
        }
    CountRealUnMatched();
    return result;
    }

static void MarkRepeatedPhrases() // Cosmetic addition: phrases that occur
                           // only once are not marked.
                           // Must be called after
                           // CountRepetitionsBetter(), where the real
                           // number of repetitions is computed.
    {
    unsigned long i;
    for(i = 0;words1 + i <= lastword;++i)
        words1[i].marked = _f_;
    if(phrases && words1)
        {
        for ( i = 0
            ; i < numberOfPhrases
            ; ++i
            )
            if(phrases[i]->RealCount() > 1)
                phrases[i]->countPhrase(words1,false);
        }
    CountRealUnMatched();
    }

//#ifdef __BORLANDC__
static void DrawGraph()
    {
    unsigned long phraseno = 1;
#ifdef __BORLANDC__
    clear();
#endif
    leastSquareFitter * line = new leastSquareFitter();
    unsigned long p;
    for ( p = 0
        ; p < numberOfPhrases
        ; ++p
        )
        {
        if(phrases[p]->stat(phraseno,line))
            {
            ++phraseno;
            }
        }
    if(numberOfPhrases > 1)
        {
        b = line->b();
#ifdef __BORLANDC__
        addXY(0.0,b,1);
#endif
        m = line->m();
#ifdef __BORLANDC__
        if(m != 0.0)
            addXY((2.0-b)/m,2.0,1);
        else
            addXY(1.0,2.0,1); // exceptional: e.g. all phrases have same count.
#endif
        }
    delete line;

    if(phrases)
        {
        size_t LengthOfWorsePhrases = 0L;
        for ( p = numberOfPhrases
            ; p != 0
            ;
            )
            {
            --p;
            size_t rc = phrases[p]->RealCount();
            if(rc > 1L)
                {
                phrases[p]->setLengthOfWorsePhrases(LengthOfWorsePhrases);
                LengthOfWorsePhrases += phrases[p]->Length()*rc;
                }
            }
        fiducialTextLength = LengthOfWorsePhrases + realUnmatched;
        size_t reducedTextLength = realUnmatched;
        for ( p = 0
            ; p < numberOfPhrases
            ; ++p
            )
            {
            size_t rc = phrases[p]->RealCount();
            if(rc > 1L)
                {
                reducedTextLength += phrases[p]->Length();
                //           LengthOfWorsePhrases = phrases[p]->getLengthOfWorsePhrases();
                phrases[p]->setAccumulatedRepetitiveness
                    (
                    (double)(afterlastword - words1 - numberOfSentenceSeparators)
                    /
                    (double)(reducedTextLength + phrases[p]->getLengthOfWorsePhrases())
                    );
                }
            }
        }
    }
//#endif

#ifdef TEST
static void WriteResults()
{
    if(typeArray && words1 && pwordlist)
        {
        FILE * fp = fopen("\\wordindex.txt","wb");
        if(fp)
            {
            unsigned long i;
            for ( i = 0
                ; i < types
                ; ++i
                )
                typeArray[i].print(fp);
            fclose(fp);
            }
        }
    if(phrases)
        {
        FILE * fp = fopen("\\phrases.txt","wb");
        if(fp)
            {
            for ( unsigned long p = 0
                ; p < numberOfPhrases
                ; ++p
                )
                {
                fprintf(fp,"%ld:",p);
                phrases[p]->print(fp);
                }
            fclose(fp);
            }
        }
}
#endif

static double RepetitivenessBetter()
{
    unsigned long i;
    if(phrases)
        {
        reducedTextLength = fiducialTextLength = realUnmatched;
        for ( i = 0
            ; i < numberOfPhrases
            ; ++i
            )
            {
            size_t rc = phrases[i]->RealCount();
            if(rc > 0L)
                {
                fiducialTextLength += phrases[i]->Length()*rc;
                reducedTextLength += phrases[i]->Length();
                }
            }
        }
    if(reducedTextLength > 0.0)
        return (double)fiducialTextLength /(double)reducedTextLength;
    else
        return 1.0;
}

static double ReductionDueToPreviousVersion(int fileno)
{
    filedata * pfile = filedatalist + fileno;
    ptrdiff_t alltokens = ((pfile + 1)->boundary - pfile->boundary) - pfile->numberOfSentenceSeparators;
    if(alltokens > 0)
        return ((double)(alltokens - pfile->realUnmatched))
                / ((double)alltokens);
    else
        return 0.0;
}

static void setWeightAsFrequency()
    {
    unsigned long i;
    for ( i = 0
        ; i < types
        ; ++i
        )
        {
        typeArray[i].setWeightAsFrequency();
        }
    }

static void setWeightAsLength()
    {
    unsigned long i;
    for ( i = 0
        ; i < types
        ; ++i
        )
        {
        typeArray[i].setWeightAsLength();
        }
    }

static void setWeightAsFrequencyTimesLength()
    {
    unsigned long i;
    for ( i = 0
        ; i < types
        ; ++i
        )
        {
        typeArray[i].setWeightAsFrequencyTimesLength();
        }
    }

static void setWeightAsFrequencyTimesLengthTimesAverageOfInverseOfWordFrequency()
    {
    unsigned long i;
    for ( i = 0
        ; i < types
        ; ++i
        )
        {
        typeArray[i].setWeightAsFrequencyTimesLengthTimesAverageOfInverseOfWordFrequency();
        }
    }

static void setWeightAsFrequencyTimesLengthTimesAllRatiosOfAverageWordFrequencyByRealWordFequency()
    {
    unsigned long i;
    for ( i = 0
        ; i < types
        ; ++i
        )
        {
        typeArray[i].setWeightAsFrequencyTimesLengthTimesAllRatiosOfAverageWordFrequencyByRealWordFequency();
        }
    }

static void setWeightAsFrequencyTimesLengthTimesAverageOfEntropy()
    {
    unsigned long i;
    for ( i = 0
        ; i < types
        ; ++i
        )
        {
        typeArray[i].setWeightAsFrequencyTimesLengthTimesAverageOfEntropy();
        }
    }

static void setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLength()
    {
    unsigned long i;
    for ( i = 0
        ; i < types
        ; ++i
        )
        {
        typeArray[i].setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLength();
        }
    }

/*static void setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLengthTimesPhraseCountReduction()
    {
    unsigned long i;
    for ( i = 0
        ; i < types
        ; ++i
        )
        {
        typeArray[i].setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLengthTimesPhraseCountReduction();
        }
    }
*/
static void setWeightAsFrequencyTimesLengthTimesAverageOfInverseWordFrequencyTimesPhraseCountReduction()
    {
    unsigned long i;
    for ( i = 0
        ; i < types
        ; ++i
        )
        {
        typeArray[i].setWeightAsFrequencyTimesLengthTimesAverageOfInverseWordFrequencyTimesPhraseCountReduction();
        }
    }

static void setWeight2005()
    {
    unsigned long i;
    logfac[0] = 0; // log 0!
    for(i = 1;i < sizeof(logfac)/sizeof(logfac[0]);++i)
        logfac[i] = logfac[i-1] + log(double(i));
    for ( i = 0
        ; i < types
        ; ++i
        )
        {
        typeArray[i].setWeight2005();
        }
    }

static void setWeight2005b()
    {
    unsigned long i;
    logfac[0] = 0; // log 0!
    for(i = 1;i < sizeof(logfac)/sizeof(logfac[0]);++i)
        logfac[i] = logfac[i-1] + log(double(i));
    for ( i = 0
        ; i < types
        ; ++i
        )
        {
        typeArray[i].setWeight2005b();
        }
    }

static void (*setWeight)(void) = //setWeightAsFrequencyTimesLengthTimesAverageOfInverseOfWordFrequency;
                            //setWeightAsFrequencyTimesLengthTimesAllRatiosOfAverageWordFrequencyByRealWordFequency;
                            setWeight2005;

bool weightIsFrequency()
    {
    return setWeight == setWeightAsFrequency;
    }

bool weightIsLength()
    {
    return setWeight == setWeightAsLength;
    }

bool weightIsFrequencyTimesLength()
    {
    return setWeight == setWeightAsFrequencyTimesLength;
    }

bool weightIsFrequencyTimesLengthTimesAverageOfInverseOfWordFrequency()
    {
    return setWeight == setWeightAsFrequencyTimesLengthTimesAverageOfInverseOfWordFrequency;
    }

bool weightIsFrequencyTimesLengthTimesAllRatiosOfAverageWordFrequencyByRealWordFequency()
    {
    return setWeight == setWeightAsFrequencyTimesLengthTimesAllRatiosOfAverageWordFrequencyByRealWordFequency;
    }

bool weightIsFrequencyTimesLengthTimesAverageOfEntropy()
    {
    return setWeight == setWeightAsFrequencyTimesLengthTimesAverageOfEntropy;
    }

bool weightIsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLength()
    {
    return setWeight == setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLength;
    }

/*bool weightIsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLengthTimesPhraseCountReduction()
    {
    return setWeight == setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLengthTimesPhraseCountReduction;
    }
*/
bool weightIsFrequencyTimesLengthTimesAverageOfInverseWordFrequencyTimesPhraseCountReduction()
    {
    return setWeight == setWeightAsFrequencyTimesLengthTimesAverageOfInverseWordFrequencyTimesPhraseCountReduction;
    }

bool weightIs2005()
    {
    return setWeight == setWeight2005;
    }

bool weightIs2005b()
    {
    return setWeight == setWeight2005b;
    }

void setRecursion(int npasses)
    {
    ::npasses = npasses;
    }

void setUnlimited(bool flag,int editMaxLimit)
    {
    if(flag)
        ::maxlimit = -1;
    else
        ::maxlimit = editMaxLimit;
    }

void setMaxLimit(int limit)
    {
    ::maxlimit = limit;
    }

void setMinLimit(int limit)
    {
    ::minlimit = limit;
    }

void chooseWeightAsFrequency()
    {
    setWeight = setWeightAsFrequency;
    }

void chooseWeightAsLength()
    {
    setWeight = setWeightAsLength;
    }

void chooseWeightAsFrequencyTimesLength()
    {
    setWeight = setWeightAsFrequencyTimesLength;
    }

void chooseWeightAsFrequencyTimesLengthTimesAverageOfInverseOfWordFrequency()
    {
    setWeight = setWeightAsFrequencyTimesLengthTimesAverageOfInverseOfWordFrequency;
    }

void chooseWeightAsFrequencyTimesLengthTimesAllRatiosOfAverageWordFrequencyByRealWordFequency()
    {
    setWeight = setWeightAsFrequencyTimesLengthTimesAllRatiosOfAverageWordFrequencyByRealWordFequency;
    }

void chooseWeightAsFrequencyTimesLengthTimesAverageOfEntropy()
    {
    setWeight = setWeightAsFrequencyTimesLengthTimesAverageOfEntropy;
    }

void chooseWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLength()
    {
    setWeight = setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLength;
    }

/*void chooseWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLengthTimesPhraseCountReduction()
    {
    setWeight = setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLengthTimesPhraseCountReduction;
    }
*/
void chooseWeightAsFrequencyTimesLengthTimesAverageOfInverseWordFrequencyTimesPhraseCountReduction()
    {
    setWeight = setWeightAsFrequencyTimesLengthTimesAverageOfInverseWordFrequencyTimesPhraseCountReduction;
    }

void chooseWeightAs2005()
    {
    setWeight = setWeight2005;
    }

void chooseWeightAs2005b()
    {
    setWeight = setWeight2005b;
    }

static int
#ifdef __BORLANDC__
    _USERENTRY
#endif
    phraseComp(const void *a, const void *b)
    {
    phrase ** A = (phrase **)a;
    phrase ** B = (phrase **)b;
    if((*B)->Weight() > (*A)->Weight())
        return 1;
    if ((*B)->Weight() < (*A)->Weight())
        return -1;
    return 0;
    }

static void SortPhrases()
    {
    if(phrases)
        {
        qsort(phrases,numberOfPhrases,sizeof(phrases[0]),phraseComp);
        if(versioncomparison)
            CountRepetitionsInTextsBetter();
        else
            CountRepetitionsBetter();
        }
    }

char ** WriteTextWithMarkings()
    {
    char ** markedfiles;
    //markedfile = writePhraseRTF(words1,lastword);
    markedfiles = writePhraseHTML(words1,lastword);
#ifdef TEST
    WriteResults();
#endif
    return markedfiles;
    }

#if 0
char * WritePhrases()
    {
    static char phrasesfile[L_tmpnam] = "";
    if(phrases)
        {
        tmpnam(phrasesfile);
        FILE * fp = fopen(phrasesfile/*"marked.txt"*/,"wb");
        if(fp)
            {
            fprintf(fp,"task: %s\n",versioncomparison ? "version comparison" : "repetitiveness checking");

            fprintf(fp,"case sensitive: %s\n",case_sensitive ? "yes" : "no");

            fprintf(fp,"fuzzy match level: ");
            switch(currentFuzzynessBoundary())
                {
                case 100:
                    fprintf(fp,"sentence");
                    break;
                case 95:
                    fprintf(fp,"95%%");
                    break;
                case 85:
                    fprintf(fp,"85%%");
                    break;
                case 75:
                    fprintf(fp,"75%%");
                    break;
                case 50:
                    fprintf(fp,"50%%");
                    break;
                case 0:
                    fprintf(fp,"no limit");
                    break;
                default:
                    fprintf(fp,"%d",currentFuzzynessBoundary());
                }
            fprintf(fp,"\n");
            if(maxlimit >= minlimit)
                {
                fprintf(fp,"words/phrase: %d - %d\n",minlimit,maxlimit);
                }
            else
                {
                fprintf(fp,"words/phrase: %d - unlimited\n",minlimit);
                }
            fprintf(fp,"passes: %d\n",npasses);

            if(setWeight == setWeightAsFrequency)
                fprintf(fp,"weight = phrase frequency\n\n");
            else if(setWeight == setWeightAsLength)
                fprintf(fp,"weight = phrase length\n\n");
            else if(setWeight == setWeightAsFrequencyTimesLength)
                fprintf(fp,"weight = phrase frequency * phrase length\n\n");
            else if(setWeight == setWeightAsFrequencyTimesLengthTimesAverageOfInverseOfWordFrequency)
                fprintf(fp,"weight = phrase frequency * phrase length * average of inverse of word frequency\n\n");
            else if(setWeight == setWeightAsFrequencyTimesLengthTimesAllRatiosOfAverageWordFrequencyByRealWordFequency)
                fprintf(fp,"weight = phrase frequency * phrase length * ratios (average word frequency / real word frequency)\n\n");
            else if(setWeight == setWeightAsFrequencyTimesLengthTimesAverageOfEntropy)
                fprintf(fp,"weight = phrase frequency * phrase length * average of word entropy\n\n");
            else if(setWeight == setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLength)
                fprintf(fp,"weight = phrase frequency * phrase length * average of word entropy * log(phrase length)\n\n");
/*            else if(setWeight == setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLengthTimesPhraseCountReduction)
                fprintf(fp,"weight = phrase frequency * phrase length * average of word entropy * log(phrase length) * Phrase count reduction factor\n\n");*/
            else if(setWeight == setWeightAsFrequencyTimesLengthTimesAverageOfInverseWordFrequencyTimesPhraseCountReduction)
                fprintf(fp,"weight = phrase frequency * phrase length * average of inverse of word frequency * Phrase count reduction factor\n\n");
            else if(setWeight == setWeight2005)
                fprintf(fp,"weight = 2005\n\n");
            else if(setWeight == setWeight2005b)
                fprintf(fp,"weight = 2005b\n\n");

            fprintf(fp,"repetitiveness = %f\n\n",repetitiveness);
            fprintf(fp,"phrase length = %f + %f * log(phrase #)\n\n",b,m);
            fprintf(fp,"   #  count weight   phrase\n");
            fprintf(fp,"---------------------------\n");
            unsigned long phraseno = 1;
            for ( unsigned long p = 0
                ; p < numberOfPhrases
                ; ++p
                )
                {
                if(phrases[p]->printsimple(fp,phraseno))
                    {
                    ++phraseno;
                    }
                }
            fclose(fp);
            }
        return phrasesfile;
        }
    return NULL;
    }
#endif

#if 0
void WritePhrasesArg(bool morphemes,FILE * fp,flagspreambule preambule,bool b_phraseno,bool b_realCount,bool b_weight, bool b_getAccumulatedRepetitiveness)
    {
    if(phrases && fp)
        {
        if(preambule.bools.b_task)
            fprintf(fp,"task: %s\n",versioncomparison ? "version comparison" : "repetitiveness checking");

        if(preambule.bools.b_case_sensitive)
            fprintf(fp,"case sensitive: %s\n",case_sensitive ? "yes" : "no");

        if(preambule.bools.b_fuzzy)
            {
            fprintf(fp,"fuzzy match level: ");
            switch(currentFuzzynessBoundary())
                {
                case 100:
                    fprintf(fp,"sentence");
                    break;
                case 95:
                    fprintf(fp,"95%%");
                    break;
                case 85:
                    fprintf(fp,"85%%");
                    break;
                case 75:
                    fprintf(fp,"75%%");
                    break;
                case 50:
                    fprintf(fp,"50%%");
                    break;
                case 0:
                    fprintf(fp,"no limit");
                    break;
                default:
                    fprintf(fp,"%d",currentFuzzynessBoundary());
                }
            fprintf(fp,"\n");
            }
        if(preambule.bools.b_limit)
            {
            if(maxlimit >= minlimit)
                {
                fprintf(fp,"words/phrase: %d - %d\n",minlimit,maxlimit);
                }
            else
                {
                fprintf(fp,"words/phrase: %d - unlimited\n",minlimit);
                }
            }
        if(preambule.bools.b_passes)
            fprintf(fp,"passes: %d\n",npasses);

        if(preambule.bools.b_weight)
            {
            if(setWeight == setWeightAsFrequency)
                fprintf(fp,"weight = phrase frequency\n\n");
            else if(setWeight == setWeightAsLength)
                fprintf(fp,"weight = phrase length\n\n");
            else if(setWeight == setWeightAsFrequencyTimesLength)
                fprintf(fp,"weight = phrase frequency * phrase length\n\n");
            else if(setWeight == setWeightAsFrequencyTimesLengthTimesAverageOfInverseOfWordFrequency)
                fprintf(fp,"weight = phrase frequency * phrase length * average of inverse of word frequency\n\n");
            else if(setWeight == setWeightAsFrequencyTimesLengthTimesAllRatiosOfAverageWordFrequencyByRealWordFequency)
                fprintf(fp,"weight = phrase frequency * phrase length * ratios (average word frequency / real word frequency)\n\n");
            else if(setWeight == setWeightAsFrequencyTimesLengthTimesAverageOfEntropy)
                fprintf(fp,"weight = phrase frequency * phrase length * average of word entropy\n\n");
            else if(setWeight == setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLength)
                fprintf(fp,"weight = phrase frequency * phrase length * average of word entropy * log(phrase length)\n\n");
    /*            else if(setWeight == setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLengthTimesPhraseCountReduction)
                fprintf(fp,"weight = phrase frequency * phrase length * average of word entropy * log(phrase length) * Phrase count reduction factor\n\n");*/
            else if(setWeight == setWeightAsFrequencyTimesLengthTimesAverageOfInverseWordFrequencyTimesPhraseCountReduction)
                fprintf(fp,"weight = phrase frequency * phrase length * average of inverse of word frequency * Phrase count reduction factor\n\n");
            else if(setWeight == setWeight2005)
                fprintf(fp,"weight = 2005\n\n");
            else if(setWeight == setWeight2005b)
                fprintf(fp,"weight = 2005b\n\n");
            }
        if(preambule.bools.b_repetitiveness)
            fprintf(fp,"repetitiveness = %f\n\n",repetitiveness);

        if(preambule.bools.b_formula)
            {
            fprintf(fp,"phrase length = %f + %f * log(phrase #)\n\n",b,m);
            }
        fprintf(fp,"<table><colgroup>");
        if(b_phraseno)
            fprintf(fp,"<col align="right" />");
        if(b_realCount)
            fprintf(fp,"<col align="right" />");
        if(b_weight)
            fprintf(fp,"<col align="char" char="." />");
        if(b_getAccumulatedRepetitiveness)
            fprintf(fp,"<col align="char" char="." />");
        fprintf(fp,"<td>Phrase</td>");
            fprintf(fp,"<col align="left" />");
        fprintf(fp,"</colgroup><thead><tr>");
        if(b_phraseno)
            fprintf(fp,"<td>#</td>");
        if(b_realCount)
            fprintf(fp,"<td>count</td>");
        if(b_weight)
            fprintf(fp,"<td>weight</td>");
        if(b_getAccumulatedRepetitiveness)
            fprintf(fp,"<td>Acc. repetitiveness</td>");
        fprintf(fp,"<td>Phrase</td>");
        fprintf(fp,"</tr></thead>\n");

        unsigned long phraseno = 1;
        for ( unsigned long p = 0
            ; p < numberOfPhrases
            ; ++p
            )
            {
            if(phrases[p]->printsimpleARG(morphemes,fp, phraseno,b_phraseno,b_realCount,b_weight, b_getAccumulatedRepetitiveness,"<tr>","</tr>\n"))
                {
                ++phraseno;
                }
            }
        }
    }
#endif

void WritePhrasesArgHTML(char ** names,double * versionalikeness,bool morphemes,FILE * fp,flagspreambule preambule,bool b_phraseno,bool b_realCount,bool b_weight, bool b_getAccumulatedRepetitiveness)
    {
    if(phrases && fp)
        {
        header(fp,"phrases");
        if(preambule.bools.b_task)
            {
            fprintf(fp,"<p>task: %s</p>\n",versioncomparison ? "version comparison" : "repetitiveness checking");
            }
        if(names[1])
            {
            fprintf(fp,"<p>alikeness:</p>\n");
            while(*names)
                {
                fprintf(fp,"<p>%s: %f</p>\n",*names,*versionalikeness);
                ++names;
                ++versionalikeness;
                }
            }

        if(preambule.bools.b_case_sensitive)
            {
            fprintf(fp,"<p>case sensitive: %s</p>\n",case_sensitive ? "yes" : "no");
            }

        if(preambule.bools.b_fuzzy)
            {
            fprintf(fp,"<p>fuzzy match level: ");
            switch(currentFuzzynessBoundary())
                {
                case 100:
                    fprintf(fp,"sentence");
                    break;
                case 95:
                    fprintf(fp,"95%%");
                    break;
                case 85:
                    fprintf(fp,"85%%");
                    break;
                case 75:
                    fprintf(fp,"75%%");
                    break;
                case 50:
                    fprintf(fp,"50%%");
                    break;
                case 0:
                    fprintf(fp,"no limit");
                    break;
                default:
                    fprintf(fp,"%d",currentFuzzynessBoundary());
                }
            fprintf(fp,"</p>\n");
            }
        if(preambule.bools.b_limit)
            {
            fprintf(fp,"<p>");
            if(maxlimit >= minlimit)
                {
                fprintf(fp,"words/phrase: %d - %d",minlimit,maxlimit);
                }
            else
                {
                fprintf(fp,"words/phrase: %d - unlimited",minlimit);
                }
            fprintf(fp,"</p>\n");
            }
        if(preambule.bools.b_passes)
            {
            fprintf(fp,"<p>passes: %d</p>\n",npasses);
            }
        if(preambule.bools.b_weight)
            {
            fprintf(fp,"<p>");
            if(setWeight == setWeightAsFrequency)
                fprintf(fp,"weight = phrase frequency");
            else if(setWeight == setWeightAsLength)
                fprintf(fp,"weight = phrase length");
            else if(setWeight == setWeightAsFrequencyTimesLength)
                fprintf(fp,"weight = phrase frequency * phrase length");
            else if(setWeight == setWeightAsFrequencyTimesLengthTimesAverageOfInverseOfWordFrequency)
                fprintf(fp,"weight = phrase frequency * phrase length * average of inverse of word frequency");
            else if(setWeight == setWeightAsFrequencyTimesLengthTimesAllRatiosOfAverageWordFrequencyByRealWordFequency)
                fprintf(fp,"weight = phrase frequency * phrase length * ratios (average word frequency / real word frequency)");
            else if(setWeight == setWeightAsFrequencyTimesLengthTimesAverageOfEntropy)
                fprintf(fp,"weight = phrase frequency * phrase length * average of word entropy");
            else if(setWeight == setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLength)
                fprintf(fp,"weight = phrase frequency * phrase length * average of word entropy * log(phrase length)");
    /*            else if(setWeight == setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLengthTimesPhraseCountReduction)
                fprintf(fp,"weight = phrase frequency * phrase length * average of word entropy * log(phrase length) * Phrase count reduction factor\n\n");*/
            else if(setWeight == setWeightAsFrequencyTimesLengthTimesAverageOfInverseWordFrequencyTimesPhraseCountReduction)
                fprintf(fp,"weight = phrase frequency * phrase length * average of inverse of word frequency * Phrase count reduction factor");
            else if(setWeight == setWeight2005)
                fprintf(fp,"weight = 2005");
            else if(setWeight == setWeight2005b)
                fprintf(fp,"weight = 2005b");
            fprintf(fp,"</p>\n");
            }
        if(preambule.bools.b_repetitiveness)
            {
            fprintf(fp,"<p>repetitiveness = %f</p>\n",repetitiveness);
            }

        if(preambule.bools.b_formula)
            {
            fprintf(fp,"<p>phrase length = %f + %f * log(phrase #)</p>\n",b,m);
            }

        fprintf(fp,"<table><colgroup>");
        if(b_phraseno)
            fprintf(fp,"<col align=\"right\" />");
        if(b_realCount)
            fprintf(fp,"<col align=\"right\" />");
        if(b_weight)
            //fprintf(fp,"<col align=\"char\" char=\".\" />");
            fprintf(fp,"<col align=\"right\" />");
        if(b_getAccumulatedRepetitiveness)
            //fprintf(fp,"<col align=\"char\" char=\".\" />");
            fprintf(fp,"<col align=\"right\" />");
        fprintf(fp,"<col align=\"left\" />");
        fprintf(fp,"</colgroup>\n<thead><tr>");
        if(b_phraseno)
            fprintf(fp,"<td>#</td>");
        if(b_realCount)
            fprintf(fp,"<td>count</td>");
        if(b_weight)
            fprintf(fp,"<td>weight</td>");
        if(b_getAccumulatedRepetitiveness)
            fprintf(fp,"<td>Acc. repetitiveness</td>");
        fprintf(fp,"</tr></thead><tbody>\n");

        unsigned long phraseno = 1;
        for ( unsigned long p = 0
            ; p < numberOfPhrases
            ; ++p
            )
            {
            if(phrases[p]->printsimpleARG(morphemes,fp, phraseno,b_phraseno,b_realCount,b_weight, b_getAccumulatedRepetitiveness,"<tr>","</tr>\n"))
                {
                ++phraseno;
                }
            }
        fprintf(fp,"</tbody></table>\n");
        fprintf(fp,"%s\n",
            "</body>\n"
            "</html>\n");
        }
    }

struct indexentry
    {
    char * wordpointer;
    bool lastOfType;
/*    ~indexentry()
        {
        lastOfType = false;
        wordpointer = NULL;
        }*/
    };

static int replacement_for_string_constituent[256];

#if DOUNICODE
#if UNICODE_CAPABLE
bool case_sensitive = false;
#else
bool case_sensitive = true;
#endif
#else
bool case_sensitive = false;
#endif
static unsigned long numberOfBytes = 0L;

static FILE *fpi = NULL;
static char * ptextBuffer = NULL;
static indexentry * theBigIndex = NULL;

static void karcopy(int kar)
    {
    *ptextBuffer++ = (char)kar;
    }

static void karicopy(int kar)
    {
#if UNICODE_CAPABLE
    *ptextBuffer++ = (char)kar;
#else
    if(kar >= 192 && kar < 224)
        *ptextBuffer++ = (char)(kar + 32);
    else
        *ptextBuffer++ = (char)tolower(kar);
#endif
    }

int
#ifdef __BORLANDC__
    _USERENTRY
#endif
    wordComp(const void *a, const void *b)
    {
    indexentry ** A = (indexentry **)a;
    indexentry ** B = (indexentry **)b;
#if UNICODE_CAPABLE    
    int cmp = strCaseCmp((*A)->wordpointer,(*B)->wordpointer);
#else
    int cmp = strcmp((*A)->wordpointer,(*B)->wordpointer);
#endif
    if(cmp == 0)
        {
        int AA = UTF8char((*A)->wordpointer,UTF8);
        int BB = UTF8char((*B)->wordpointer,UTF8);
        /* If a type can be written with upper case but also with lower case, 
        prefer the lower case version, just to please the eye: */
        if(isUpper(AA))
            {
            if(isLower(BB))
                return -1;
            }
        else if(isUpper(BB))
            {
            return 1;
            }

        if((*A) > (*B))
            return 1;
        return -1;
        }
    return cmp;
    }

static indexentry ** pindex;

void countTypes()
    {
    if(pindex[0])
        {
        char * prev = pindex[0]->wordpointer;
        pindex[0]->lastOfType = false;
        types = 1L;
        unsigned long frequency = 1L;
        lowestfreq = ULONG_MAX;
        highestfreq = 0L;
        unsigned long j;
        for ( j = 1
            ; j < tokens
            ; ++j
            )
            {
#if UNICODE_CAPABLE    
            if(strCaseCmp(prev,pindex[j]->wordpointer))
#else
            if(strcmp(prev,pindex[j]->wordpointer))
#endif
                {
                prev = pindex[j]->wordpointer;
                if(frequency < lowestfreq)
                    {
                    lowestfreq = frequency;
                    lowtype = types - 1;
                    }
                if(frequency > highestfreq)
                    {
                    highestfreq = frequency;
                    hightype = types - 1;
                    }
                ++types;
                pindex[j-1]->lastOfType = true;
                frequency = 1L;
                }
            else
                {
                ++frequency;
                pindex[j-1]->lastOfType = false;
                }
            }
        pindex[j-1]->lastOfType = true;
        }
    }

enum Inside {nowhere,instring,indelimiter,inignored,inatomic,insentencedelimiter};
static Inside inside = nowhere;
static void countbewerk(void)
    {
    bool eof = false, lf = true;
    do
        {
        int ikar = fgetc(fpi);
        if(ikar == EOF)
            {
            eof = true;
            ikar = '\0';
            }
        switch(character[ikar])
            {
            case string_constituent:
                {
                if(inside != instring)
                    {
                    lf = false;
                    inside = instring;
                    }
                ++numberOfBytes;
                break;
                }
            case string_constituent_to_be_replaced:
                {
                if(inside != instring)
                    {
                    lf = false;
                    inside = instring;
                    }
                if(replacement_for_string_constituent[ikar])
                    ++numberOfBytes;
                else
                    if (!lf)
                        {
                        ++tokens;
                        lf = true;
                        }
                break;
                }
            case token_delimiter:
                {
                if (!lf)
                    {
                    ++tokens;
                    lf = true;
                    }
                inside = indelimiter;
                break;
                }
            case sentence_delimiter:
                {
                if (!lf)
                    {
                    ++tokens;
                    lf = true;
                    }
                ++numberOfBytes;
                ++tokens;
                inside = insentencedelimiter;
                break;
                }
            case ignored_symbol:
                {
                inside = inignored;
                break;
                }
            case atomic_string:
                {
                if (!lf)
                    {
                    ++tokens;
                    lf = true;
                    }
                ++numberOfBytes;
                ++tokens;
                inside = inatomic;
                break;
                }
            default:
                ;
            }
        }
    while(!eof);
    }

static void newtoken(long end)
    {
    words1[tokens].fileEndPos = end;
    *ptextBuffer++ = '\0';
    ++tokens;
    theBigIndex[tokens].wordpointer = ptextBuffer;
    }

static void copybewerk(void (*uitvoer)(int kar))
    {
    bool eof = false,lf = true;
    do
        {
        int ikar = fgetc(fpi);
        if(ikar == EOF)
            {
            eof = true;
            ikar = '\0';
            }
        switch(character[ikar])
            {
            case string_constituent:
                {
                if(inside != instring)
                    {
                    lf = false;
                    inside = instring;
                    words1[tokens].fileStartPos = inpos;
                    }
                uitvoer(ikar);
                break;
                }
            case string_constituent_to_be_replaced:
                {
                if(inside != instring)
                    {
                    lf = false;
                    inside = instring;
                    words1[tokens].fileStartPos = inpos;
                    }
                if(replacement_for_string_constituent[ikar])
                    uitvoer(replacement_for_string_constituent[ikar]);
                else
                    if (!lf)
                        {
                        newtoken(inpos);
                        lf = true;
                        }
                break;
                }
            case token_delimiter:
                {
                if (!lf)
                    {
                    newtoken(inpos);
                    lf = true;
                    }
                inside = indelimiter;
                break;
                }
            case sentence_delimiter:
                {
                if (!lf)
                    {
                    newtoken(inpos);
                    lf = true;
                    }
                words1[tokens].fileStartPos = inpos;
                uitvoer(ikar);
                newtoken(inpos + 1L);
                inside = insentencedelimiter;
                break;
                }
            case ignored_symbol:
                {
                if(inside != inignored)
                    {
                    inside = inignored;
                    words1[tokens].fileEndPos = inpos;
                    }
                break;
                }
            case atomic_string:
                {
                if (!lf)
                    {
                    newtoken(inpos);
                    lf = true;
                    }
                words1[tokens].fileStartPos = inpos;
                uitvoer(ikar);
                newtoken(inpos + 1L);
                inside = inatomic;
                break;
                }
            default:
                ;
            }
        inpos++;
        }
    while(!eof);
    }

static void ReadTexts(const char ** sis)
    {
    const char ** psi;
    unsigned long i;
    unsigned long j;
    int nofiles;

    tokens = 0L;
    numberOfBytes = 0L;
    for(psi = sis,nofiles = 0;*psi;++psi,++nofiles)
        {
        fpi = fopen(*psi,"rb");
        if(fpi)
            {
            inpos = 0L;
            countbewerk();
            fclose(fpi);
            }
        }

    if(filedatalist)
        delete [] filedatalist;

    filedatalist = new filedata[nofiles+1];

    if(textBuffer)
        delete [] textBuffer;

    textBuffer = new char[numberOfBytes + tokens]; // (reserve one byte for indicating the end of a word)
    ptextBuffer = textBuffer;

    theBigIndex = new indexentry[tokens + 1]; // last entry is not used.
    theBigIndex[0].wordpointer = ptextBuffer;
    if(words1)
        delete [] words1;
    words1 = new word[tokens + 1]; // last element not used, but introduced to eliminate expensive "if"
    afterlastword = words1 + tokens;
    lastword = afterlastword - 1;
    gtokens = tokens;
    tokens = 0L;
    words1[0].fileStartPos = -1L;
    filedata * pfile = filedatalist;
    for(psi = sis;*psi;++psi,++pfile)
        {
        pfile->filename = new char[strlen(*psi)+1];
        strcpy(pfile->filename,*psi);
        fpi = fopen(*psi,"rb");
        if(fpi)
            {
            inpos = 0L;
            pfile->boundary = words1 + tokens;
            if(case_sensitive)
                copybewerk(karcopy);
            else
                copybewerk(karicopy);
            fclose(fpi);
            }
        else
            {
            fprintf(stderr,"Cannot open %s for reading\n",pfile->filename);
            exit(-13);
            }
        }
    pfile->filename = NULL;
    pfile->boundary = words1 + tokens;
    pindex = new indexentry * [tokens + 1]; // last entry is not used.
    pindex[tokens] = NULL;
    for ( j = 0
        ; j </*=*/ tokens
        ; ++j
        )
        pindex[j] = theBigIndex + j;
#ifdef TEST
    FILE *fpo;
    if((fpo = fopen("\\num.txt","wb")) != NULL)
        {
        for ( j = 0
            ; j < tokens
            ; ++j
            )
            fprintf(fpo,"%lu:%s\n",j,pindex[j]->wordpointer);
        fclose(fpo);
        }
#endif
    qsort(pindex,tokens,sizeof(pindex[0]),wordComp);
    countTypes();
    averageTypeFrequency = (double)tokens / (double)types;
#ifdef TEST
    if((fpo = fopen("\\sorted.txt","wb")) != NULL)
        {
        for ( j = 0
            ; j < tokens
            ; ++j
            )
            fprintf(fpo,"%lu:%d:%s\n",j,pindex[j]->lastOfType,pindex[j]->wordpointer);
        fclose(fpo);
        }
#endif
    if(typeArray)
        delete [] typeArray;
    typeArray = new type[types];
    if(pwordlist)
        delete [] pwordlist;
    pwordlist = new word * [tokens];
    for(i = 0;i < tokens;++i)
        {
        words1[i].marked = _f_;
        words1[i].tp = NULL;
        }

	assert(lastword == words1 + i - 1);
	assert(afterlastword == words1 + i);

    unsigned long frequency = 0L;
    types = 0L;
    for ( j = 0
        ; j < tokens
        ; ++j
        )
        {
        word * wordp = words1 + (pindex[j] - theBigIndex);
        pwordlist[j] = wordp;
        wordp->tp = typeArray + types;
		assert(wordp->tp);
        ++frequency;
        if(pindex[j]->lastOfType)
            {
            typeArray[types].create(pindex[j]->wordpointer,pwordlist + (j + 1) - frequency,frequency);
            frequency = 0L;
            ++types;
            }
        }
    delete [] theBigIndex;
    theBigIndex = NULL;

    delete [] pindex;
    pindex = NULL;
    }

static void escap_fill(void)
{
for (int ikar = 0; ikar < 256 ; ikar++)
    switch(ikar)
        {
        case '\a' :
        case '\b' :
        case '\f' :
        case '\n' :
        case '\r' :
        case '\t' :
        case '\v' :
        case '\\' :
            character[ikar] = ignored_symbol;
            break;
        }
}

static void atomic_string_fill(void)
{
for (int ikar = 0; ikar < 256 ; ikar++)
    if(  character[ikar] != ignored_symbol
      && character[ikar] != string_constituent
      && character[ikar] != string_constituent_to_be_replaced
      && character[ikar] != token_delimiter
      && character[ikar] != sentence_delimiter
#ifdef UNNECESSARYSTUFF
      && character[ikar] != control_character_to_be_kept
      && character[ikar] != character_only_occurring_at_start_of_string
      && character[ikar] != character_only_occurring_at_end_of_string
#endif
      )
        character[ikar] = atomic_string;
}


static void ignored_symbol_fill(void)
{
for (int ikar = 0; ikar < 256 ; ikar++)
    if (ikar < ' '
#ifdef UNNECESSARYSTUFF
    && character[ikar] != control_character_to_be_kept
#else
    && character[ikar] != atomic_string
#endif
    )
        character[ikar] = ignored_symbol;
}


#ifdef UNNECESSARYSTUFF
static void character_only_occurring_at_start_of_string_fill(void)
{
/*
character[(unsigned int)'(']=character_only_occurring_at_start_of_string;
character[(unsigned int)'[(unsigned int)']=character_only_occurring_at_start_of_string;
character[(unsigned int)'{']=character_only_occurring_at_start_of_string;
character[(unsigned int)'<']=character_only_occurring_at_start_of_string;
*/
}


static void character_only_occurring_at_end_of_string_fill(void)
{
/*
character[(unsigned int)')']=character_only_occurring_at_end_of_string;
character[(unsigned int)']']=character_only_occurring_at_end_of_string;
character[(unsigned int)'}']=character_only_occurring_at_end_of_string;
character[(unsigned int)'>']=character_only_occurring_at_end_of_string;
*/
}
#endif

static void repla_fill(void)
    {
    /*
    character[(unsigned int)' '] = string_constituent_to_be_replaced;
    replacement_for_string_constituent[(unsigned int)' '] = '\n';
    character[(unsigned int)'\n'] = string_constituent_to_be_replaced;
    replacement_for_string_constituent[(unsigned int)'\n'] = '\n';
    character[(unsigned int)'\t'] = string_constituent_to_be_replaced;
    replacement_for_string_constituent[(unsigned int)'\t'] = '\n';
    */
    }


static void string_constituent_fill(void)
{
for (int ikar = 0; ikar < 256 ; ikar++)
    if ((ikar >= '0' && ikar <= '9') ||
        (ikar >= 'A' && ikar <= 'Z') ||
        (ikar >= 'a' && ikar <= 'z') ||
#ifdef CODEPAGE
        (ikar >= 128 && ikar <= 154) ||
        (ikar >= 160 && ikar <= 165))
#else
#if DOUNICODE
        (ikar & 0x80))
#else
        (ikar >= 192))
#endif
#endif
        character[ikar] = string_constituent;
character[(unsigned int)'&'] = string_constituent;
character[(unsigned int)'\''] = string_constituent;
character[(unsigned int)'*'] = string_constituent;
character[(unsigned int)'+'] = string_constituent;
character[(unsigned int)'-'] = string_constituent;
character[(unsigned int)'/'] = string_constituent;
character[(unsigned int)'<'] = string_constituent;
character[(unsigned int)'>'] = string_constituent;
character[(unsigned int)'_'] = string_constituent;
character[(unsigned int)'\\'] = string_constituent;
}

static void token_delimiter_fill(void)
{
character[(unsigned int)'\0'] = token_delimiter;
character[(unsigned int)' '] = token_delimiter;
character[(unsigned int)'\n'] = token_delimiter;
character[(unsigned int)'\t'] = token_delimiter;
}

static void sentence_delimiter_fill(void)
{
character[(unsigned int)'.'] = sentence_delimiter;
character[(unsigned int)'!'] = sentence_delimiter;
character[(unsigned int)'?'] = sentence_delimiter;
character[(unsigned int)';'] = sentence_delimiter;
}



void fill_character_properties(void)
    {
    for(int i = 0;i < 256;i++)
        {
        character[i] = atomic_string;
//        replacement_for_string_constituent[i] = '\n';
        replacement_for_string_constituent[i] = '\0';
        }
    string_constituent_fill();
    escap_fill();
    ignored_symbol_fill();
    repla_fill();
#ifdef UNNECESSARYSTUFF
    character_only_occurring_at_start_of_string_fill();
    character_only_occurring_at_end_of_string_fill();
#endif
    atomic_string_fill();
    token_delimiter_fill();
    sentence_delimiter_fill();
    }

void ShiftToNextProp(int i)
    {
    character[i] = (charprop)((int)character[i] + 1);
    if(character[i] == lastcharprop)
        character[i] = (charprop)((int)firstcharprop + 1);
    }

void phrase::print(FILE * fp)
    {
    unsigned long i;
    fprintf(fp,"  [");
    for ( i = 0L
        ; i < length
        ; ++i
        )
        {
        fprintf(fp,"%s",wording[i].tp->name());
        if(i < length - 1)
            fprintf(fp," ");
        }
    fprintf(fp,"]x%ld > %ld length %ld offset %ld weight %f\n",count,realCount,length,offset,weight);
//    if(next)
//        next->print(fp);
    }
/*
bool phrase::printsimple(FILE * fp, unsigned long phraseno)
    {
    if(realCount > 1)
        {
        unsigned long i;
        fprintf(fp,"%4ld ",phraseno);
        fprintf(fp,"%4ld %9.6f %9.6f  ",realCount,weight,getAccumulatedRepetitiveness());
        for ( i = 0L
            ; i < length
            ; ++i
            )
            {
            fprintf(fp,"%s",wording[i].tp->name());
            if(i < length - 1)
                fprintf(fp," ");
            }
        fprintf(fp,"\n");
        return true;
        }
    return false;
    }
*/
bool phrase::printsimpleARG(bool morphemes,FILE * fp, unsigned long phraseno,bool b_phraseno,bool b_realCount,bool b_weight, bool b_getAccumulatedRepetitiveness,const char * A,const char * Z)
    {
    if(realCount > 1)
        {
        unsigned long i;
        if(A)
            fprintf(fp,"%s",A);
        if(b_phraseno)
            fprintf(fp,"<td>%4ld</td>",phraseno);
        if(b_realCount)
            fprintf(fp,"<td>%4ld</td>",realCount);
        if(b_weight)
            fprintf(fp,"<td>%9.6f</td>",weight);
        if(b_getAccumulatedRepetitiveness)
            fprintf(fp,"<td>%9.6f</td>",getAccumulatedRepetitiveness());
        fprintf(fp,"<td>");
        for ( i = 0L
            ; i < length
            ; ++i
            )
            {
            fprintf(fp,"%s",wording[i].tp->name());
            if(!morphemes && i < length - 1)
                fprintf(fp," ");
            }
        fprintf(fp,"</td>");
        if(Z)
            fprintf(fp,"%s",Z);
        else
            fprintf(fp,"\n");
        return true;
        }
    return false;
    }

//#ifdef __BORLANDC__
bool phrase::stat(unsigned long phraseno,leastSquareFitter * line)
    {
    if(realCount > 1
    && length > 2     // heuristic
    )
        {
        double x = log((double)phraseno);
        line->add(x,length);
#ifdef __BORLANDC__
        addXY(x,length,0);
#endif
        return true;
        }
    return false;
    }
//#endif

void phrase::setWeightAsFrequencyTimesLengthTimesAverageOfInverseOfWordFrequency()
    { // Phrase Frequency * SUM("wavelength"), where wavelength=average distance between occurrences of word=inverse of frequency
    double av = 0.0;
    unsigned long i;
    for ( i = 0
        ; i < length
        ; ++i
        )
        av += 1.0/wording[i].tp->getFrequency();
    weight = av * count;
    if(next)
        next->setWeightAsFrequencyTimesLengthTimesAverageOfInverseOfWordFrequency();
    }

void phrase::setWeightAsFrequencyTimesLengthTimesAllRatiosOfAverageWordFrequencyByRealWordFequency()
    { // Phrase Frequency * phrase length * PRODUCT("wavelength"/meanwavelength), where wavelength=average distance between occurrences of word=inverse of frequency
    double prod = 1.0;
    unsigned long i;
    for ( i = 0
        ; i < length
        ; ++i
        )
        {
        if(wording[i].tp->isWord())
            prod *= averageTypeFrequency/wording[i].tp->getFrequency();
        }
    weight = prod * count * length;
    if(next)
        next->setWeightAsFrequencyTimesLengthTimesAllRatiosOfAverageWordFrequencyByRealWordFequency();
    }

/*
static double entropy(word * wording,unsigned long length)
    {
    unsigned long i;
    double S = 0.0;
    for ( i = 0
        ; i < length
        ; ++i
        )
        {
        double freq = wording[i].tp->getFrequency();
        S += freq * log(freq);
        }
    return S;
    }
*/

void phrase::setWeightAsFrequencyTimesLengthTimesAverageOfEntropy()
    {
    double av = 0.0;
    unsigned long i;
    for ( i = 0
        ; i < length
        ; ++i
        )
        {
        double freq = wording[i].tp->getFrequency();
        double prob = 1.0/freq;
        av -= prob * log(prob);
        }
    weight = av * count;
    if(next)
        next->setWeightAsFrequencyTimesLengthTimesAverageOfEntropy();
    }

void phrase::setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLength()
    {
    double av = 0.0;
    unsigned long i;
    for ( i = 0
        ; i < length
        ; ++i
        )
        {
        double freq = wording[i].tp->getFrequency();
        double prob = 1.0/freq;
        av -= prob * log((double)prob);
        }
    weight = av * count * log((double)length);
    if(next)
        next->setWeightAsFrequencyTimesLengthTimesAverageOfEntropy();
    }
/*
void phrase::setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLengthTimesPhraseCountReduction()
    {
    double av = 0.0;
    unsigned long i;
    for ( i = 0
        ; i < length
        ; ++i
        )
        {
        double freq = wording[i].tp->getFrequency();
        double prob = 1.0/freq;
        av -= prob * log(prob);
        }
    weight = av * realCount * log(length); // count * (realCount/count) = realCount
    if(next)
        next->setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLengthTimesPhraseCountReduction();
    }
*/

void phrase::setWeightAsFrequencyTimesLengthTimesAverageOfInverseWordFrequencyTimesPhraseCountReduction()
    {
    double av = 0.0;
    unsigned long i;
    for ( i = 0
        ; i < length
        ; ++i
        )
        av += 1.0/wording[i].tp->getFrequency();
    weight = av * realCount; // count * (realCount/count) = realCount
    if(next)
        next->setWeightAsFrequencyTimesLengthTimesAverageOfInverseWordFrequencyTimesPhraseCountReduction();
    }

double LogFac(unsigned long realCount)
    {
    double sum;
    if(realCount < sizeof(logfac)/sizeof(logfac[0]))
        {
        sum = logfac[realCount];
        }
    else
        {
        sum = logfac[sizeof(logfac)/sizeof(logfac[0])-1];
        unsigned long i;
        for ( i = sizeof(logfac)/sizeof(logfac[0])
            ; i <= realCount
            ; ++i
            )
            sum += log(double(i));
        }
    return sum;
    }


#if 1
void phrase::setWeight2005()
    {
    double sum = 0.0;
    double Sum = 0.0;
    unsigned long i;
    unsigned long j;
/*
x  = type
 i

n = number of words in text
m = number of repetitions of phrase

x ... x  = a sequence of l types x ... x  ('phrase')
 1     l                          1     l
               l
p(x ...x ) = PROD p(x )
   1    l     i=1    i

 m
P (x ...x ) = Probability that phrase x ...x  occurs m times in a text with length n
 n  1    l                             1    l
                m
              PROD (n - jl + 1)
              j=1                            m                  n - ml
            = ----------------- (p(x ... x ))  (1 - p(x ... x ))
                     m!             1     l            1     l


       m                         m
  log P (x ... x ) = - log m! + SUM log(n-jl+1) + m log p(x ...x ) + (n - ml) log (1 - p(x ...x ))
       n  1     l               j=1                        1    l                         1    l

*/


/*
    -log m!
*/
    sum = -LogFac(realCount);

    Sum += sum;

/*
     m
    SUM log(n-jl+1)
    j=1
*/

    sum = 0.0;

    for ( j = 1
        ; j <= realCount
        ; ++j
        )
        {
        if(tokens - j*length + 1 > 0)
            {
            sum += log((double)(tokens - j*length + 1));
            }
        }

    Sum += sum;

/*
    m log p(x ... x )
             1     l
*/
    sum = 0.0;

    for ( i = 0
        ; i < length
        ; ++i
        )
        {
        if(wording[i].tp->getFrequency() > 0)
            {
            sum += log((double)(wording[i].tp->getFrequency()));
            }
        else
            sum = 0;
        }
    if(tokens > 0)
        {
        sum = realCount * (sum - length * log((double)tokens));
        }
    else
        sum = 0;

    Sum += sum;

/*
(n - ml) log (1 - p(x ...x ))
                     1    l

*/
    sum = 0.0;

    double prod = 1.0;
    for ( i = 0
        ; i < length
        ; ++i
        )
        {
        if(wording[i].tp->getFrequency() > 0)
            {
            prod *= (double)(wording[i].tp->getFrequency())/(double)tokens;
            }
        else
            {
            prod = 0.0;
            break;
            }
        }

    if(prod < 1.0)
        {
        sum += (tokens - realCount*length)*log(1.0 - prod);
        }
    else
        sum = 0;

    Sum += sum;

    weight = -Sum;
    if(next)
        next->setWeight2005();
    }
#else
// Same function, but with count instead of realCount.
void phrase::setWeight2005()
    {
    double sum = 0.0;
    double Sum = 0.0;
    unsigned long i;
    unsigned long j;
/*
x  = type
 i

n = number of words in text
m = number of repetitions of phrase

x ... x  = a sequence of l types x ... x  ('phrase')
 1     l                          1     l
               l
p(x ...x ) = PROD p(x )
   1    l     i=1    i

 m
P (x ...x ) = Probability that phrase x ...x  occurs m times in a text with length n
 n  1    l                             1    l
                m
              PROD (n - jl + 1)
              j=1                            m                  n - ml
            = ----------------- (p(x ... x ))  (1 - p(x ... x ))
                     m!             1     l            1     l


       m                         m
  log P (x ... x ) = - log m! + SUM log(n-jl+1) + m log p(x ...x ) + (n - ml) log (1 - p(x ...x ))
       n  1     l               j=1                        1    l                         1    l

*/


/*
    -log m!
*/
    if(count <= sizeof(logfac))
        sum = -logfac[count-1];
    else
        {
        sum = -logfac[sizeof(logfac)-1];
        for ( i = sizeof(logfac)
            ; i <= count
            ; ++i
            )
            sum -= log(double(i));
        }

    Sum += sum;
/*
     m
    SUM log(n-jl+1)
    j=1
*/

    sum = 0.0;

    for ( j = 1
        ; j <= count
        ; ++j
        )
        {
        if(tokens - j*length + 1 > 0)
            {
            sum += log(tokens - j*length + 1);
            }
        }

    Sum += sum;

/*
    m log p(x ... x )
             1     l
*/
    sum = 0.0;

    for ( i = 0
        ; i < length
        ; ++i
        )
        {
        if(wording[i].tp->getFrequency() > 0)
            {
            sum += log(wording[i].tp->getFrequency());
            }
        else
            sum = 0;
        }
    if(tokens > 0)
        {
        sum = count * (sum - length * log((double)tokens));
        }
    else
        sum = 0;

    Sum += sum;

/*
(n - ml) log (1 - p(x ...x ))
                     1    l

*/
    sum = 0.0;

    double prod = 1.0;
    for ( i = 0
        ; i < length
        ; ++i
        )
        {
        if(wording[i].tp->getFrequency() > 0)
            {
            prod *= (double)(wording[i].tp->getFrequency())/(double)tokens;
            }
        else
            {
            prod = 0.0;
            break;
            }
        }

    if(prod < 1.0)
        {
        sum += (tokens - count*length)*log(1.0 - prod);
        }
    else
        sum = 0;

    Sum += sum;

    weight = -Sum; // count * (realCount/count) = realCount
    if(next)
        next->setWeight2005();
    }
#endif

// This weight function decrements a word's frequency each time the word occurs
// in a sequence. The probability that a sequence occurs more often than any
// of its words thereby becomes zero. In the previous weight function the word
// probabilities are fixed and there is a non-zero probability that a sequence
// occurs more often than any of its words.
// There are some differences in the order of the phrases, but they are not that
// big.
void phrase::setWeight2005b()
    {
    double Sum = 0.0;
    unsigned long i;
    unsigned long j;

    double prod = 1.0;
    for(j = 1; j <= realCount;++j)
        {
        prod = 1.0;
        Sum += log((double)(tokens - j * length + 1));
        for ( i = 0
            ; i < length
            ; ++i
            )
            {
            if(wording[i].tp->getFrequency() > j - 1)
                {
                Sum += log((double)(wording[i].tp->getFrequency() - j + 1));
                if(wording[i].tp->getFrequency() > j)
                    {
                    prod *= (wording[i].tp->getFrequency() - j)/tokens;
                    }
                }
            }
        if(1.0 - prod > 0)
            Sum += (tokens - j * length) * log(1.0 - prod);
  //      else
    //        break;
        }

    Sum -= length * realCount * log((double)tokens);

    Sum -= LogFac(realCount);

    weight = -Sum;
    if(next)
        next->setWeight2005b();
    }


unsigned long phrase::countPhrase(word * words, bool recount)
    {
    word * firstMarked = NULL, * lastMarked = NULL;
    if(recount)
        realCount = 0;
    word * const * index = wording[offset].tp->getIndex();
    const unsigned long frequency = wording[offset].tp->getFrequency();
    for ( word * const * q = index
        ; q < index + frequency
        ; q++
        )
        {
        word * cand = *q - offset;
        if  (  cand >= words // candidates can not start before begin of text
            && cand + length <= afterlastword
                            // candidates can not end after end of text
            && goodSize(cand,length,words/*firstOfText*/,afterlastword /*firstOfNextText*/)
            )
            {
            word * r, * s;
            for ( r = cand, s = wording
                ;    s < wording + length
                  && r->marked == _f_
                  && r->tp == s->tp
                  && s->tp != NULL
                ; ++r,++s
                )
                ;
            if(s == wording+length)
                {
                firstMarked = cand;
                lastMarked = cand + length - 1;
                firstMarked->marked = _b_; // begin
                for ( r = firstMarked + 1
                    ; r < lastMarked
                    ; ++r
                    )
                    r->marked = _t_;
                lastMarked->marked |= _e_; // end
                if(recount)
                    ++realCount;
                }
            }
        }
    if(realCount == 1)
        {                       // one occurrence is NO repetition.
        for ( word * r = firstMarked
            ; r <= lastMarked
            ; ++r
            )
            r->marked = _f_;
        realCount = 0L;
        }
    return realCount;
    }

unsigned long phrase::countPhraseInText(/*word * words, */bool recount,
        word * textFirst, word * nextTextFirst)
    {
    word * firstMarked /*= NULL*/, * lastMarked/* = NULL*/;
    unsigned long lRealCount;
    if(recount)
        realCount = 0;
    lRealCount = 0;
    word * const * index = wording[offset].tp->getIndex();
    const unsigned long frequency = wording[offset].tp->getFrequency();
    for ( word * const * q = index
        ; q < index + frequency
        ; q++
        ) // traverse all occurrences of this phrase
        {
        word * cand = *q - offset;
        if  (  cand >= textFirst // candidates can not start before begin of text
            && cand + length <= nextTextFirst
                            // candidates can not end after end of text
            && goodSize(cand,length,textFirst,nextTextFirst)
            )
            {
            word * r, * s;
//            LOG("%ld---%ld",textFirst, nextTextFirst);
            for ( r = cand, s = wording
                ;    s < wording + length
                  && (/*LOG("%d %c %s",r,marked[r],words[r]->name())
                     ,*/r->marked == _f_
                     )
                  && r->tp == s->tp
                  && s->tp != NULL
                ; ++r,++s
                )
                ;
            if(s == wording + length)
                {
                firstMarked = cand;
                lastMarked = cand + length - 1;
//                LOG("MARK! firstMarked %d lastMarked %d",firstMarked,lastMarked);
                firstMarked->marked = _B_; // begin
                for ( r = firstMarked + 1
                    ; r < lastMarked
                    ; ++r
                    )
                    r->marked = _t_;
                lastMarked->marked |= _e_; // end
                ++realCount;
                ++lRealCount;
                }
/*            else
                LOG("NOmark");*/
            }
        }
//  LOG("realCount %d",realCount);
    return lRealCount;
    }

void phrase::confirmPhraseInText(word * textFirst, word * nextTextFirst)
    {
    word * firstMarked/* = NULL*/;
    word * const * index = wording[offset].tp->getIndex();
    const unsigned long frequency = wording[offset].tp->getFrequency();
    for ( word * const * q = index
        ; q < index + frequency
        ; q++
        ) // traverse all occurrences of this phrase
        {
        word * cand = *q - offset;
        if  (  cand >= textFirst // candidates can not start before begin of text
            && cand + length <= nextTextFirst
                            // candidates can not end after end of text
            )
            {
            firstMarked = cand;
            if(firstMarked->marked & _B_)
                {
                firstMarked->marked &= ~_B_;
                firstMarked->marked |= _b_;
                }
            }
        }
    }

unsigned long phrase::uncountPhraseInText(/*word * words,*/
        word * textFirst, word * nextTextFirst)
    {
    word * firstMarked /*= NULL*/, * lastMarked/* = NULL*/;
    unsigned long lRealCount;
    lRealCount = 0;
    word * const * index = wording[offset].tp->getIndex();
    const unsigned long frequency = wording[offset].tp->getFrequency();
    for ( word * const * q = index
        ; q < index + frequency
        ; q++
        )
        {
        word * cand = *q - offset;
        if  (  cand >= textFirst // candidates can not start before begin of text
            && cand + length <= nextTextFirst
                            // candidates can not end after end of text
            )
            {
            word * r, * s;
            firstMarked = cand;
            lastMarked = cand + length - 1;
//          LOG("firstMarked %d lastMarked %d",firstMarked,lastMarked);
            if(  firstMarked->marked & _B_
              && lastMarked->marked & _e_
              )
                {
//              LOG("Be");
                for ( r = firstMarked + 1, s = wording + 1
                    ;    r < lastMarked
                      && (r->marked & _t_)
                      && r->tp == s->tp
                      && s->tp != NULL
                    ; ++r,++s
                    )
                    ;
                }
            else
                {
//              LOG("%c%c",marked[firstMarked],marked[lastMarked]);
                r = NULL;
                }
//          LOG("r %d lastMarked %d",r,lastMarked);
            if(r >= lastMarked)
                {
                for ( r = firstMarked
                    ; r <= lastMarked
                    ; ++r
                    )
                    r->marked = _f_;
                --realCount;
                ++lRealCount;
                }
            }
        }
//  LOG("undo realCount %d",realCount);
    return lRealCount;
    }

static int FuzzynessBoundary = 0;

int currentFuzzynessBoundary()
    {
    return FuzzynessBoundary;
    }

void selectFuzzynessBoundary(int perc)
    {
    FuzzynessBoundary = perc;
    switch(perc)
        {
        case 100:
            FindReps = FindRepsAsSentence;
            goodSize = goodSentenceSize;
            break;
        case 0:
            FindReps = FindRepsWithinSentence;
            goodSize = goodAllPhraseSize;
            break;
        default:
            FindReps = FindRepsWithinSentence;
            goodSize = goodPhraseSize;
            MaxUncovered = (100.0/(double)perc) - 1.0;
            break;
        }
    }


unsigned long GetLowestfreq()
    {
    return lowestfreq;
    }

unsigned long GetHighestfreq()
    {
    return highestfreq;
    }

ptrdiff_t GetNumberOfTokens()
    {
    return (afterlastword - words1) - numberOfSentenceSeparators;
    }

ptrdiff_t GetNumberOfTokensFile(int fileno)
    {
    return filedatalist[fileno+1].boundary - filedatalist[fileno].boundary;
    }

unsigned long GetNumberOfSentenceSeparatorsFile(int fileno)
    {
    return filedatalist[fileno].numberOfSentenceSeparators;
    }

unsigned long GetRealUnmatchedFile(int fileno)
    {
    return filedatalist[fileno].realUnmatched;
    }

unsigned long GetNumberOfTypes()
    {
    return types;
    }

unsigned long GetLowestfreq(const char ** Type)
    {
    if(Type)
        *Type = typeArray[lowtype].name();
    return lowestfreq;
    }

unsigned long GetHighestfreq(const char ** Type)
    {
    if(Type)
        *Type = typeArray[hightype].name();
    return highestfreq;
    }

/*
static unsigned long GetNumberOfUnmatchedWords()
    {
    return unmatched;
    }
*/

size_t GetFiducialTextLength()
    {
    return fiducialTextLength;
    }

size_t GetReducedTextLength()
    {
    return reducedTextLength;
    }

unsigned long GetRealUnmatched()
    {
    return realUnmatched;
    }

double ComputeRepetitiveness(const char ** sis, double * versionalikeness,bool morphemes)
    {
    ReadTexts(sis);
    unsigned long i;
    if(morphemes)
        {
        FILE * fw;
        const char * names[] = {"words.txt",NULL};
        fw = fopen(names[0],"wb");
        for ( i = 0
            ; i < types
            ; ++i
            )
            {
            const char * tp = typeArray[i].name();
            int kar;
            fprintf(fw,"^ ");
            while((kar = getUTF8char(tp,UTF8)) != 0)
                {
                char let[7];
                int len = UnicodeToUtf8(kar,let,sizeof(let)-1);
                let[len] = 0;
                fprintf(fw,"%s ",let);
                }
            fprintf(fw,"$ .\n");
            /* Returns *s if s isn't UTF8 and increments s with 1.
            Otherwise returns character and shifts s to start of next character.
            Returns 0 if end of string reached. In that case, s becomes invalid
            (pointing past the zero).
            */
            }
        fclose(fw);
        ReadTexts(names);
        }
    else
        {
        for ( i = 0
            ; i < types
            ; ++i
            )
            {
            const char * tp = typeArray[i].name();
            while(UTF8 && getUTF8char(tp,UTF8) != 0)
                {
                ;
                }
            if(!UTF8)
                {
                printf("%s\n",typeArray[i].name());
                getchar();
                }
            }
        }
    Repetitions();
    CountRepetitions();
//    setWeightAsFrequencyTimesLengthTimesAverageOfInverseOfWordFrequency();
//    setWeightAsFrequencyTimesLengthTimesAverageOfEntropy();
//    if(setWeight == setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLengthTimesPhraseCountReduction)
    for(int i = 0; i < npasses; ++i)
        {
        setWeight();
        SortPhrases();
        }
#if 0
    if(setWeight == setWeightAsFrequencyTimesLengthTimesAverageOfInverseWordFrequencyTimesPhraseCountReduction)
        {
        setWeightAsFrequencyTimesLengthTimesAverageOfInverseOfWordFrequency();
        SortPhrases();
        for(int i = 0;i < 1;++i) // The weights change as a result of sorting and count reduction.
                            // One might repeat this step a few times, to attain a "stable" order.
            {
            setWeight();
            SortPhrases();
            }
        }
    else
        {
        setWeight();
        SortPhrases();
        }
#endif
//    WriteResults();
    double rep = RepetitivenessBetter();
    repetitiveness = rep;
    int fileno;
    if(versionalikeness)
        for(fileno = 0;sis[fileno] != NULL;fileno++)
            versionalikeness[fileno] = ReductionDueToPreviousVersion(fileno);
    MarkRepeatedPhrases();
//#ifdef __BORLANDC__
    DrawGraph();
//#endif
    WriteTextWithMarkings();
    return rep;
    }

#if !defined __BORLANDC__
// If you want a DOS app you can use this main() as a starter.
int main(int argc, char ** argv)
{
    if(argc < 2)
        {
        printf("%s -h for usage\n",argv[0]);
        exit(0);
        }
    optionStruct options;
    switch(options.readArgs(argc,argv))
        {
        case GoOn:
            break;
        case Leave:
            exit(0);
        case Error:
            exit(1);
        }

    fill_character_properties();
//    double versionalikeness = 0.0;
//    printf("%s : %f\n",argv[1],ComputeRepetitiveness(argv+1,&versionalikeness));
    if(argc - optind > 1)
        versioncomparison = true;
    npasses = 2;
    if(options.p)
        {
        if(!strcmp(options.p,"1"))
            npasses = 1;
        }
    if(options.w)
        {
        struct wrec
            {
            const char * n;
            const char * w;
            void (*f)();
            };
        wrec wrecs[]=
            {
                {"1","F",setWeightAsFrequency},
                {"2","L",setWeightAsLength},
                {"3","FL",setWeightAsFrequencyTimesLength},
                {"4","FL/wf",setWeightAsFrequencyTimesLengthTimesAverageOfInverseOfWordFrequency},
                {"5","FLR",setWeightAsFrequencyTimesLengthTimesAllRatiosOfAverageWordFrequencyByRealWordFequency},
                {"6","FLE",setWeightAsFrequencyTimesLengthTimesAverageOfEntropy},
                {"7","FLElogL",setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLength},
                //setWeightAsFrequencyTimesLengthTimesAverageOfEntropyTimesLogLengthTimesPhraseCountReduction();
                {"8","FL/wfP",setWeightAsFrequencyTimesLengthTimesAverageOfInverseWordFrequencyTimesPhraseCountReduction},
                {"9","2005",setWeight2005},
                {"10","2005b",setWeight2005b},
                {0,0,0}
            };
        for(int i = 0;wrecs[i].n;++i)
            {
            if(!strcmp(wrecs[i].n,options.w) || !strcmp(wrecs[i].w,options.w))
                {
                setWeight = wrecs[i].f;
                break;
                }
            }
        }

    int N = argc - optind;
    double * versionalikeness = new double[N];
    ComputeRepetitiveness((const char **)(argv+optind),versionalikeness,options.letters);
    //WriteResults();
    flagspreambule preambule;
    preambule.allflags = 0;
    preambule.bools.b_case_sensitive = true;
    preambule.bools.b_formula = true;
    preambule.bools.b_fuzzy = true;
    preambule.bools.b_limit = true;
    preambule.bools.b_passes = true;
    preambule.bools.b_repetitiveness = true;
    preambule.bools.b_task = true;
    preambule.bools.b_weight = true;
    //printf("\nPhrases in file:%s\n",WritePhrases());
    FILE * fout = stdout;
    if(options.o)
        {
        fout = fopen(options.o,"w");
        if(!fout)
            {
            printf("Cannot open %s for writing\n",options.o);
            exit(-1);
            }
        }
    
    //WritePhrasesArg(options.letters,fout,preambule,/*b_phraseno*/false,/*b_realCount*/true,/*b_weight*/false, /*b_getAccumulatedRepetitiveness*/false);
    WritePhrasesArgHTML(argv+optind,versionalikeness,options.letters,fout,preambule,/*b_phraseno*/false,/*b_realCount*/true,/*b_weight*/false, /*b_getAccumulatedRepetitiveness*/false);
    if(fout != stdout)
        fclose(fout);
    return 0;
}
#endif




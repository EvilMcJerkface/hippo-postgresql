/*
 *  Copyright (c) 2012 Daisuke Okanohara
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *   1. Redistributions of source code must retain the above Copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above Copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   3. Neither the name of the authors nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 */

#include <am/succinct/rsdic/EnumCoder.hpp>
#include <am/succinct/utils.hpp>


NS_IZENELIB_AM_BEGIN

namespace succinct
{
namespace rsdic
{

uint64_t EnumCoder::Encode(uint64_t val, size_t rank_sb)
{
    if (rank_sb == 0 || rank_sb == kBlockSize) return 0;

    if (rank_sb > 32)
    {
        val = ~val;
        rank_sb = kBlockSize - rank_sb;
    }

    uint64_t code = 0;
    for (size_t i = 0; i < kBlockSize; ++i)
    {
        if (val >> i & 1ULL)
        {
            code += kCombinationTable64_[rank_sb--][kBlockSize - i - 1];
        }
    }

    return code;
}

uint64_t EnumCoder::Decode(uint64_t code, size_t rank_sb)
{
    if (rank_sb == 0) return 0;
    if (rank_sb == kBlockSize) return ~0ULL;

    size_t orig_rank_sb = rank_sb;
    rank_sb = rank_sb > 32 ? kBlockSize - rank_sb : rank_sb;

    uint64_t val = 0;
    uint64_t zero_case_num;
    for (size_t i = 0; i < kBlockSize; ++i)
    {
        zero_case_num = kCombinationTable64_[rank_sb][kBlockSize - i - 1];
        if (code >= zero_case_num)
        {
            val |= 1ULL << i;
            code -= zero_case_num;
            if (--rank_sb == 0) break;
        }
    }

    return orig_rank_sb <= 32 ? val : ~val;
}

bool EnumCoder::GetBit(uint64_t code, size_t rank_sb, size_t pos)
{
    if (rank_sb == 0) return false;
    if (rank_sb == kBlockSize) return true;
    if (Len(rank_sb) == kBlockSize) return code >> pos & 1ULL;

    size_t orig_rank_sb = rank_sb;
    rank_sb = rank_sb > 32 ? kBlockSize - rank_sb : rank_sb;

    uint64_t zero_case_num;
    for (size_t i = 0; i < pos; ++i)
    {
        zero_case_num = kCombinationTable64_[rank_sb][kBlockSize - i - 1];
        if (code >= zero_case_num)
        {
            code -= zero_case_num;
            --rank_sb;
        }
    }

    return (orig_rank_sb > 32) ^ (code >= kCombinationTable64_[rank_sb][kBlockSize - pos - 1]);
}

bool EnumCoder::GetBit(uint64_t code, size_t rank_sb, size_t pos, size_t& rank)
{
    if (rank_sb == 0)
    {
        rank = 0;
        return false;
    }
    if (rank_sb == kBlockSize)
    {
        rank = pos;
        return true;
    }
    if (Len(rank_sb) == kBlockSize)
    {
        rank = SuccinctUtils::popcount(code & ((1ULL << pos) - 1));
        return code >> pos & 1ULL;
    }

    size_t orig_rank_sb = rank_sb;
    rank_sb = rank_sb > 32 ? kBlockSize - rank_sb : rank_sb;

    rank = rank_sb;
    uint64_t zero_case_num;
    for (size_t i = 0; i < pos; ++i)
    {
        zero_case_num = kCombinationTable64_[rank_sb][kBlockSize - i - 1];
        if (code >= zero_case_num)
        {
            code -= zero_case_num;
            --rank_sb;
        }
    }

    if (orig_rank_sb <= 32)
    {
        rank -= rank_sb;
        return code >= kCombinationTable64_[rank_sb][kBlockSize - pos - 1];
    }
    else
    {
        rank = pos - rank + rank_sb;
        return code < kCombinationTable64_[rank_sb][kBlockSize - pos - 1];
    }
}

size_t EnumCoder::Rank(uint64_t code, size_t rank_sb, size_t pos)
{
    if (rank_sb == 0) return 0;
    if (rank_sb == kBlockSize) return pos;
    if (Len(rank_sb) == kBlockSize) return SuccinctUtils::popcount(code & ((1ULL << pos) - 1));

    size_t orig_rank_sb = rank_sb;
    rank_sb = rank_sb > 32 ? kBlockSize - rank_sb : rank_sb;

    size_t cur_rank = rank_sb;
    uint64_t zero_case_num;
    for (size_t i = 0; i < pos; ++i)
    {
        zero_case_num = kCombinationTable64_[cur_rank][kBlockSize - i - 1];
        if (code >= zero_case_num)
        {
            code -= zero_case_num;
            --cur_rank;
        }
    }

    return orig_rank_sb <= 32 ? rank_sb - cur_rank : pos - rank_sb + cur_rank;
}

size_t EnumCoder::Select0Enum_(uint64_t code, size_t rank_sb, size_t num)
{
    rank_sb = kBlockSize - rank_sb;
    uint64_t zero_case_num;
    for (size_t offset = 0; offset < kBlockSize; ++offset)
    {
        zero_case_num = kCombinationTable64_[rank_sb][kBlockSize - offset - 1];
        if (code >= zero_case_num)
        {
            code -= zero_case_num;
            --rank_sb;
        }
        else
        {
            if (num == 0) return offset;
            --num;
        }
    }

    __assert(false);
    return 0;
}

size_t EnumCoder::Select1Enum_(uint64_t code, size_t rank_sb, size_t num)
{
    uint64_t zero_case_num;
    for (size_t offset = 0; offset < kBlockSize; ++offset)
    {
        zero_case_num = kCombinationTable64_[rank_sb][kBlockSize - offset - 1];
        if (code >= zero_case_num)
        {
            if (num == 0) return offset;
            --num;
            code -= zero_case_num;
            --rank_sb;
        }
    }

    __assert(false);
    return 0;
}

size_t EnumCoder::Select(uint64_t code, size_t rank_sb, size_t num, bool bit)
{
    __assert(num < rank_sb);

    if (rank_sb == kBlockSize) return num;
    if (Len(rank_sb) == kBlockSize) return SuccinctUtils::selectBlock(bit ? code : ~code, num);

    return rank_sb <= 32 ? Select1Enum_(code, rank_sb, num) : Select0Enum_(code, rank_sb, num);
}

const uint8_t EnumCoder::kEnumCodeLength_[65] =
{
    0,  6,  11, 16, 20, 23, 27, 30, 33, 35, 38, 40, 42, 44, 46, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 46, 44, 42, 40, 38, 35, 33, 30, 27, 23, 20, 16, 11, 6,
    0
};

const uint64_t EnumCoder::kCombinationTable64_[33][64] =
{
    { 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL, 1ULL },
    { 0ULL, 1ULL, 2ULL, 3ULL, 4ULL, 5ULL, 6ULL, 7ULL, 8ULL, 9ULL, 10ULL, 11ULL, 12ULL, 13ULL, 14ULL, 15ULL, 16ULL, 17ULL, 18ULL, 19ULL, 20ULL, 21ULL, 22ULL, 23ULL, 24ULL, 25ULL, 26ULL, 27ULL, 28ULL, 29ULL, 30ULL, 31ULL, 32ULL, 33ULL, 34ULL, 35ULL, 36ULL, 37ULL, 38ULL, 39ULL, 40ULL, 41ULL, 42ULL, 43ULL, 44ULL, 45ULL, 46ULL, 47ULL, 48ULL, 49ULL, 50ULL, 51ULL, 52ULL, 53ULL, 54ULL, 55ULL, 56ULL, 57ULL, 58ULL, 59ULL, 60ULL, 61ULL, 62ULL, 63ULL },
    { 0ULL, 0ULL, 1ULL, 3ULL, 6ULL, 10ULL, 15ULL, 21ULL, 28ULL, 36ULL, 45ULL, 55ULL, 66ULL, 78ULL, 91ULL, 105ULL, 120ULL, 136ULL, 153ULL, 171ULL, 190ULL, 210ULL, 231ULL, 253ULL, 276ULL, 300ULL, 325ULL, 351ULL, 378ULL, 406ULL, 435ULL, 465ULL, 496ULL, 528ULL, 561ULL, 595ULL, 630ULL, 666ULL, 703ULL, 741ULL, 780ULL, 820ULL, 861ULL, 903ULL, 946ULL, 990ULL, 1035ULL, 1081ULL, 1128ULL, 1176ULL, 1225ULL, 1275ULL, 1326ULL, 1378ULL, 1431ULL, 1485ULL, 1540ULL, 1596ULL, 1653ULL, 1711ULL, 1770ULL, 1830ULL, 1891ULL, 1953ULL },
    { 0ULL, 0ULL, 0ULL, 1ULL, 4ULL, 10ULL, 20ULL, 35ULL, 56ULL, 84ULL, 120ULL, 165ULL, 220ULL, 286ULL, 364ULL, 455ULL, 560ULL, 680ULL, 816ULL, 969ULL, 1140ULL, 1330ULL, 1540ULL, 1771ULL, 2024ULL, 2300ULL, 2600ULL, 2925ULL, 3276ULL, 3654ULL, 4060ULL, 4495ULL, 4960ULL, 5456ULL, 5984ULL, 6545ULL, 7140ULL, 7770ULL, 8436ULL, 9139ULL, 9880ULL, 10660ULL, 11480ULL, 12341ULL, 13244ULL, 14190ULL, 15180ULL, 16215ULL, 17296ULL, 18424ULL, 19600ULL, 20825ULL, 22100ULL, 23426ULL, 24804ULL, 26235ULL, 27720ULL, 29260ULL, 30856ULL, 32509ULL, 34220ULL, 35990ULL, 37820ULL, 39711ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 5ULL, 15ULL, 35ULL, 70ULL, 126ULL, 210ULL, 330ULL, 495ULL, 715ULL, 1001ULL, 1365ULL, 1820ULL, 2380ULL, 3060ULL, 3876ULL, 4845ULL, 5985ULL, 7315ULL, 8855ULL, 10626ULL, 12650ULL, 14950ULL, 17550ULL, 20475ULL, 23751ULL, 27405ULL, 31465ULL, 35960ULL, 40920ULL, 46376ULL, 52360ULL, 58905ULL, 66045ULL, 73815ULL, 82251ULL, 91390ULL, 101270ULL, 111930ULL, 123410ULL, 135751ULL, 148995ULL, 163185ULL, 178365ULL, 194580ULL, 211876ULL, 230300ULL, 249900ULL, 270725ULL, 292825ULL, 316251ULL, 341055ULL, 367290ULL, 395010ULL, 424270ULL, 455126ULL, 487635ULL, 521855ULL, 557845ULL, 595665ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 6ULL, 21ULL, 56ULL, 126ULL, 252ULL, 462ULL, 792ULL, 1287ULL, 2002ULL, 3003ULL, 4368ULL, 6188ULL, 8568ULL, 11628ULL, 15504ULL, 20349ULL, 26334ULL, 33649ULL, 42504ULL, 53130ULL, 65780ULL, 80730ULL, 98280ULL, 118755ULL, 142506ULL, 169911ULL, 201376ULL, 237336ULL, 278256ULL, 324632ULL, 376992ULL, 435897ULL, 501942ULL, 575757ULL, 658008ULL, 749398ULL, 850668ULL, 962598ULL, 1086008ULL, 1221759ULL, 1370754ULL, 1533939ULL, 1712304ULL, 1906884ULL, 2118760ULL, 2349060ULL, 2598960ULL, 2869685ULL, 3162510ULL, 3478761ULL, 3819816ULL, 4187106ULL, 4582116ULL, 5006386ULL, 5461512ULL, 5949147ULL, 6471002ULL, 7028847ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 7ULL, 28ULL, 84ULL, 210ULL, 462ULL, 924ULL, 1716ULL, 3003ULL, 5005ULL, 8008ULL, 12376ULL, 18564ULL, 27132ULL, 38760ULL, 54264ULL, 74613ULL, 100947ULL, 134596ULL, 177100ULL, 230230ULL, 296010ULL, 376740ULL, 475020ULL, 593775ULL, 736281ULL, 906192ULL, 1107568ULL, 1344904ULL, 1623160ULL, 1947792ULL, 2324784ULL, 2760681ULL, 3262623ULL, 3838380ULL, 4496388ULL, 5245786ULL, 6096454ULL, 7059052ULL, 8145060ULL, 9366819ULL, 10737573ULL, 12271512ULL, 13983816ULL, 15890700ULL, 18009460ULL, 20358520ULL, 22957480ULL, 25827165ULL, 28989675ULL, 32468436ULL, 36288252ULL, 40475358ULL, 45057474ULL, 50063860ULL, 55525372ULL, 61474519ULL, 67945521ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 8ULL, 36ULL, 120ULL, 330ULL, 792ULL, 1716ULL, 3432ULL, 6435ULL, 11440ULL, 19448ULL, 31824ULL, 50388ULL, 77520ULL, 116280ULL, 170544ULL, 245157ULL, 346104ULL, 480700ULL, 657800ULL, 888030ULL, 1184040ULL, 1560780ULL, 2035800ULL, 2629575ULL, 3365856ULL, 4272048ULL, 5379616ULL, 6724520ULL, 8347680ULL, 10295472ULL, 12620256ULL, 15380937ULL, 18643560ULL, 22481940ULL, 26978328ULL, 32224114ULL, 38320568ULL, 45379620ULL, 53524680ULL, 62891499ULL, 73629072ULL, 85900584ULL, 99884400ULL, 115775100ULL, 133784560ULL, 154143080ULL, 177100560ULL, 202927725ULL, 231917400ULL, 264385836ULL, 300674088ULL, 341149446ULL, 386206920ULL, 436270780ULL, 491796152ULL, 553270671ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 9ULL, 45ULL, 165ULL, 495ULL, 1287ULL, 3003ULL, 6435ULL, 12870ULL, 24310ULL, 43758ULL, 75582ULL, 125970ULL, 203490ULL, 319770ULL, 490314ULL, 735471ULL, 1081575ULL, 1562275ULL, 2220075ULL, 3108105ULL, 4292145ULL, 5852925ULL, 7888725ULL, 10518300ULL, 13884156ULL, 18156204ULL, 23535820ULL, 30260340ULL, 38608020ULL, 48903492ULL, 61523748ULL, 76904685ULL, 95548245ULL, 118030185ULL, 145008513ULL, 177232627ULL, 215553195ULL, 260932815ULL, 314457495ULL, 377348994ULL, 450978066ULL, 536878650ULL, 636763050ULL, 752538150ULL, 886322710ULL, 1040465790ULL, 1217566350ULL, 1420494075ULL, 1652411475ULL, 1916797311ULL, 2217471399ULL, 2558620845ULL, 2944827765ULL, 3381098545ULL, 3872894697ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 10ULL, 55ULL, 220ULL, 715ULL, 2002ULL, 5005ULL, 11440ULL, 24310ULL, 48620ULL, 92378ULL, 167960ULL, 293930ULL, 497420ULL, 817190ULL, 1307504ULL, 2042975ULL, 3124550ULL, 4686825ULL, 6906900ULL, 10015005ULL, 14307150ULL, 20160075ULL, 28048800ULL, 38567100ULL, 52451256ULL, 70607460ULL, 94143280ULL, 124403620ULL, 163011640ULL, 211915132ULL, 273438880ULL, 350343565ULL, 445891810ULL, 563921995ULL, 708930508ULL, 886163135ULL, 1101716330ULL, 1362649145ULL, 1677106640ULL, 2054455634ULL, 2505433700ULL, 3042312350ULL, 3679075400ULL, 4431613550ULL, 5317936260ULL, 6358402050ULL, 7575968400ULL, 8996462475ULL, 10648873950ULL, 12565671261ULL, 14783142660ULL, 17341763505ULL, 20286591270ULL, 23667689815ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 11ULL, 66ULL, 286ULL, 1001ULL, 3003ULL, 8008ULL, 19448ULL, 43758ULL, 92378ULL, 184756ULL, 352716ULL, 646646ULL, 1144066ULL, 1961256ULL, 3268760ULL, 5311735ULL, 8436285ULL, 13123110ULL, 20030010ULL, 30045015ULL, 44352165ULL, 64512240ULL, 92561040ULL, 131128140ULL, 183579396ULL, 254186856ULL, 348330136ULL, 472733756ULL, 635745396ULL, 847660528ULL, 1121099408ULL, 1471442973ULL, 1917334783ULL, 2481256778ULL, 3190187286ULL, 4076350421ULL, 5178066751ULL, 6540715896ULL, 8217822536ULL, 10272278170ULL, 12777711870ULL, 15820024220ULL, 19499099620ULL, 23930713170ULL, 29248649430ULL, 35607051480ULL, 43183019880ULL, 52179482355ULL, 62828356305ULL, 75394027566ULL, 90177170226ULL, 107518933731ULL, 127805525001ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 12ULL, 78ULL, 364ULL, 1365ULL, 4368ULL, 12376ULL, 31824ULL, 75582ULL, 167960ULL, 352716ULL, 705432ULL, 1352078ULL, 2496144ULL, 4457400ULL, 7726160ULL, 13037895ULL, 21474180ULL, 34597290ULL, 54627300ULL, 84672315ULL, 129024480ULL, 193536720ULL, 286097760ULL, 417225900ULL, 600805296ULL, 854992152ULL, 1203322288ULL, 1676056044ULL, 2311801440ULL, 3159461968ULL, 4280561376ULL, 5752004349ULL, 7669339132ULL, 10150595910ULL, 13340783196ULL, 17417133617ULL, 22595200368ULL, 29135916264ULL, 37353738800ULL, 47626016970ULL, 60403728840ULL, 76223753060ULL, 95722852680ULL, 119653565850ULL, 148902215280ULL, 184509266760ULL, 227692286640ULL, 279871768995ULL, 342700125300ULL, 418094152866ULL, 508271323092ULL, 615790256823ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 13ULL, 91ULL, 455ULL, 1820ULL, 6188ULL, 18564ULL, 50388ULL, 125970ULL, 293930ULL, 646646ULL, 1352078ULL, 2704156ULL, 5200300ULL, 9657700ULL, 17383860ULL, 30421755ULL, 51895935ULL, 86493225ULL, 141120525ULL, 225792840ULL, 354817320ULL, 548354040ULL, 834451800ULL, 1251677700ULL, 1852482996ULL, 2707475148ULL, 3910797436ULL, 5586853480ULL, 7898654920ULL, 11058116888ULL, 15338678264ULL, 21090682613ULL, 28760021745ULL, 38910617655ULL, 52251400851ULL, 69668534468ULL, 92263734836ULL, 121399651100ULL, 158753389900ULL, 206379406870ULL, 266783135710ULL, 343006888770ULL, 438729741450ULL, 558383307300ULL, 707285522580ULL, 891794789340ULL, 1119487075980ULL, 1399358844975ULL, 1742058970275ULL, 2160153123141ULL, 2668424446233ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 14ULL, 105ULL, 560ULL, 2380ULL, 8568ULL, 27132ULL, 77520ULL, 203490ULL, 497420ULL, 1144066ULL, 2496144ULL, 5200300ULL, 10400600ULL, 20058300ULL, 37442160ULL, 67863915ULL, 119759850ULL, 206253075ULL, 347373600ULL, 573166440ULL, 927983760ULL, 1476337800ULL, 2310789600ULL, 3562467300ULL, 5414950296ULL, 8122425444ULL, 12033222880ULL, 17620076360ULL, 25518731280ULL, 36576848168ULL, 51915526432ULL, 73006209045ULL, 101766230790ULL, 140676848445ULL, 192928249296ULL, 262596783764ULL, 354860518600ULL, 476260169700ULL, 635013559600ULL, 841392966470ULL, 1108176102180ULL, 1451182990950ULL, 1889912732400ULL, 2448296039700ULL, 3155581562280ULL, 4047376351620ULL, 5166863427600ULL, 6566222272575ULL, 8308281242850ULL, 10468434365991ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 15ULL, 120ULL, 680ULL, 3060ULL, 11628ULL, 38760ULL, 116280ULL, 319770ULL, 817190ULL, 1961256ULL, 4457400ULL, 9657700ULL, 20058300ULL, 40116600ULL, 77558760ULL, 145422675ULL, 265182525ULL, 471435600ULL, 818809200ULL, 1391975640ULL, 2319959400ULL, 3796297200ULL, 6107086800ULL, 9669554100ULL, 15084504396ULL, 23206929840ULL, 35240152720ULL, 52860229080ULL, 78378960360ULL, 114955808528ULL, 166871334960ULL, 239877544005ULL, 341643774795ULL, 482320623240ULL, 675248872536ULL, 937845656300ULL, 1292706174900ULL, 1768966344600ULL, 2403979904200ULL, 3245372870670ULL, 4353548972850ULL, 5804731963800ULL, 7694644696200ULL, 10142940735900ULL, 13298522298180ULL, 17345898649800ULL, 22512762077400ULL, 29078984349975ULL, 37387265592825ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 16ULL, 136ULL, 816ULL, 3876ULL, 15504ULL, 54264ULL, 170544ULL, 490314ULL, 1307504ULL, 3268760ULL, 7726160ULL, 17383860ULL, 37442160ULL, 77558760ULL, 155117520ULL, 300540195ULL, 565722720ULL, 1037158320ULL, 1855967520ULL, 3247943160ULL, 5567902560ULL, 9364199760ULL, 15471286560ULL, 25140840660ULL, 40225345056ULL, 63432274896ULL, 98672427616ULL, 151532656696ULL, 229911617056ULL, 344867425584ULL, 511738760544ULL, 751616304549ULL, 1093260079344ULL, 1575580702584ULL, 2250829575120ULL, 3188675231420ULL, 4481381406320ULL, 6250347750920ULL, 8654327655120ULL, 11899700525790ULL, 16253249498640ULL, 22057981462440ULL, 29752626158640ULL, 39895566894540ULL, 53194089192720ULL, 70539987842520ULL, 93052749919920ULL, 122131734269895ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 17ULL, 153ULL, 969ULL, 4845ULL, 20349ULL, 74613ULL, 245157ULL, 735471ULL, 2042975ULL, 5311735ULL, 13037895ULL, 30421755ULL, 67863915ULL, 145422675ULL, 300540195ULL, 601080390ULL, 1166803110ULL, 2203961430ULL, 4059928950ULL, 7307872110ULL, 12875774670ULL, 22239974430ULL, 37711260990ULL, 62852101650ULL, 103077446706ULL, 166509721602ULL, 265182149218ULL, 416714805914ULL, 646626422970ULL, 991493848554ULL, 1503232609098ULL, 2254848913647ULL, 3348108992991ULL, 4923689695575ULL, 7174519270695ULL, 10363194502115ULL, 14844575908435ULL, 21094923659355ULL, 29749251314475ULL, 41648951840265ULL, 57902201338905ULL, 79960182801345ULL, 109712808959985ULL, 149608375854525ULL, 202802465047245ULL, 273342452889765ULL, 366395202809685ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 18ULL, 171ULL, 1140ULL, 5985ULL, 26334ULL, 100947ULL, 346104ULL, 1081575ULL, 3124550ULL, 8436285ULL, 21474180ULL, 51895935ULL, 119759850ULL, 265182525ULL, 565722720ULL, 1166803110ULL, 2333606220ULL, 4537567650ULL, 8597496600ULL, 15905368710ULL, 28781143380ULL, 51021117810ULL, 88732378800ULL, 151584480450ULL, 254661927156ULL, 421171648758ULL, 686353797976ULL, 1103068603890ULL, 1749695026860ULL, 2741188875414ULL, 4244421484512ULL, 6499270398159ULL, 9847379391150ULL, 14771069086725ULL, 21945588357420ULL, 32308782859535ULL, 47153358767970ULL, 68248282427325ULL, 97997533741800ULL, 139646485582065ULL, 197548686920970ULL, 277508869722315ULL, 387221678682300ULL, 536830054536825ULL, 739632519584070ULL, 1012974972473835ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 19ULL, 190ULL, 1330ULL, 7315ULL, 33649ULL, 134596ULL, 480700ULL, 1562275ULL, 4686825ULL, 13123110ULL, 34597290ULL, 86493225ULL, 206253075ULL, 471435600ULL, 1037158320ULL, 2203961430ULL, 4537567650ULL, 9075135300ULL, 17672631900ULL, 33578000610ULL, 62359143990ULL, 113380261800ULL, 202112640600ULL, 353697121050ULL, 608359048206ULL, 1029530696964ULL, 1715884494940ULL, 2818953098830ULL, 4568648125690ULL, 7309837001104ULL, 11554258485616ULL, 18053528883775ULL, 27900908274925ULL, 42671977361650ULL, 64617565719070ULL, 96926348578605ULL, 144079707346575ULL, 212327989773900ULL, 310325523515700ULL, 449972009097765ULL, 647520696018735ULL, 925029565741050ULL, 1312251244423350ULL, 1849081298960175ULL, 2588713818544245ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 20ULL, 210ULL, 1540ULL, 8855ULL, 42504ULL, 177100ULL, 657800ULL, 2220075ULL, 6906900ULL, 20030010ULL, 54627300ULL, 141120525ULL, 347373600ULL, 818809200ULL, 1855967520ULL, 4059928950ULL, 8597496600ULL, 17672631900ULL, 35345263800ULL, 68923264410ULL, 131282408400ULL, 244662670200ULL, 446775310800ULL, 800472431850ULL, 1408831480056ULL, 2438362177020ULL, 4154246671960ULL, 6973199770790ULL, 11541847896480ULL, 18851684897584ULL, 30405943383200ULL, 48459472266975ULL, 76360380541900ULL, 119032357903550ULL, 183649923622620ULL, 280576272201225ULL, 424655979547800ULL, 636983969321700ULL, 947309492837400ULL, 1397281501935165ULL, 2044802197953900ULL, 2969831763694950ULL, 4282083008118300ULL, 6131164307078475ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 21ULL, 231ULL, 1771ULL, 10626ULL, 53130ULL, 230230ULL, 888030ULL, 3108105ULL, 10015005ULL, 30045015ULL, 84672315ULL, 225792840ULL, 573166440ULL, 1391975640ULL, 3247943160ULL, 7307872110ULL, 15905368710ULL, 33578000610ULL, 68923264410ULL, 137846528820ULL, 269128937220ULL, 513791607420ULL, 960566918220ULL, 1761039350070ULL, 3169870830126ULL, 5608233007146ULL, 9762479679106ULL, 16735679449896ULL, 28277527346376ULL, 47129212243960ULL, 77535155627160ULL, 125994627894135ULL, 202355008436035ULL, 321387366339585ULL, 505037289962205ULL, 785613562163430ULL, 1210269541711230ULL, 1847253511032930ULL, 2794563003870330ULL, 4191844505805495ULL, 6236646703759395ULL, 9206478467454345ULL, 13488561475572645ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 22ULL, 253ULL, 2024ULL, 12650ULL, 65780ULL, 296010ULL, 1184040ULL, 4292145ULL, 14307150ULL, 44352165ULL, 129024480ULL, 354817320ULL, 927983760ULL, 2319959400ULL, 5567902560ULL, 12875774670ULL, 28781143380ULL, 62359143990ULL, 131282408400ULL, 269128937220ULL, 538257874440ULL, 1052049481860ULL, 2012616400080ULL, 3773655750150ULL, 6943526580276ULL, 12551759587422ULL, 22314239266528ULL, 39049918716424ULL, 67327446062800ULL, 114456658306760ULL, 191991813933920ULL, 317986441828055ULL, 520341450264090ULL, 841728816603675ULL, 1346766106565880ULL, 2132379668729310ULL, 3342649210440540ULL, 5189902721473470ULL, 7984465725343800ULL, 12176310231149295ULL, 18412956934908690ULL, 27619435402363035ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 23ULL, 276ULL, 2300ULL, 14950ULL, 80730ULL, 376740ULL, 1560780ULL, 5852925ULL, 20160075ULL, 64512240ULL, 193536720ULL, 548354040ULL, 1476337800ULL, 3796297200ULL, 9364199760ULL, 22239974430ULL, 51021117810ULL, 113380261800ULL, 244662670200ULL, 513791607420ULL, 1052049481860ULL, 2104098963720ULL, 4116715363800ULL, 7890371113950ULL, 14833897694226ULL, 27385657281648ULL, 49699896548176ULL, 88749815264600ULL, 156077261327400ULL, 270533919634160ULL, 462525733568080ULL, 780512175396135ULL, 1300853625660225ULL, 2142582442263900ULL, 3489348548829780ULL, 5621728217559090ULL, 8964377427999630ULL, 14154280149473100ULL, 22138745874816900ULL, 34315056105966195ULL, 52728013040874885ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 24ULL, 300ULL, 2600ULL, 17550ULL, 98280ULL, 475020ULL, 2035800ULL, 7888725ULL, 28048800ULL, 92561040ULL, 286097760ULL, 834451800ULL, 2310789600ULL, 6107086800ULL, 15471286560ULL, 37711260990ULL, 88732378800ULL, 202112640600ULL, 446775310800ULL, 960566918220ULL, 2012616400080ULL, 4116715363800ULL, 8233430727600ULL, 16123801841550ULL, 30957699535776ULL, 58343356817424ULL, 108043253365600ULL, 196793068630200ULL, 352870329957600ULL, 623404249591760ULL, 1085929983159840ULL, 1866442158555975ULL, 3167295784216200ULL, 5309878226480100ULL, 8799226775309880ULL, 14420954992868970ULL, 23385332420868600ULL, 37539612570341700ULL, 59678358445158600ULL, 93993414551124795ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 25ULL, 325ULL, 2925ULL, 20475ULL, 118755ULL, 593775ULL, 2629575ULL, 10518300ULL, 38567100ULL, 131128140ULL, 417225900ULL, 1251677700ULL, 3562467300ULL, 9669554100ULL, 25140840660ULL, 62852101650ULL, 151584480450ULL, 353697121050ULL, 800472431850ULL, 1761039350070ULL, 3773655750150ULL, 7890371113950ULL, 16123801841550ULL, 32247603683100ULL, 63205303218876ULL, 121548660036300ULL, 229591913401900ULL, 426384982032100ULL, 779255311989700ULL, 1402659561581460ULL, 2488589544741300ULL, 4355031703297275ULL, 7522327487513475ULL, 12832205713993575ULL, 21631432489303455ULL, 36052387482172425ULL, 59437719903041025ULL, 96977332473382725ULL, 156655690918541325ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 26ULL, 351ULL, 3276ULL, 23751ULL, 142506ULL, 736281ULL, 3365856ULL, 13884156ULL, 52451256ULL, 183579396ULL, 600805296ULL, 1852482996ULL, 5414950296ULL, 15084504396ULL, 40225345056ULL, 103077446706ULL, 254661927156ULL, 608359048206ULL, 1408831480056ULL, 3169870830126ULL, 6943526580276ULL, 14833897694226ULL, 30957699535776ULL, 63205303218876ULL, 126410606437752ULL, 247959266474052ULL, 477551179875952ULL, 903936161908052ULL, 1683191473897752ULL, 3085851035479212ULL, 5574440580220512ULL, 9929472283517787ULL, 17451799771031262ULL, 30284005485024837ULL, 51915437974328292ULL, 87967825456500717ULL, 147405545359541742ULL, 244382877832924467ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 27ULL, 378ULL, 3654ULL, 27405ULL, 169911ULL, 906192ULL, 4272048ULL, 18156204ULL, 70607460ULL, 254186856ULL, 854992152ULL, 2707475148ULL, 8122425444ULL, 23206929840ULL, 63432274896ULL, 166509721602ULL, 421171648758ULL, 1029530696964ULL, 2438362177020ULL, 5608233007146ULL, 12551759587422ULL, 27385657281648ULL, 58343356817424ULL, 121548660036300ULL, 247959266474052ULL, 495918532948104ULL, 973469712824056ULL, 1877405874732108ULL, 3560597348629860ULL, 6646448384109072ULL, 12220888964329584ULL, 22150361247847371ULL, 39602161018878633ULL, 69886166503903470ULL, 121801604478231762ULL, 209769429934732479ULL, 357174975294274221ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 28ULL, 406ULL, 4060ULL, 31465ULL, 201376ULL, 1107568ULL, 5379616ULL, 23535820ULL, 94143280ULL, 348330136ULL, 1203322288ULL, 3910797436ULL, 12033222880ULL, 35240152720ULL, 98672427616ULL, 265182149218ULL, 686353797976ULL, 1715884494940ULL, 4154246671960ULL, 9762479679106ULL, 22314239266528ULL, 49699896548176ULL, 108043253365600ULL, 229591913401900ULL, 477551179875952ULL, 973469712824056ULL, 1946939425648112ULL, 3824345300380220ULL, 7384942649010080ULL, 14031391033119152ULL, 26252279997448736ULL, 48402641245296107ULL, 88004802264174740ULL, 157890968768078210ULL, 279692573246309972ULL, 489462003181042451ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 29ULL, 435ULL, 4495ULL, 35960ULL, 237336ULL, 1344904ULL, 6724520ULL, 30260340ULL, 124403620ULL, 472733756ULL, 1676056044ULL, 5586853480ULL, 17620076360ULL, 52860229080ULL, 151532656696ULL, 416714805914ULL, 1103068603890ULL, 2818953098830ULL, 6973199770790ULL, 16735679449896ULL, 39049918716424ULL, 88749815264600ULL, 196793068630200ULL, 426384982032100ULL, 903936161908052ULL, 1877405874732108ULL, 3824345300380220ULL, 7648690600760440ULL, 15033633249770520ULL, 29065024282889672ULL, 55317304280338408ULL, 103719945525634515ULL, 191724747789809255ULL, 349615716557887465ULL, 629308289804197437ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 30ULL, 465ULL, 4960ULL, 40920ULL, 278256ULL, 1623160ULL, 8347680ULL, 38608020ULL, 163011640ULL, 635745396ULL, 2311801440ULL, 7898654920ULL, 25518731280ULL, 78378960360ULL, 229911617056ULL, 646626422970ULL, 1749695026860ULL, 4568648125690ULL, 11541847896480ULL, 28277527346376ULL, 67327446062800ULL, 156077261327400ULL, 352870329957600ULL, 779255311989700ULL, 1683191473897752ULL, 3560597348629860ULL, 7384942649010080ULL, 15033633249770520ULL, 30067266499541040ULL, 59132290782430712ULL, 114449595062769120ULL, 218169540588403635ULL, 409894288378212890ULL, 759510004936100355ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 31ULL, 496ULL, 5456ULL, 46376ULL, 324632ULL, 1947792ULL, 10295472ULL, 48903492ULL, 211915132ULL, 847660528ULL, 3159461968ULL, 11058116888ULL, 36576848168ULL, 114955808528ULL, 344867425584ULL, 991493848554ULL, 2741188875414ULL, 7309837001104ULL, 18851684897584ULL, 47129212243960ULL, 114456658306760ULL, 270533919634160ULL, 623404249591760ULL, 1402659561581460ULL, 3085851035479212ULL, 6646448384109072ULL, 14031391033119152ULL, 29065024282889672ULL, 59132290782430712ULL, 118264581564861424ULL, 232714176627630544ULL, 450883717216034179ULL, 860778005594247069ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 32ULL, 528ULL, 5984ULL, 52360ULL, 376992ULL, 2324784ULL, 12620256ULL, 61523748ULL, 273438880ULL, 1121099408ULL, 4280561376ULL, 15338678264ULL, 51915526432ULL, 166871334960ULL, 511738760544ULL, 1503232609098ULL, 4244421484512ULL, 11554258485616ULL, 30405943383200ULL, 77535155627160ULL, 191991813933920ULL, 462525733568080ULL, 1085929983159840ULL, 2488589544741300ULL, 5574440580220512ULL, 12220888964329584ULL, 26252279997448736ULL, 55317304280338408ULL, 114449595062769120ULL, 232714176627630544ULL, 465428353255261088ULL, 916312070471295267ULL },
    { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 33ULL, 561ULL, 6545ULL, 58905ULL, 435897ULL, 2760681ULL, 15380937ULL, 76904685ULL, 350343565ULL, 1471442973ULL, 5752004349ULL, 21090682613ULL, 73006209045ULL, 239877544005ULL, 751616304549ULL, 2254848913647ULL, 6499270398159ULL, 18053528883775ULL, 48459472266975ULL, 125994627894135ULL, 317986441828055ULL, 780512175396135ULL, 1866442158555975ULL, 4355031703297275ULL, 9929472283517787ULL, 22150361247847371ULL, 48402641245296107ULL, 103719945525634515ULL, 218169540588403635ULL, 450883717216034179ULL, 916312070471295267ULL }
};

}
}

NS_IZENELIB_AM_END
#line 1 "lcron.ragel"
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define MAX_LEN 100

#define DAYS_OF_MONTH_START 1
#define DAYS_OF_MONTH_END 31
#define MONTHS_START 1
#define MONTHS_END 12
#define DAYS_OF_WEEK_START 1
#define YEARS_START 1970
#define TM_YEAR_START 1900

/* Remember about Unix Millenium Bug; mktime() will simply return -1 for dates
 * later than 2038 but we will avoid a couple of iterations. */
#if __WORDSIZE > 32
#define YEARS_END 2099
#else
#define YEARS_END 2037
#endif

#define SECONDS_LEN 60
#define MINUTES_LEN 60
#define HOURS_LEN 24
#define DAYS_OF_MONTH_LEN 31
#define MONTHS_LEN 12
#define DAYS_OF_WEEK_LEN 7
#define YEARS_LEN (YEARS_END - YEARS_START + 1)
#define NUMBER_OF_WEEKS 5

#define SUNDAY 0
#define MONDAY 1
#define FRIDAY 5
#define SATURDAY 6

#define MARK_NORMAL 1
#define MARK_WEEKDAY 2
#define MARK_1_IN_MONTH 4
#define MARK_2_IN_MONTH 8
#define MARK_3_IN_MONTH 16
#define MARK_4_IN_MONTH 32
#define MARK_5_IN_MONTH 64
#define MARK_L_IN_MONTH 128

#define call_l_next(literal) \
    time_t t = luaL_optint(L, 2, time(NULL)); \
    lua_pop(L, lua_gettop(L)); \
    lua_pushliteral(L, literal); \
    lua_pushinteger(L, t); \
    return l_next(L)

#line 330 "lcron.ragel"



#line 63 "lcron.c"
static const int crontab_start = 1;
static const int crontab_first_final = 269;
static const int crontab_error = 0;

static const int crontab_en_main = 1;

#line 333 "lcron.ragel"

static inline void inc_year(struct tm *broken_time) {
    broken_time->tm_mon = 0;
    broken_time->tm_mday = 1;
    broken_time->tm_hour = 0;
    broken_time->tm_min = 0;
    broken_time->tm_sec = 0;
}

static inline void inc_month(struct tm *broken_time) {
    broken_time->tm_mday = 1;
    broken_time->tm_hour = 0;
    broken_time->tm_min = 0;
    broken_time->tm_sec = 0;
}

static inline void inc_mday(struct tm *broken_time) {
    broken_time->tm_hour = 0;
    broken_time->tm_min = 0;
    broken_time->tm_sec = 0;
}

static inline void inc_hour(struct tm *broken_time) {
    broken_time->tm_min = 0;
    broken_time->tm_sec = 0;
}

static inline void inc_min(struct tm *broken_time) {
    broken_time->tm_sec = 0;
}

static inline void next_mday(struct tm *broken_time) {
    broken_time->tm_mday++;
    broken_time->tm_isdst = -1;
    mktime(broken_time);
}

static inline void copy_time(struct tm *src, struct tm *dst) {
    dst->tm_year = src->tm_year;
    dst->tm_mon = src->tm_mon;
    dst->tm_mday = src->tm_mday;
    dst->tm_hour = src->tm_hour;
    dst->tm_min = src->tm_min;
    dst->tm_sec = src->tm_sec;
    dst->tm_wday = src->tm_wday;
    dst->tm_isdst = -1;
}

static int is_last_day_of_month(struct tm *src_time) {
    struct tm broken_time;

    if (src_time->tm_mday < 28) {
        return 0;
    }

    copy_time(src_time, &broken_time);

    int day = broken_time.tm_mday;
    int month = broken_time.tm_mon;
    int last_day = day;

    while (broken_time.tm_mon == month) {
        last_day = broken_time.tm_mday;
        next_mday(&broken_time);
    }

    return last_day == day;
}

static inline int is_closest_weekday(struct tm *src_time, short days_of_month[]) {
    int i, j;
    int day, month;
    struct tm broken_time;

    day = src_time->tm_mday;
    month = src_time->tm_mon;

    if (src_time->tm_wday != MONDAY && src_time->tm_wday != FRIDAY) {
        return 0;
    }

    copy_time(src_time, &broken_time);

    if (broken_time.tm_wday == MONDAY && day > 1) {
        if (day == 3) {
            return days_of_month[0] & MARK_WEEKDAY || days_of_month[1] & MARK_WEEKDAY;
        } else {
            return days_of_month[day - DAYS_OF_MONTH_START - 1] & MARK_WEEKDAY;
        }
    } else if (broken_time.tm_wday == FRIDAY) {
        /* Look at the following Saturday */
        next_mday(&broken_time);

        if (broken_time.tm_mon != month) {
            return 0;
        }

        if (days_of_month[day - DAYS_OF_MONTH_START + 1] & MARK_WEEKDAY) {
            return 1;
        }

        /* We also have to check the situation when the last day of the month is
         * Sunday
         */
        next_mday(&broken_time);

        return broken_time.tm_mon == month &&
            days_of_month[day - DAYS_OF_MONTH_START + 2] & MARK_WEEKDAY &&
            is_last_day_of_month(&broken_time);
    }
}

static inline int is_numbered_week_day(struct tm *src_time, short days_of_week[]) {
    struct tm broken_time;
    int wday = src_time->tm_wday;
    short day_marks = days_of_week[wday];
    int result = 0;
    int day = src_time->tm_mday;
    int month = src_time->tm_mon;

    if (!day_marks) {
        return 0;
    }

    copy_time(src_time, &broken_time);

    if (day_marks & MARK_L_IN_MONTH) {
        result = 1;
        next_mday(&broken_time);

        while (broken_time.tm_mon == month) {
            if (broken_time.tm_wday == wday) {
                result = 0;
                break;
            }
            next_mday(&broken_time);
        }
    }

    if (!result && day_marks >= MARK_1_IN_MONTH && day_marks < MARK_L_IN_MONTH) {
        int i, current_week;

        for (i=1; i<=7; i++) {
            broken_time.tm_mday = i;
            broken_time.tm_mon = month;
            mktime(&broken_time);
            if (broken_time.tm_wday == wday) {
                break;
            }
        }

        current_week = (day - i) / 7;
        result = day_marks & (MARK_1_IN_MONTH << current_week);
    }

    return result;
}

static int l_next(lua_State *L) {
    int i;
    int cs, res = 0;

    /* Variables used within Ragel action blocks */
    int range_start, range_end;
    int increment_start, increment_interval;
    int numbered_day, numbered_week;
    short *current;
    int current_len, current_offset;

    char expression[MAX_LEN];
    int expression_len;
    time_t current_time, time_result;
    struct tm *current_time_tm;
    struct tm broken_time;

    expression_len = lua_objlen(L, 1);

    if (expression_len >= MAX_LEN) {
        lua_pushinteger(L, -1);
        return 1;
    }

    /* We're modifing the string in-place so let's make a copy */
    strcpy(expression, luaL_checkstring(L, 1));

    /* Initialize Ragel variables */
    char *p = expression;
    char *pe = p + expression_len;
    char *eof = pe;
    char *token_start = p;

    short seconds[SECONDS_LEN] = {0};
    short minutes[MINUTES_LEN] = {0};
    short hours[HOURS_LEN] = {0};
    short days_of_month[DAYS_OF_MONTH_LEN + 1] = {0};
    short months[MONTHS_LEN] = {0};
    short days_of_week[DAYS_OF_WEEK_LEN] = {0};
    short years[YEARS_LEN] = {0};
    short second, minute, hour, day_of_month, month, day_of_week, year;

    /* Because year is optional we set all items to 1 initially */
    for (i=0; i<YEARS_LEN; i++) {
        years[i] = 1;
    }

    
#line 277 "lcron.c"
	{
	cs = crontab_start;
	}
#line 539 "lcron.ragel"
    
#line 283 "lcron.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
case 1:
	switch( (*p) ) {
		case 42: goto tr0;
		case 48: goto tr2;
		case 64: goto st232;
	}
	if ( (*p) > 53 ) {
		if ( 54 <= (*p) && (*p) <= 57 )
			goto tr2;
	} else if ( (*p) >= 49 )
		goto tr3;
	goto st0;
st0:
cs = 0;
	goto _out;
tr0:
#line 65 "lcron.ragel"
	{
        current = seconds;
        current_len = SECONDS_LEN;
        current_offset = 0;
    }
	goto st2;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
#line 316 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr5;
		case 44: goto tr6;
		case 47: goto tr7;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr5;
	goto st0;
tr5:
#line 114 "lcron.ragel"
	{
        for (i = 0; i < current_len; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st3;
tr500:
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st3;
tr506:
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st3;
tr510:
#line 146 "lcron.ragel"
	{
        increment_interval = atoi(token_start);
    }
#line 150 "lcron.ragel"
	{
        while (increment_start < current_len) {
            current[increment_start] |= MARK_NORMAL;
            increment_start += increment_interval;
        }
    }
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 372 "lcron.c"
	switch( (*p) ) {
		case 32: goto st3;
		case 42: goto tr9;
		case 48: goto tr10;
	}
	if ( (*p) < 49 ) {
		if ( 9 <= (*p) && (*p) <= 13 )
			goto st3;
	} else if ( (*p) > 53 ) {
		if ( 54 <= (*p) && (*p) <= 57 )
			goto tr10;
	} else
		goto tr11;
	goto st0;
tr9:
#line 71 "lcron.ragel"
	{
        current = minutes;
        current_len = MINUTES_LEN;
        current_offset = 0;
    }
	goto st4;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 399 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr12;
		case 44: goto tr13;
		case 47: goto tr14;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr12;
	goto st0;
tr12:
#line 114 "lcron.ragel"
	{
        for (i = 0; i < current_len; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st5;
tr483:
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st5;
tr489:
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st5;
tr493:
#line 146 "lcron.ragel"
	{
        increment_interval = atoi(token_start);
    }
#line 150 "lcron.ragel"
	{
        while (increment_start < current_len) {
            current[increment_start] |= MARK_NORMAL;
            increment_start += increment_interval;
        }
    }
	goto st5;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
#line 455 "lcron.c"
	switch( (*p) ) {
		case 32: goto st5;
		case 42: goto tr16;
		case 49: goto tr18;
		case 50: goto tr19;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr17;
	} else if ( (*p) >= 9 )
		goto st5;
	goto st0;
tr16:
#line 77 "lcron.ragel"
	{
        current = hours;
        current_len = HOURS_LEN;
        current_offset = 0;
    }
	goto st6;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 480 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr20;
		case 44: goto tr21;
		case 47: goto tr22;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr20;
	goto st0;
tr20:
#line 114 "lcron.ragel"
	{
        for (i = 0; i < current_len; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st7;
tr465:
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st7;
tr472:
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st7;
tr476:
#line 146 "lcron.ragel"
	{
        increment_interval = atoi(token_start);
    }
#line 150 "lcron.ragel"
	{
        while (increment_start < current_len) {
            current[increment_start] |= MARK_NORMAL;
            increment_start += increment_interval;
        }
    }
	goto st7;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 536 "lcron.c"
	switch( (*p) ) {
		case 32: goto st7;
		case 42: goto tr24;
		case 51: goto tr26;
		case 63: goto tr28;
		case 76: goto tr29;
	}
	if ( (*p) < 49 ) {
		if ( 9 <= (*p) && (*p) <= 13 )
			goto st7;
	} else if ( (*p) > 50 ) {
		if ( 52 <= (*p) && (*p) <= 57 )
			goto tr27;
	} else
		goto tr25;
	goto st0;
tr24:
#line 83 "lcron.ragel"
	{
        current = days_of_month;
        current_len = DAYS_OF_MONTH_LEN;
        current_offset = DAYS_OF_MONTH_START;
    }
	goto st8;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 565 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr30;
		case 44: goto tr31;
		case 47: goto tr32;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr30;
	goto st0;
tr30:
#line 114 "lcron.ragel"
	{
        for (i = 0; i < current_len; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st9;
tr222:
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st9;
tr231:
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st9;
tr235:
#line 146 "lcron.ragel"
	{
        increment_interval = atoi(token_start);
    }
#line 150 "lcron.ragel"
	{
        while (increment_start < current_len) {
            current[increment_start] |= MARK_NORMAL;
            increment_start += increment_interval;
        }
    }
	goto st9;
tr238:
#line 161 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_WEEKDAY;
    }
	goto st9;
tr240:
#line 157 "lcron.ragel"
	{
        current[DAYS_OF_MONTH_LEN] |= MARK_NORMAL;
    }
	goto st9;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
#line 633 "lcron.c"
	switch( (*p) ) {
		case 32: goto st9;
		case 42: goto tr34;
		case 49: goto tr35;
		case 65: goto tr37;
		case 68: goto tr38;
		case 70: goto tr39;
		case 74: goto tr40;
		case 77: goto tr41;
		case 78: goto tr42;
		case 79: goto tr43;
		case 83: goto tr44;
		case 97: goto tr37;
		case 100: goto tr38;
		case 102: goto tr39;
		case 106: goto tr40;
		case 109: goto tr41;
		case 110: goto tr42;
		case 111: goto tr43;
		case 115: goto tr44;
	}
	if ( (*p) > 13 ) {
		if ( 50 <= (*p) && (*p) <= 57 )
			goto tr36;
	} else if ( (*p) >= 9 )
		goto st9;
	goto st0;
tr34:
#line 89 "lcron.ragel"
	{
        current = months;
        current_len = MONTHS_LEN;
        current_offset = MONTHS_START;
    }
	goto st10;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
#line 673 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr45;
		case 44: goto tr46;
		case 47: goto tr47;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr45;
	goto st0;
tr45:
#line 114 "lcron.ragel"
	{
        for (i = 0; i < current_len; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st11;
tr79:
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st11;
tr94:
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st11;
tr100:
#line 242 "lcron.ragel"
	{*token_start = '4';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st11;
tr103:
#line 246 "lcron.ragel"
	{*token_start = '8';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st11;
tr107:
#line 250 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '2';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st11;
tr111:
#line 240 "lcron.ragel"
	{*token_start = '2';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st11;
tr116:
#line 239 "lcron.ragel"
	{*token_start = '1';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st11;
tr120:
#line 245 "lcron.ragel"
	{*token_start = '7';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st11;
tr122:
#line 244 "lcron.ragel"
	{*token_start = '6';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st11;
tr127:
#line 241 "lcron.ragel"
	{*token_start = '3';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st11;
tr129:
#line 243 "lcron.ragel"
	{*token_start = '5';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st11;
tr133:
#line 249 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '1';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st11;
tr137:
#line 248 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '0';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st11;
tr141:
#line 247 "lcron.ragel"
	{*token_start = '9';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st11;
tr144:
#line 146 "lcron.ragel"
	{
        increment_interval = atoi(token_start);
    }
#line 150 "lcron.ragel"
	{
        while (increment_start < current_len) {
            current[increment_start] |= MARK_NORMAL;
            increment_start += increment_interval;
        }
    }
	goto st11;
tr150:
#line 242 "lcron.ragel"
	{*token_start = '4';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st11;
tr155:
#line 246 "lcron.ragel"
	{*token_start = '8';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st11;
tr161:
#line 250 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '2';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st11;
tr167:
#line 240 "lcron.ragel"
	{*token_start = '2';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st11;
tr174:
#line 239 "lcron.ragel"
	{*token_start = '1';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st11;
tr180:
#line 245 "lcron.ragel"
	{*token_start = '7';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st11;
tr184:
#line 244 "lcron.ragel"
	{*token_start = '6';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st11;
tr191:
#line 241 "lcron.ragel"
	{*token_start = '3';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st11;
tr195:
#line 243 "lcron.ragel"
	{*token_start = '5';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st11;
tr201:
#line 249 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '1';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st11;
tr207:
#line 248 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '0';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st11;
tr213:
#line 247 "lcron.ragel"
	{*token_start = '9';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st11;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
#line 1041 "lcron.c"
	switch( (*p) ) {
		case 32: goto st11;
		case 63: goto tr49;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto st11;
	goto st0;
tr49:
#line 95 "lcron.ragel"
	{
        current = days_of_week;
        current_len = DAYS_OF_WEEK_LEN;
        current_offset = DAYS_OF_WEEK_START;
    }
	goto st269;
st269:
	if ( ++p == pe )
		goto _test_eof269;
case 269:
#line 1061 "lcron.c"
	if ( (*p) == 32 )
		goto tr555;
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr555;
	goto st0;
tr555:
#line 114 "lcron.ragel"
	{
        for (i = 0; i < current_len; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st12;
tr566:
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st12;
tr574:
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st12;
tr592:
#line 146 "lcron.ragel"
	{
        increment_interval = atoi(token_start);
    }
#line 150 "lcron.ragel"
	{
        while (increment_start < current_len) {
            current[increment_start] |= MARK_NORMAL;
            increment_start += increment_interval;
        }
    }
	goto st12;
tr572:
#line 169 "lcron.ragel"
	{
        numbered_week = atoi(token_start) - DAYS_OF_WEEK_START;
    }
#line 173 "lcron.ragel"
	{
        days_of_week[numbered_day] |= MARK_1_IN_MONTH << numbered_week;
    }
	goto st12;
tr576:
#line 271 "lcron.ragel"
	{*token_start = '6';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st12;
tr578:
#line 273 "lcron.ragel"
	{*token_start = '1';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st12;
tr580:
#line 267 "lcron.ragel"
	{*token_start = '2';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st12;
tr582:
#line 272 "lcron.ragel"
	{*token_start = '7';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st12;
tr584:
#line 266 "lcron.ragel"
	{*token_start = '1';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st12;
tr586:
#line 270 "lcron.ragel"
	{*token_start = '5';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st12;
tr588:
#line 268 "lcron.ragel"
	{*token_start = '3';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st12;
tr590:
#line 269 "lcron.ragel"
	{*token_start = '4';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st12;
tr595:
#line 177 "lcron.ragel"
	{
        days_of_week[numbered_day] |= MARK_L_IN_MONTH;
    }
	goto st12;
tr597:
#line 271 "lcron.ragel"
	{*token_start = '6';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st12;
tr603:
#line 273 "lcron.ragel"
	{*token_start = '1';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st12;
tr609:
#line 267 "lcron.ragel"
	{*token_start = '2';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st12;
tr615:
#line 272 "lcron.ragel"
	{*token_start = '7';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st12;
tr621:
#line 266 "lcron.ragel"
	{*token_start = '1';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st12;
tr627:
#line 270 "lcron.ragel"
	{*token_start = '5';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st12;
tr633:
#line 268 "lcron.ragel"
	{*token_start = '3';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st12;
tr639:
#line 269 "lcron.ragel"
	{*token_start = '4';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st12;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
#line 1338 "lcron.c"
	switch( (*p) ) {
		case 32: goto st12;
		case 42: goto tr51;
		case 49: goto tr52;
		case 50: goto tr53;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto st12;
	goto st0;
tr51:
#line 101 "lcron.ragel"
	{
        current = years;
        current_len = YEARS_LEN;
        current_offset = YEARS_START;
        for (i=0; i<YEARS_LEN; i++) {
            years[i] = 0;
        }
    }
	goto st270;
st270:
	if ( ++p == pe )
		goto _test_eof270;
case 270:
#line 1363 "lcron.c"
	switch( (*p) ) {
		case 44: goto tr556;
		case 47: goto tr557;
	}
	goto st0;
tr556:
#line 114 "lcron.ragel"
	{
        for (i = 0; i < current_len; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st13;
tr558:
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st13;
tr561:
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st13;
tr562:
#line 146 "lcron.ragel"
	{
        increment_interval = atoi(token_start);
    }
#line 150 "lcron.ragel"
	{
        while (increment_start < current_len) {
            current[increment_start] |= MARK_NORMAL;
            increment_start += increment_interval;
        }
    }
	goto st13;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
#line 1416 "lcron.c"
	switch( (*p) ) {
		case 42: goto st270;
		case 49: goto tr55;
		case 50: goto tr56;
	}
	goto st0;
tr52:
#line 101 "lcron.ragel"
	{
        current = years;
        current_len = YEARS_LEN;
        current_offset = YEARS_START;
        for (i=0; i<YEARS_LEN; i++) {
            years[i] = 0;
        }
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st14;
tr55:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st14;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
#line 1448 "lcron.c"
	if ( (*p) == 57 )
		goto st15;
	goto st0;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
	if ( 55 <= (*p) && (*p) <= 57 )
		goto st16;
	goto st0;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st271;
	goto st0;
st271:
	if ( ++p == pe )
		goto _test_eof271;
case 271:
	switch( (*p) ) {
		case 44: goto tr558;
		case 45: goto tr559;
		case 47: goto tr560;
	}
	goto st0;
tr559:
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st17;
st17:
	if ( ++p == pe )
		goto _test_eof17;
case 17:
#line 1486 "lcron.c"
	switch( (*p) ) {
		case 49: goto tr60;
		case 50: goto tr61;
	}
	goto st0;
tr60:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st18;
st18:
	if ( ++p == pe )
		goto _test_eof18;
case 18:
#line 1502 "lcron.c"
	if ( (*p) == 57 )
		goto st19;
	goto st0;
st19:
	if ( ++p == pe )
		goto _test_eof19;
case 19:
	if ( 55 <= (*p) && (*p) <= 57 )
		goto st20;
	goto st0;
st20:
	if ( ++p == pe )
		goto _test_eof20;
case 20:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st272;
	goto st0;
st272:
	if ( ++p == pe )
		goto _test_eof272;
case 272:
	if ( (*p) == 44 )
		goto tr561;
	goto st0;
tr61:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st21;
st21:
	if ( ++p == pe )
		goto _test_eof21;
case 21:
#line 1537 "lcron.c"
	if ( (*p) == 48 )
		goto st22;
	goto st0;
st22:
	if ( ++p == pe )
		goto _test_eof22;
case 22:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st20;
	goto st0;
tr557:
#line 142 "lcron.ragel"
	{
        increment_start = 0;
    }
	goto st23;
tr560:
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st23;
st23:
	if ( ++p == pe )
		goto _test_eof23;
case 23:
#line 1564 "lcron.c"
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr66;
	goto st0;
tr66:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st273;
st273:
	if ( ++p == pe )
		goto _test_eof273;
case 273:
#line 1578 "lcron.c"
	if ( (*p) == 44 )
		goto tr562;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st273;
	goto st0;
tr53:
#line 101 "lcron.ragel"
	{
        current = years;
        current_len = YEARS_LEN;
        current_offset = YEARS_START;
        for (i=0; i<YEARS_LEN; i++) {
            years[i] = 0;
        }
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st24;
tr56:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st24;
st24:
	if ( ++p == pe )
		goto _test_eof24;
case 24:
#line 1609 "lcron.c"
	if ( (*p) == 48 )
		goto st25;
	goto st0;
st25:
	if ( ++p == pe )
		goto _test_eof25;
case 25:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st16;
	goto st0;
tr46:
#line 114 "lcron.ragel"
	{
        for (i = 0; i < current_len; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st26;
tr80:
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st26;
tr95:
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st26;
tr101:
#line 242 "lcron.ragel"
	{*token_start = '4';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st26;
tr104:
#line 246 "lcron.ragel"
	{*token_start = '8';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st26;
tr108:
#line 250 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '2';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st26;
tr112:
#line 240 "lcron.ragel"
	{*token_start = '2';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st26;
tr117:
#line 239 "lcron.ragel"
	{*token_start = '1';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st26;
tr121:
#line 245 "lcron.ragel"
	{*token_start = '7';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st26;
tr123:
#line 244 "lcron.ragel"
	{*token_start = '6';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st26;
tr128:
#line 241 "lcron.ragel"
	{*token_start = '3';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st26;
tr130:
#line 243 "lcron.ragel"
	{*token_start = '5';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st26;
tr134:
#line 249 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '1';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st26;
tr138:
#line 248 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '0';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st26;
tr142:
#line 247 "lcron.ragel"
	{*token_start = '9';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st26;
tr145:
#line 146 "lcron.ragel"
	{
        increment_interval = atoi(token_start);
    }
#line 150 "lcron.ragel"
	{
        while (increment_start < current_len) {
            current[increment_start] |= MARK_NORMAL;
            increment_start += increment_interval;
        }
    }
	goto st26;
tr151:
#line 242 "lcron.ragel"
	{*token_start = '4';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st26;
tr156:
#line 246 "lcron.ragel"
	{*token_start = '8';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st26;
tr162:
#line 250 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '2';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st26;
tr168:
#line 240 "lcron.ragel"
	{*token_start = '2';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st26;
tr175:
#line 239 "lcron.ragel"
	{*token_start = '1';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st26;
tr181:
#line 245 "lcron.ragel"
	{*token_start = '7';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st26;
tr185:
#line 244 "lcron.ragel"
	{*token_start = '6';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st26;
tr192:
#line 241 "lcron.ragel"
	{*token_start = '3';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st26;
tr196:
#line 243 "lcron.ragel"
	{*token_start = '5';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st26;
tr202:
#line 249 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '1';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st26;
tr208:
#line 248 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '0';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st26;
tr214:
#line 247 "lcron.ragel"
	{*token_start = '9';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st26;
st26:
	if ( ++p == pe )
		goto _test_eof26;
case 26:
#line 1979 "lcron.c"
	switch( (*p) ) {
		case 42: goto st10;
		case 49: goto tr69;
		case 65: goto tr71;
		case 68: goto tr72;
		case 70: goto tr73;
		case 74: goto tr74;
		case 77: goto tr75;
		case 78: goto tr76;
		case 79: goto tr77;
		case 83: goto tr78;
		case 97: goto tr71;
		case 100: goto tr72;
		case 102: goto tr73;
		case 106: goto tr74;
		case 109: goto tr75;
		case 110: goto tr76;
		case 111: goto tr77;
		case 115: goto tr78;
	}
	if ( 50 <= (*p) && (*p) <= 57 )
		goto tr70;
	goto st0;
tr35:
#line 89 "lcron.ragel"
	{
        current = months;
        current_len = MONTHS_LEN;
        current_offset = MONTHS_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st27;
tr69:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st27;
st27:
	if ( ++p == pe )
		goto _test_eof27;
case 27:
#line 2025 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr79;
		case 44: goto tr80;
		case 45: goto tr81;
		case 47: goto tr82;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 50 )
			goto st63;
	} else if ( (*p) >= 9 )
		goto tr79;
	goto st0;
tr81:
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st28;
tr152:
#line 242 "lcron.ragel"
	{*token_start = '4';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st28;
tr157:
#line 246 "lcron.ragel"
	{*token_start = '8';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st28;
tr163:
#line 250 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '2';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st28;
tr169:
#line 240 "lcron.ragel"
	{*token_start = '2';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st28;
tr176:
#line 239 "lcron.ragel"
	{*token_start = '1';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st28;
tr182:
#line 245 "lcron.ragel"
	{*token_start = '7';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st28;
tr186:
#line 244 "lcron.ragel"
	{*token_start = '6';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st28;
tr193:
#line 241 "lcron.ragel"
	{*token_start = '3';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st28;
tr197:
#line 243 "lcron.ragel"
	{*token_start = '5';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st28;
tr203:
#line 249 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '1';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st28;
tr209:
#line 248 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '0';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st28;
tr215:
#line 247 "lcron.ragel"
	{*token_start = '9';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st28;
st28:
	if ( ++p == pe )
		goto _test_eof28;
case 28:
#line 2144 "lcron.c"
	switch( (*p) ) {
		case 49: goto tr84;
		case 65: goto tr86;
		case 68: goto tr87;
		case 70: goto tr88;
		case 74: goto tr89;
		case 77: goto tr90;
		case 78: goto tr91;
		case 79: goto tr92;
		case 83: goto tr93;
		case 97: goto tr86;
		case 100: goto tr87;
		case 102: goto tr88;
		case 106: goto tr89;
		case 109: goto tr90;
		case 110: goto tr91;
		case 111: goto tr92;
		case 115: goto tr93;
	}
	if ( 50 <= (*p) && (*p) <= 57 )
		goto tr85;
	goto st0;
tr84:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st29;
st29:
	if ( ++p == pe )
		goto _test_eof29;
case 29:
#line 2177 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr94;
		case 44: goto tr95;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 50 )
			goto st30;
	} else if ( (*p) >= 9 )
		goto tr94;
	goto st0;
tr85:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st30;
st30:
	if ( ++p == pe )
		goto _test_eof30;
case 30:
#line 2198 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr94;
		case 44: goto tr95;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr94;
	goto st0;
tr86:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st31;
st31:
	if ( ++p == pe )
		goto _test_eof31;
case 31:
#line 2216 "lcron.c"
	switch( (*p) ) {
		case 80: goto st32;
		case 85: goto st34;
		case 112: goto st32;
		case 117: goto st34;
	}
	goto st0;
st32:
	if ( ++p == pe )
		goto _test_eof32;
case 32:
	switch( (*p) ) {
		case 82: goto st33;
		case 114: goto st33;
	}
	goto st0;
st33:
	if ( ++p == pe )
		goto _test_eof33;
case 33:
	switch( (*p) ) {
		case 32: goto tr100;
		case 44: goto tr101;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr100;
	goto st0;
st34:
	if ( ++p == pe )
		goto _test_eof34;
case 34:
	switch( (*p) ) {
		case 71: goto st35;
		case 103: goto st35;
	}
	goto st0;
st35:
	if ( ++p == pe )
		goto _test_eof35;
case 35:
	switch( (*p) ) {
		case 32: goto tr103;
		case 44: goto tr104;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr103;
	goto st0;
tr87:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st36;
st36:
	if ( ++p == pe )
		goto _test_eof36;
case 36:
#line 2274 "lcron.c"
	switch( (*p) ) {
		case 69: goto st37;
		case 101: goto st37;
	}
	goto st0;
st37:
	if ( ++p == pe )
		goto _test_eof37;
case 37:
	switch( (*p) ) {
		case 67: goto st38;
		case 99: goto st38;
	}
	goto st0;
st38:
	if ( ++p == pe )
		goto _test_eof38;
case 38:
	switch( (*p) ) {
		case 32: goto tr107;
		case 44: goto tr108;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr107;
	goto st0;
tr88:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st39;
st39:
	if ( ++p == pe )
		goto _test_eof39;
case 39:
#line 2310 "lcron.c"
	switch( (*p) ) {
		case 69: goto st40;
		case 101: goto st40;
	}
	goto st0;
st40:
	if ( ++p == pe )
		goto _test_eof40;
case 40:
	switch( (*p) ) {
		case 66: goto st41;
		case 98: goto st41;
	}
	goto st0;
st41:
	if ( ++p == pe )
		goto _test_eof41;
case 41:
	switch( (*p) ) {
		case 32: goto tr111;
		case 44: goto tr112;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr111;
	goto st0;
tr89:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st42;
st42:
	if ( ++p == pe )
		goto _test_eof42;
case 42:
#line 2346 "lcron.c"
	switch( (*p) ) {
		case 65: goto st43;
		case 85: goto st45;
		case 97: goto st43;
		case 117: goto st45;
	}
	goto st0;
st43:
	if ( ++p == pe )
		goto _test_eof43;
case 43:
	switch( (*p) ) {
		case 78: goto st44;
		case 110: goto st44;
	}
	goto st0;
st44:
	if ( ++p == pe )
		goto _test_eof44;
case 44:
	switch( (*p) ) {
		case 32: goto tr116;
		case 44: goto tr117;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr116;
	goto st0;
st45:
	if ( ++p == pe )
		goto _test_eof45;
case 45:
	switch( (*p) ) {
		case 76: goto st46;
		case 78: goto st47;
		case 108: goto st46;
		case 110: goto st47;
	}
	goto st0;
st46:
	if ( ++p == pe )
		goto _test_eof46;
case 46:
	switch( (*p) ) {
		case 32: goto tr120;
		case 44: goto tr121;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr120;
	goto st0;
st47:
	if ( ++p == pe )
		goto _test_eof47;
case 47:
	switch( (*p) ) {
		case 32: goto tr122;
		case 44: goto tr123;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr122;
	goto st0;
tr90:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st48;
st48:
	if ( ++p == pe )
		goto _test_eof48;
case 48:
#line 2417 "lcron.c"
	switch( (*p) ) {
		case 65: goto st49;
		case 97: goto st49;
	}
	goto st0;
st49:
	if ( ++p == pe )
		goto _test_eof49;
case 49:
	switch( (*p) ) {
		case 82: goto st50;
		case 89: goto st51;
		case 114: goto st50;
		case 121: goto st51;
	}
	goto st0;
st50:
	if ( ++p == pe )
		goto _test_eof50;
case 50:
	switch( (*p) ) {
		case 32: goto tr127;
		case 44: goto tr128;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr127;
	goto st0;
st51:
	if ( ++p == pe )
		goto _test_eof51;
case 51:
	switch( (*p) ) {
		case 32: goto tr129;
		case 44: goto tr130;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr129;
	goto st0;
tr91:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st52;
st52:
	if ( ++p == pe )
		goto _test_eof52;
case 52:
#line 2466 "lcron.c"
	switch( (*p) ) {
		case 79: goto st53;
		case 111: goto st53;
	}
	goto st0;
st53:
	if ( ++p == pe )
		goto _test_eof53;
case 53:
	switch( (*p) ) {
		case 86: goto st54;
		case 118: goto st54;
	}
	goto st0;
st54:
	if ( ++p == pe )
		goto _test_eof54;
case 54:
	switch( (*p) ) {
		case 32: goto tr133;
		case 44: goto tr134;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr133;
	goto st0;
tr92:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st55;
st55:
	if ( ++p == pe )
		goto _test_eof55;
case 55:
#line 2502 "lcron.c"
	switch( (*p) ) {
		case 67: goto st56;
		case 99: goto st56;
	}
	goto st0;
st56:
	if ( ++p == pe )
		goto _test_eof56;
case 56:
	switch( (*p) ) {
		case 84: goto st57;
		case 116: goto st57;
	}
	goto st0;
st57:
	if ( ++p == pe )
		goto _test_eof57;
case 57:
	switch( (*p) ) {
		case 32: goto tr137;
		case 44: goto tr138;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr137;
	goto st0;
tr93:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st58;
st58:
	if ( ++p == pe )
		goto _test_eof58;
case 58:
#line 2538 "lcron.c"
	switch( (*p) ) {
		case 69: goto st59;
		case 101: goto st59;
	}
	goto st0;
st59:
	if ( ++p == pe )
		goto _test_eof59;
case 59:
	switch( (*p) ) {
		case 80: goto st60;
		case 112: goto st60;
	}
	goto st0;
st60:
	if ( ++p == pe )
		goto _test_eof60;
case 60:
	switch( (*p) ) {
		case 32: goto tr141;
		case 44: goto tr142;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr141;
	goto st0;
tr47:
#line 142 "lcron.ragel"
	{
        increment_start = 0;
    }
	goto st61;
tr82:
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st61;
tr153:
#line 242 "lcron.ragel"
	{*token_start = '4';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st61;
tr158:
#line 246 "lcron.ragel"
	{*token_start = '8';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st61;
tr164:
#line 250 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '2';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st61;
tr170:
#line 240 "lcron.ragel"
	{*token_start = '2';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st61;
tr177:
#line 239 "lcron.ragel"
	{*token_start = '1';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st61;
tr183:
#line 245 "lcron.ragel"
	{*token_start = '7';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st61;
tr187:
#line 244 "lcron.ragel"
	{*token_start = '6';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st61;
tr194:
#line 241 "lcron.ragel"
	{*token_start = '3';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st61;
tr198:
#line 243 "lcron.ragel"
	{*token_start = '5';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st61;
tr204:
#line 249 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '1';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st61;
tr210:
#line 248 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '0';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st61;
tr216:
#line 247 "lcron.ragel"
	{*token_start = '9';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st61;
st61:
	if ( ++p == pe )
		goto _test_eof61;
case 61:
#line 2676 "lcron.c"
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr143;
	goto st0;
tr143:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st62;
st62:
	if ( ++p == pe )
		goto _test_eof62;
case 62:
#line 2690 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr144;
		case 44: goto tr145;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st62;
	} else if ( (*p) >= 9 )
		goto tr144;
	goto st0;
tr36:
#line 89 "lcron.ragel"
	{
        current = months;
        current_len = MONTHS_LEN;
        current_offset = MONTHS_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st63;
tr70:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st63;
st63:
	if ( ++p == pe )
		goto _test_eof63;
case 63:
#line 2723 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr79;
		case 44: goto tr80;
		case 45: goto tr81;
		case 47: goto tr82;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr79;
	goto st0;
tr37:
#line 89 "lcron.ragel"
	{
        current = months;
        current_len = MONTHS_LEN;
        current_offset = MONTHS_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st64;
tr71:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st64;
st64:
	if ( ++p == pe )
		goto _test_eof64;
case 64:
#line 2755 "lcron.c"
	switch( (*p) ) {
		case 80: goto st65;
		case 85: goto st67;
		case 112: goto st65;
		case 117: goto st67;
	}
	goto st0;
st65:
	if ( ++p == pe )
		goto _test_eof65;
case 65:
	switch( (*p) ) {
		case 82: goto st66;
		case 114: goto st66;
	}
	goto st0;
st66:
	if ( ++p == pe )
		goto _test_eof66;
case 66:
	switch( (*p) ) {
		case 32: goto tr150;
		case 44: goto tr151;
		case 45: goto tr152;
		case 47: goto tr153;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr150;
	goto st0;
st67:
	if ( ++p == pe )
		goto _test_eof67;
case 67:
	switch( (*p) ) {
		case 71: goto st68;
		case 103: goto st68;
	}
	goto st0;
st68:
	if ( ++p == pe )
		goto _test_eof68;
case 68:
	switch( (*p) ) {
		case 32: goto tr155;
		case 44: goto tr156;
		case 45: goto tr157;
		case 47: goto tr158;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr155;
	goto st0;
tr38:
#line 89 "lcron.ragel"
	{
        current = months;
        current_len = MONTHS_LEN;
        current_offset = MONTHS_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st69;
tr72:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st69;
st69:
	if ( ++p == pe )
		goto _test_eof69;
case 69:
#line 2829 "lcron.c"
	switch( (*p) ) {
		case 69: goto st70;
		case 101: goto st70;
	}
	goto st0;
st70:
	if ( ++p == pe )
		goto _test_eof70;
case 70:
	switch( (*p) ) {
		case 67: goto st71;
		case 99: goto st71;
	}
	goto st0;
st71:
	if ( ++p == pe )
		goto _test_eof71;
case 71:
	switch( (*p) ) {
		case 32: goto tr161;
		case 44: goto tr162;
		case 45: goto tr163;
		case 47: goto tr164;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr161;
	goto st0;
tr39:
#line 89 "lcron.ragel"
	{
        current = months;
        current_len = MONTHS_LEN;
        current_offset = MONTHS_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st72;
tr73:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st72;
st72:
	if ( ++p == pe )
		goto _test_eof72;
case 72:
#line 2879 "lcron.c"
	switch( (*p) ) {
		case 69: goto st73;
		case 101: goto st73;
	}
	goto st0;
st73:
	if ( ++p == pe )
		goto _test_eof73;
case 73:
	switch( (*p) ) {
		case 66: goto st74;
		case 98: goto st74;
	}
	goto st0;
st74:
	if ( ++p == pe )
		goto _test_eof74;
case 74:
	switch( (*p) ) {
		case 32: goto tr167;
		case 44: goto tr168;
		case 45: goto tr169;
		case 47: goto tr170;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr167;
	goto st0;
tr40:
#line 89 "lcron.ragel"
	{
        current = months;
        current_len = MONTHS_LEN;
        current_offset = MONTHS_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st75;
tr74:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st75;
st75:
	if ( ++p == pe )
		goto _test_eof75;
case 75:
#line 2929 "lcron.c"
	switch( (*p) ) {
		case 65: goto st76;
		case 85: goto st78;
		case 97: goto st76;
		case 117: goto st78;
	}
	goto st0;
st76:
	if ( ++p == pe )
		goto _test_eof76;
case 76:
	switch( (*p) ) {
		case 78: goto st77;
		case 110: goto st77;
	}
	goto st0;
st77:
	if ( ++p == pe )
		goto _test_eof77;
case 77:
	switch( (*p) ) {
		case 32: goto tr174;
		case 44: goto tr175;
		case 45: goto tr176;
		case 47: goto tr177;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr174;
	goto st0;
st78:
	if ( ++p == pe )
		goto _test_eof78;
case 78:
	switch( (*p) ) {
		case 76: goto st79;
		case 78: goto st80;
		case 108: goto st79;
		case 110: goto st80;
	}
	goto st0;
st79:
	if ( ++p == pe )
		goto _test_eof79;
case 79:
	switch( (*p) ) {
		case 32: goto tr180;
		case 44: goto tr181;
		case 45: goto tr182;
		case 47: goto tr183;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr180;
	goto st0;
st80:
	if ( ++p == pe )
		goto _test_eof80;
case 80:
	switch( (*p) ) {
		case 32: goto tr184;
		case 44: goto tr185;
		case 45: goto tr186;
		case 47: goto tr187;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr184;
	goto st0;
tr41:
#line 89 "lcron.ragel"
	{
        current = months;
        current_len = MONTHS_LEN;
        current_offset = MONTHS_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st81;
tr75:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st81;
st81:
	if ( ++p == pe )
		goto _test_eof81;
case 81:
#line 3018 "lcron.c"
	switch( (*p) ) {
		case 65: goto st82;
		case 97: goto st82;
	}
	goto st0;
st82:
	if ( ++p == pe )
		goto _test_eof82;
case 82:
	switch( (*p) ) {
		case 82: goto st83;
		case 89: goto st84;
		case 114: goto st83;
		case 121: goto st84;
	}
	goto st0;
st83:
	if ( ++p == pe )
		goto _test_eof83;
case 83:
	switch( (*p) ) {
		case 32: goto tr191;
		case 44: goto tr192;
		case 45: goto tr193;
		case 47: goto tr194;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr191;
	goto st0;
st84:
	if ( ++p == pe )
		goto _test_eof84;
case 84:
	switch( (*p) ) {
		case 32: goto tr195;
		case 44: goto tr196;
		case 45: goto tr197;
		case 47: goto tr198;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr195;
	goto st0;
tr42:
#line 89 "lcron.ragel"
	{
        current = months;
        current_len = MONTHS_LEN;
        current_offset = MONTHS_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st85;
tr76:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st85;
st85:
	if ( ++p == pe )
		goto _test_eof85;
case 85:
#line 3083 "lcron.c"
	switch( (*p) ) {
		case 79: goto st86;
		case 111: goto st86;
	}
	goto st0;
st86:
	if ( ++p == pe )
		goto _test_eof86;
case 86:
	switch( (*p) ) {
		case 86: goto st87;
		case 118: goto st87;
	}
	goto st0;
st87:
	if ( ++p == pe )
		goto _test_eof87;
case 87:
	switch( (*p) ) {
		case 32: goto tr201;
		case 44: goto tr202;
		case 45: goto tr203;
		case 47: goto tr204;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr201;
	goto st0;
tr43:
#line 89 "lcron.ragel"
	{
        current = months;
        current_len = MONTHS_LEN;
        current_offset = MONTHS_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st88;
tr77:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st88;
st88:
	if ( ++p == pe )
		goto _test_eof88;
case 88:
#line 3133 "lcron.c"
	switch( (*p) ) {
		case 67: goto st89;
		case 99: goto st89;
	}
	goto st0;
st89:
	if ( ++p == pe )
		goto _test_eof89;
case 89:
	switch( (*p) ) {
		case 84: goto st90;
		case 116: goto st90;
	}
	goto st0;
st90:
	if ( ++p == pe )
		goto _test_eof90;
case 90:
	switch( (*p) ) {
		case 32: goto tr207;
		case 44: goto tr208;
		case 45: goto tr209;
		case 47: goto tr210;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr207;
	goto st0;
tr44:
#line 89 "lcron.ragel"
	{
        current = months;
        current_len = MONTHS_LEN;
        current_offset = MONTHS_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st91;
tr78:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st91;
st91:
	if ( ++p == pe )
		goto _test_eof91;
case 91:
#line 3183 "lcron.c"
	switch( (*p) ) {
		case 69: goto st92;
		case 101: goto st92;
	}
	goto st0;
st92:
	if ( ++p == pe )
		goto _test_eof92;
case 92:
	switch( (*p) ) {
		case 80: goto st93;
		case 112: goto st93;
	}
	goto st0;
st93:
	if ( ++p == pe )
		goto _test_eof93;
case 93:
	switch( (*p) ) {
		case 32: goto tr213;
		case 44: goto tr214;
		case 45: goto tr215;
		case 47: goto tr216;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr213;
	goto st0;
tr31:
#line 114 "lcron.ragel"
	{
        for (i = 0; i < current_len; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st94;
tr223:
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st94;
tr232:
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st94;
tr236:
#line 146 "lcron.ragel"
	{
        increment_interval = atoi(token_start);
    }
#line 150 "lcron.ragel"
	{
        while (increment_start < current_len) {
            current[increment_start] |= MARK_NORMAL;
            increment_start += increment_interval;
        }
    }
	goto st94;
tr239:
#line 161 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_WEEKDAY;
    }
	goto st94;
tr241:
#line 157 "lcron.ragel"
	{
        current[DAYS_OF_MONTH_LEN] |= MARK_NORMAL;
    }
	goto st94;
st94:
	if ( ++p == pe )
		goto _test_eof94;
case 94:
#line 3270 "lcron.c"
	switch( (*p) ) {
		case 42: goto st8;
		case 51: goto tr219;
		case 76: goto st105;
	}
	if ( (*p) > 50 ) {
		if ( 52 <= (*p) && (*p) <= 57 )
			goto tr220;
	} else if ( (*p) >= 49 )
		goto tr218;
	goto st0;
tr25:
#line 83 "lcron.ragel"
	{
        current = days_of_month;
        current_len = DAYS_OF_MONTH_LEN;
        current_offset = DAYS_OF_MONTH_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st95;
tr218:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st95;
st95:
	if ( ++p == pe )
		goto _test_eof95;
case 95:
#line 3304 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr222;
		case 44: goto tr223;
		case 45: goto tr224;
		case 47: goto tr225;
		case 87: goto st103;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st102;
	} else if ( (*p) >= 9 )
		goto tr222;
	goto st0;
tr224:
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st96;
st96:
	if ( ++p == pe )
		goto _test_eof96;
case 96:
#line 3328 "lcron.c"
	if ( (*p) == 51 )
		goto tr229;
	if ( (*p) > 50 ) {
		if ( 52 <= (*p) && (*p) <= 57 )
			goto tr230;
	} else if ( (*p) >= 49 )
		goto tr228;
	goto st0;
tr228:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st97;
st97:
	if ( ++p == pe )
		goto _test_eof97;
case 97:
#line 3347 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr231;
		case 44: goto tr232;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st98;
	} else if ( (*p) >= 9 )
		goto tr231;
	goto st0;
tr230:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st98;
st98:
	if ( ++p == pe )
		goto _test_eof98;
case 98:
#line 3368 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr231;
		case 44: goto tr232;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr231;
	goto st0;
tr229:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st99;
st99:
	if ( ++p == pe )
		goto _test_eof99;
case 99:
#line 3386 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr231;
		case 44: goto tr232;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 49 )
			goto st98;
	} else if ( (*p) >= 9 )
		goto tr231;
	goto st0;
tr32:
#line 142 "lcron.ragel"
	{
        increment_start = 0;
    }
	goto st100;
tr225:
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st100;
st100:
	if ( ++p == pe )
		goto _test_eof100;
case 100:
#line 3413 "lcron.c"
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr234;
	goto st0;
tr234:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st101;
st101:
	if ( ++p == pe )
		goto _test_eof101;
case 101:
#line 3427 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr235;
		case 44: goto tr236;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st101;
	} else if ( (*p) >= 9 )
		goto tr235;
	goto st0;
tr27:
#line 83 "lcron.ragel"
	{
        current = days_of_month;
        current_len = DAYS_OF_MONTH_LEN;
        current_offset = DAYS_OF_MONTH_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st102;
tr220:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st102;
st102:
	if ( ++p == pe )
		goto _test_eof102;
case 102:
#line 3460 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr222;
		case 44: goto tr223;
		case 45: goto tr224;
		case 47: goto tr225;
		case 87: goto st103;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr222;
	goto st0;
st103:
	if ( ++p == pe )
		goto _test_eof103;
case 103:
	switch( (*p) ) {
		case 32: goto tr238;
		case 44: goto tr239;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr238;
	goto st0;
tr26:
#line 83 "lcron.ragel"
	{
        current = days_of_month;
        current_len = DAYS_OF_MONTH_LEN;
        current_offset = DAYS_OF_MONTH_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st104;
tr219:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st104;
st104:
	if ( ++p == pe )
		goto _test_eof104;
case 104:
#line 3504 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr222;
		case 44: goto tr223;
		case 45: goto tr224;
		case 47: goto tr225;
		case 87: goto st103;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 49 )
			goto st102;
	} else if ( (*p) >= 9 )
		goto tr222;
	goto st0;
tr29:
#line 83 "lcron.ragel"
	{
        current = days_of_month;
        current_len = DAYS_OF_MONTH_LEN;
        current_offset = DAYS_OF_MONTH_START;
    }
	goto st105;
st105:
	if ( ++p == pe )
		goto _test_eof105;
case 105:
#line 3530 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr240;
		case 44: goto tr241;
		case 87: goto st103;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr240;
	goto st0;
tr28:
#line 83 "lcron.ragel"
	{
        current = days_of_month;
        current_len = DAYS_OF_MONTH_LEN;
        current_offset = DAYS_OF_MONTH_START;
    }
	goto st106;
st106:
	if ( ++p == pe )
		goto _test_eof106;
case 106:
#line 3551 "lcron.c"
	if ( (*p) == 32 )
		goto tr242;
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr242;
	goto st0;
tr242:
#line 114 "lcron.ragel"
	{
        for (i = 0; i < current_len; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st107;
st107:
	if ( ++p == pe )
		goto _test_eof107;
case 107:
#line 3569 "lcron.c"
	switch( (*p) ) {
		case 32: goto st107;
		case 42: goto tr244;
		case 49: goto tr245;
		case 65: goto tr247;
		case 68: goto tr248;
		case 70: goto tr249;
		case 74: goto tr250;
		case 77: goto tr251;
		case 78: goto tr252;
		case 79: goto tr253;
		case 83: goto tr254;
		case 97: goto tr247;
		case 100: goto tr248;
		case 102: goto tr249;
		case 106: goto tr250;
		case 109: goto tr251;
		case 110: goto tr252;
		case 111: goto tr253;
		case 115: goto tr254;
	}
	if ( (*p) > 13 ) {
		if ( 50 <= (*p) && (*p) <= 57 )
			goto tr246;
	} else if ( (*p) >= 9 )
		goto st107;
	goto st0;
tr244:
#line 89 "lcron.ragel"
	{
        current = months;
        current_len = MONTHS_LEN;
        current_offset = MONTHS_START;
    }
	goto st108;
st108:
	if ( ++p == pe )
		goto _test_eof108;
case 108:
#line 3609 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr255;
		case 44: goto tr256;
		case 47: goto tr257;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr255;
	goto st0;
tr255:
#line 114 "lcron.ragel"
	{
        for (i = 0; i < current_len; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st109;
tr323:
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st109;
tr338:
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st109;
tr344:
#line 242 "lcron.ragel"
	{*token_start = '4';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st109;
tr347:
#line 246 "lcron.ragel"
	{*token_start = '8';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st109;
tr351:
#line 250 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '2';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st109;
tr355:
#line 240 "lcron.ragel"
	{*token_start = '2';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st109;
tr360:
#line 239 "lcron.ragel"
	{*token_start = '1';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st109;
tr364:
#line 245 "lcron.ragel"
	{*token_start = '7';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st109;
tr366:
#line 244 "lcron.ragel"
	{*token_start = '6';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st109;
tr371:
#line 241 "lcron.ragel"
	{*token_start = '3';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st109;
tr373:
#line 243 "lcron.ragel"
	{*token_start = '5';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st109;
tr377:
#line 249 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '1';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st109;
tr381:
#line 248 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '0';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st109;
tr385:
#line 247 "lcron.ragel"
	{*token_start = '9';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st109;
tr388:
#line 146 "lcron.ragel"
	{
        increment_interval = atoi(token_start);
    }
#line 150 "lcron.ragel"
	{
        while (increment_start < current_len) {
            current[increment_start] |= MARK_NORMAL;
            increment_start += increment_interval;
        }
    }
	goto st109;
tr394:
#line 242 "lcron.ragel"
	{*token_start = '4';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st109;
tr399:
#line 246 "lcron.ragel"
	{*token_start = '8';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st109;
tr405:
#line 250 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '2';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st109;
tr411:
#line 240 "lcron.ragel"
	{*token_start = '2';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st109;
tr418:
#line 239 "lcron.ragel"
	{*token_start = '1';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st109;
tr424:
#line 245 "lcron.ragel"
	{*token_start = '7';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st109;
tr428:
#line 244 "lcron.ragel"
	{*token_start = '6';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st109;
tr435:
#line 241 "lcron.ragel"
	{*token_start = '3';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st109;
tr439:
#line 243 "lcron.ragel"
	{*token_start = '5';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st109;
tr445:
#line 249 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '1';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st109;
tr451:
#line 248 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '0';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st109;
tr457:
#line 247 "lcron.ragel"
	{*token_start = '9';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st109;
st109:
	if ( ++p == pe )
		goto _test_eof109;
case 109:
#line 3977 "lcron.c"
	switch( (*p) ) {
		case 32: goto st109;
		case 42: goto tr259;
		case 70: goto tr261;
		case 76: goto tr262;
		case 77: goto tr263;
		case 83: goto tr264;
		case 84: goto tr265;
		case 87: goto tr266;
		case 102: goto tr261;
		case 109: goto tr263;
		case 115: goto tr264;
		case 116: goto tr265;
		case 119: goto tr266;
	}
	if ( (*p) > 13 ) {
		if ( 49 <= (*p) && (*p) <= 55 )
			goto tr260;
	} else if ( (*p) >= 9 )
		goto st109;
	goto st0;
tr259:
#line 95 "lcron.ragel"
	{
        current = days_of_week;
        current_len = DAYS_OF_WEEK_LEN;
        current_offset = DAYS_OF_WEEK_START;
    }
	goto st274;
st274:
	if ( ++p == pe )
		goto _test_eof274;
case 274:
#line 4011 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr555;
		case 44: goto tr564;
		case 47: goto tr565;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr555;
	goto st0;
tr564:
#line 114 "lcron.ragel"
	{
        for (i = 0; i < current_len; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st110;
tr568:
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st110;
tr575:
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st110;
tr593:
#line 146 "lcron.ragel"
	{
        increment_interval = atoi(token_start);
    }
#line 150 "lcron.ragel"
	{
        while (increment_start < current_len) {
            current[increment_start] |= MARK_NORMAL;
            increment_start += increment_interval;
        }
    }
	goto st110;
tr573:
#line 169 "lcron.ragel"
	{
        numbered_week = atoi(token_start) - DAYS_OF_WEEK_START;
    }
#line 173 "lcron.ragel"
	{
        days_of_week[numbered_day] |= MARK_1_IN_MONTH << numbered_week;
    }
	goto st110;
tr577:
#line 271 "lcron.ragel"
	{*token_start = '6';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st110;
tr579:
#line 273 "lcron.ragel"
	{*token_start = '1';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st110;
tr581:
#line 267 "lcron.ragel"
	{*token_start = '2';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st110;
tr583:
#line 272 "lcron.ragel"
	{*token_start = '7';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st110;
tr585:
#line 266 "lcron.ragel"
	{*token_start = '1';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st110;
tr587:
#line 270 "lcron.ragel"
	{*token_start = '5';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st110;
tr589:
#line 268 "lcron.ragel"
	{*token_start = '3';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st110;
tr591:
#line 269 "lcron.ragel"
	{*token_start = '4';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st110;
tr596:
#line 177 "lcron.ragel"
	{
        days_of_week[numbered_day] |= MARK_L_IN_MONTH;
    }
	goto st110;
tr599:
#line 271 "lcron.ragel"
	{*token_start = '6';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st110;
tr605:
#line 273 "lcron.ragel"
	{*token_start = '1';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st110;
tr611:
#line 267 "lcron.ragel"
	{*token_start = '2';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st110;
tr617:
#line 272 "lcron.ragel"
	{*token_start = '7';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st110;
tr623:
#line 266 "lcron.ragel"
	{*token_start = '1';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st110;
tr629:
#line 270 "lcron.ragel"
	{*token_start = '5';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st110;
tr635:
#line 268 "lcron.ragel"
	{*token_start = '3';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st110;
tr641:
#line 269 "lcron.ragel"
	{*token_start = '4';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st110;
st110:
	if ( ++p == pe )
		goto _test_eof110;
case 110:
#line 4291 "lcron.c"
	switch( (*p) ) {
		case 42: goto st274;
		case 70: goto tr269;
		case 76: goto tr270;
		case 77: goto tr271;
		case 83: goto tr272;
		case 84: goto tr273;
		case 87: goto tr274;
		case 102: goto tr269;
		case 109: goto tr271;
		case 115: goto tr272;
		case 116: goto tr273;
		case 119: goto tr274;
	}
	if ( 49 <= (*p) && (*p) <= 55 )
		goto tr268;
	goto st0;
tr268:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st275;
tr260:
#line 95 "lcron.ragel"
	{
        current = days_of_week;
        current_len = DAYS_OF_WEEK_LEN;
        current_offset = DAYS_OF_WEEK_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st275;
st275:
	if ( ++p == pe )
		goto _test_eof275;
case 275:
#line 4331 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr566;
		case 35: goto tr567;
		case 44: goto tr568;
		case 45: goto tr569;
		case 47: goto tr570;
		case 76: goto tr571;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr566;
	goto st0;
tr567:
#line 165 "lcron.ragel"
	{
        numbered_day = atoi(token_start) - DAYS_OF_WEEK_START;
    }
	goto st111;
tr598:
#line 271 "lcron.ragel"
	{*token_start = '6';}
#line 165 "lcron.ragel"
	{
        numbered_day = atoi(token_start) - DAYS_OF_WEEK_START;
    }
	goto st111;
tr604:
#line 273 "lcron.ragel"
	{*token_start = '1';}
#line 165 "lcron.ragel"
	{
        numbered_day = atoi(token_start) - DAYS_OF_WEEK_START;
    }
	goto st111;
tr610:
#line 267 "lcron.ragel"
	{*token_start = '2';}
#line 165 "lcron.ragel"
	{
        numbered_day = atoi(token_start) - DAYS_OF_WEEK_START;
    }
	goto st111;
tr616:
#line 272 "lcron.ragel"
	{*token_start = '7';}
#line 165 "lcron.ragel"
	{
        numbered_day = atoi(token_start) - DAYS_OF_WEEK_START;
    }
	goto st111;
tr622:
#line 266 "lcron.ragel"
	{*token_start = '1';}
#line 165 "lcron.ragel"
	{
        numbered_day = atoi(token_start) - DAYS_OF_WEEK_START;
    }
	goto st111;
tr628:
#line 270 "lcron.ragel"
	{*token_start = '5';}
#line 165 "lcron.ragel"
	{
        numbered_day = atoi(token_start) - DAYS_OF_WEEK_START;
    }
	goto st111;
tr634:
#line 268 "lcron.ragel"
	{*token_start = '3';}
#line 165 "lcron.ragel"
	{
        numbered_day = atoi(token_start) - DAYS_OF_WEEK_START;
    }
	goto st111;
tr640:
#line 269 "lcron.ragel"
	{*token_start = '4';}
#line 165 "lcron.ragel"
	{
        numbered_day = atoi(token_start) - DAYS_OF_WEEK_START;
    }
	goto st111;
st111:
	if ( ++p == pe )
		goto _test_eof111;
case 111:
#line 4417 "lcron.c"
	if ( 49 <= (*p) && (*p) <= 53 )
		goto tr275;
	goto st0;
tr275:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st276;
st276:
	if ( ++p == pe )
		goto _test_eof276;
case 276:
#line 4431 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr572;
		case 44: goto tr573;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr572;
	goto st0;
tr569:
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st112;
tr600:
#line 271 "lcron.ragel"
	{*token_start = '6';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st112;
tr606:
#line 273 "lcron.ragel"
	{*token_start = '1';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st112;
tr612:
#line 267 "lcron.ragel"
	{*token_start = '2';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st112;
tr618:
#line 272 "lcron.ragel"
	{*token_start = '7';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st112;
tr624:
#line 266 "lcron.ragel"
	{*token_start = '1';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st112;
tr630:
#line 270 "lcron.ragel"
	{*token_start = '5';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st112;
tr636:
#line 268 "lcron.ragel"
	{*token_start = '3';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st112;
tr642:
#line 269 "lcron.ragel"
	{*token_start = '4';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st112;
st112:
	if ( ++p == pe )
		goto _test_eof112;
case 112:
#line 4513 "lcron.c"
	switch( (*p) ) {
		case 70: goto tr277;
		case 76: goto tr278;
		case 77: goto tr279;
		case 83: goto tr280;
		case 84: goto tr281;
		case 87: goto tr282;
		case 102: goto tr277;
		case 109: goto tr279;
		case 115: goto tr280;
		case 116: goto tr281;
		case 119: goto tr282;
	}
	if ( 49 <= (*p) && (*p) <= 55 )
		goto tr276;
	goto st0;
tr276:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st277;
st277:
	if ( ++p == pe )
		goto _test_eof277;
case 277:
#line 4540 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr574;
		case 44: goto tr575;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr574;
	goto st0;
tr277:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st113;
st113:
	if ( ++p == pe )
		goto _test_eof113;
case 113:
#line 4558 "lcron.c"
	switch( (*p) ) {
		case 82: goto st114;
		case 114: goto st114;
	}
	goto st0;
st114:
	if ( ++p == pe )
		goto _test_eof114;
case 114:
	switch( (*p) ) {
		case 73: goto st278;
		case 105: goto st278;
	}
	goto st0;
st278:
	if ( ++p == pe )
		goto _test_eof278;
case 278:
	switch( (*p) ) {
		case 32: goto tr576;
		case 44: goto tr577;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr576;
	goto st0;
tr278:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st279;
st279:
	if ( ++p == pe )
		goto _test_eof279;
case 279:
#line 4594 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr578;
		case 44: goto tr579;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr578;
	goto st0;
tr279:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st115;
st115:
	if ( ++p == pe )
		goto _test_eof115;
case 115:
#line 4612 "lcron.c"
	switch( (*p) ) {
		case 79: goto st116;
		case 111: goto st116;
	}
	goto st0;
st116:
	if ( ++p == pe )
		goto _test_eof116;
case 116:
	switch( (*p) ) {
		case 78: goto st280;
		case 110: goto st280;
	}
	goto st0;
st280:
	if ( ++p == pe )
		goto _test_eof280;
case 280:
	switch( (*p) ) {
		case 32: goto tr580;
		case 44: goto tr581;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr580;
	goto st0;
tr280:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st117;
st117:
	if ( ++p == pe )
		goto _test_eof117;
case 117:
#line 4648 "lcron.c"
	switch( (*p) ) {
		case 65: goto st118;
		case 85: goto st119;
		case 97: goto st118;
		case 117: goto st119;
	}
	goto st0;
st118:
	if ( ++p == pe )
		goto _test_eof118;
case 118:
	switch( (*p) ) {
		case 84: goto st281;
		case 116: goto st281;
	}
	goto st0;
st281:
	if ( ++p == pe )
		goto _test_eof281;
case 281:
	switch( (*p) ) {
		case 32: goto tr582;
		case 44: goto tr583;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr582;
	goto st0;
st119:
	if ( ++p == pe )
		goto _test_eof119;
case 119:
	switch( (*p) ) {
		case 78: goto st282;
		case 110: goto st282;
	}
	goto st0;
st282:
	if ( ++p == pe )
		goto _test_eof282;
case 282:
	switch( (*p) ) {
		case 32: goto tr584;
		case 44: goto tr585;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr584;
	goto st0;
tr281:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st120;
st120:
	if ( ++p == pe )
		goto _test_eof120;
case 120:
#line 4706 "lcron.c"
	switch( (*p) ) {
		case 72: goto st121;
		case 85: goto st122;
		case 104: goto st121;
		case 117: goto st122;
	}
	goto st0;
st121:
	if ( ++p == pe )
		goto _test_eof121;
case 121:
	switch( (*p) ) {
		case 85: goto st283;
		case 117: goto st283;
	}
	goto st0;
st283:
	if ( ++p == pe )
		goto _test_eof283;
case 283:
	switch( (*p) ) {
		case 32: goto tr586;
		case 44: goto tr587;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr586;
	goto st0;
st122:
	if ( ++p == pe )
		goto _test_eof122;
case 122:
	switch( (*p) ) {
		case 69: goto st284;
		case 101: goto st284;
	}
	goto st0;
st284:
	if ( ++p == pe )
		goto _test_eof284;
case 284:
	switch( (*p) ) {
		case 32: goto tr588;
		case 44: goto tr589;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr588;
	goto st0;
tr282:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st123;
st123:
	if ( ++p == pe )
		goto _test_eof123;
case 123:
#line 4764 "lcron.c"
	switch( (*p) ) {
		case 69: goto st124;
		case 101: goto st124;
	}
	goto st0;
st124:
	if ( ++p == pe )
		goto _test_eof124;
case 124:
	switch( (*p) ) {
		case 68: goto st285;
		case 100: goto st285;
	}
	goto st0;
st285:
	if ( ++p == pe )
		goto _test_eof285;
case 285:
	switch( (*p) ) {
		case 32: goto tr590;
		case 44: goto tr591;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr590;
	goto st0;
tr565:
#line 142 "lcron.ragel"
	{
        increment_start = 0;
    }
	goto st125;
tr570:
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st125;
tr601:
#line 271 "lcron.ragel"
	{*token_start = '6';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st125;
tr607:
#line 273 "lcron.ragel"
	{*token_start = '1';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st125;
tr613:
#line 267 "lcron.ragel"
	{*token_start = '2';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st125;
tr619:
#line 272 "lcron.ragel"
	{*token_start = '7';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st125;
tr625:
#line 266 "lcron.ragel"
	{*token_start = '1';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st125;
tr631:
#line 270 "lcron.ragel"
	{*token_start = '5';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st125;
tr637:
#line 268 "lcron.ragel"
	{*token_start = '3';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st125;
tr643:
#line 269 "lcron.ragel"
	{*token_start = '4';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st125;
st125:
	if ( ++p == pe )
		goto _test_eof125;
case 125:
#line 4870 "lcron.c"
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr297;
	goto st0;
tr297:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st286;
st286:
	if ( ++p == pe )
		goto _test_eof286;
case 286:
#line 4884 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr592;
		case 44: goto tr593;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st286;
	} else if ( (*p) >= 9 )
		goto tr592;
	goto st0;
tr571:
#line 165 "lcron.ragel"
	{
        numbered_day = atoi(token_start) - DAYS_OF_WEEK_START;
    }
	goto st287;
tr602:
#line 271 "lcron.ragel"
	{*token_start = '6';}
#line 165 "lcron.ragel"
	{
        numbered_day = atoi(token_start) - DAYS_OF_WEEK_START;
    }
	goto st287;
tr608:
#line 273 "lcron.ragel"
	{*token_start = '1';}
#line 165 "lcron.ragel"
	{
        numbered_day = atoi(token_start) - DAYS_OF_WEEK_START;
    }
	goto st287;
tr614:
#line 267 "lcron.ragel"
	{*token_start = '2';}
#line 165 "lcron.ragel"
	{
        numbered_day = atoi(token_start) - DAYS_OF_WEEK_START;
    }
	goto st287;
tr620:
#line 272 "lcron.ragel"
	{*token_start = '7';}
#line 165 "lcron.ragel"
	{
        numbered_day = atoi(token_start) - DAYS_OF_WEEK_START;
    }
	goto st287;
tr626:
#line 266 "lcron.ragel"
	{*token_start = '1';}
#line 165 "lcron.ragel"
	{
        numbered_day = atoi(token_start) - DAYS_OF_WEEK_START;
    }
	goto st287;
tr632:
#line 270 "lcron.ragel"
	{*token_start = '5';}
#line 165 "lcron.ragel"
	{
        numbered_day = atoi(token_start) - DAYS_OF_WEEK_START;
    }
	goto st287;
tr638:
#line 268 "lcron.ragel"
	{*token_start = '3';}
#line 165 "lcron.ragel"
	{
        numbered_day = atoi(token_start) - DAYS_OF_WEEK_START;
    }
	goto st287;
tr644:
#line 269 "lcron.ragel"
	{*token_start = '4';}
#line 165 "lcron.ragel"
	{
        numbered_day = atoi(token_start) - DAYS_OF_WEEK_START;
    }
	goto st287;
st287:
	if ( ++p == pe )
		goto _test_eof287;
case 287:
#line 4969 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr595;
		case 44: goto tr596;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr595;
	goto st0;
tr269:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st126;
tr261:
#line 95 "lcron.ragel"
	{
        current = days_of_week;
        current_len = DAYS_OF_WEEK_LEN;
        current_offset = DAYS_OF_WEEK_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st126;
st126:
	if ( ++p == pe )
		goto _test_eof126;
case 126:
#line 4999 "lcron.c"
	switch( (*p) ) {
		case 82: goto st127;
		case 114: goto st127;
	}
	goto st0;
st127:
	if ( ++p == pe )
		goto _test_eof127;
case 127:
	switch( (*p) ) {
		case 73: goto st288;
		case 105: goto st288;
	}
	goto st0;
st288:
	if ( ++p == pe )
		goto _test_eof288;
case 288:
	switch( (*p) ) {
		case 32: goto tr597;
		case 35: goto tr598;
		case 44: goto tr599;
		case 45: goto tr600;
		case 47: goto tr601;
		case 76: goto tr602;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr597;
	goto st0;
tr270:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st289;
tr262:
#line 95 "lcron.ragel"
	{
        current = days_of_week;
        current_len = DAYS_OF_WEEK_LEN;
        current_offset = DAYS_OF_WEEK_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st289;
st289:
	if ( ++p == pe )
		goto _test_eof289;
case 289:
#line 5051 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr603;
		case 35: goto tr604;
		case 44: goto tr605;
		case 45: goto tr606;
		case 47: goto tr607;
		case 76: goto tr608;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr603;
	goto st0;
tr271:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st128;
tr263:
#line 95 "lcron.ragel"
	{
        current = days_of_week;
        current_len = DAYS_OF_WEEK_LEN;
        current_offset = DAYS_OF_WEEK_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st128;
st128:
	if ( ++p == pe )
		goto _test_eof128;
case 128:
#line 5085 "lcron.c"
	switch( (*p) ) {
		case 79: goto st129;
		case 111: goto st129;
	}
	goto st0;
st129:
	if ( ++p == pe )
		goto _test_eof129;
case 129:
	switch( (*p) ) {
		case 78: goto st290;
		case 110: goto st290;
	}
	goto st0;
st290:
	if ( ++p == pe )
		goto _test_eof290;
case 290:
	switch( (*p) ) {
		case 32: goto tr609;
		case 35: goto tr610;
		case 44: goto tr611;
		case 45: goto tr612;
		case 47: goto tr613;
		case 76: goto tr614;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr609;
	goto st0;
tr272:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st130;
tr264:
#line 95 "lcron.ragel"
	{
        current = days_of_week;
        current_len = DAYS_OF_WEEK_LEN;
        current_offset = DAYS_OF_WEEK_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st130;
st130:
	if ( ++p == pe )
		goto _test_eof130;
case 130:
#line 5137 "lcron.c"
	switch( (*p) ) {
		case 65: goto st131;
		case 85: goto st132;
		case 97: goto st131;
		case 117: goto st132;
	}
	goto st0;
st131:
	if ( ++p == pe )
		goto _test_eof131;
case 131:
	switch( (*p) ) {
		case 84: goto st291;
		case 116: goto st291;
	}
	goto st0;
st291:
	if ( ++p == pe )
		goto _test_eof291;
case 291:
	switch( (*p) ) {
		case 32: goto tr615;
		case 35: goto tr616;
		case 44: goto tr617;
		case 45: goto tr618;
		case 47: goto tr619;
		case 76: goto tr620;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr615;
	goto st0;
st132:
	if ( ++p == pe )
		goto _test_eof132;
case 132:
	switch( (*p) ) {
		case 78: goto st292;
		case 110: goto st292;
	}
	goto st0;
st292:
	if ( ++p == pe )
		goto _test_eof292;
case 292:
	switch( (*p) ) {
		case 32: goto tr621;
		case 35: goto tr622;
		case 44: goto tr623;
		case 45: goto tr624;
		case 47: goto tr625;
		case 76: goto tr626;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr621;
	goto st0;
tr273:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st133;
tr265:
#line 95 "lcron.ragel"
	{
        current = days_of_week;
        current_len = DAYS_OF_WEEK_LEN;
        current_offset = DAYS_OF_WEEK_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st133;
st133:
	if ( ++p == pe )
		goto _test_eof133;
case 133:
#line 5215 "lcron.c"
	switch( (*p) ) {
		case 72: goto st134;
		case 85: goto st135;
		case 104: goto st134;
		case 117: goto st135;
	}
	goto st0;
st134:
	if ( ++p == pe )
		goto _test_eof134;
case 134:
	switch( (*p) ) {
		case 85: goto st293;
		case 117: goto st293;
	}
	goto st0;
st293:
	if ( ++p == pe )
		goto _test_eof293;
case 293:
	switch( (*p) ) {
		case 32: goto tr627;
		case 35: goto tr628;
		case 44: goto tr629;
		case 45: goto tr630;
		case 47: goto tr631;
		case 76: goto tr632;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr627;
	goto st0;
st135:
	if ( ++p == pe )
		goto _test_eof135;
case 135:
	switch( (*p) ) {
		case 69: goto st294;
		case 101: goto st294;
	}
	goto st0;
st294:
	if ( ++p == pe )
		goto _test_eof294;
case 294:
	switch( (*p) ) {
		case 32: goto tr633;
		case 35: goto tr634;
		case 44: goto tr635;
		case 45: goto tr636;
		case 47: goto tr637;
		case 76: goto tr638;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr633;
	goto st0;
tr274:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st136;
tr266:
#line 95 "lcron.ragel"
	{
        current = days_of_week;
        current_len = DAYS_OF_WEEK_LEN;
        current_offset = DAYS_OF_WEEK_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st136;
st136:
	if ( ++p == pe )
		goto _test_eof136;
case 136:
#line 5293 "lcron.c"
	switch( (*p) ) {
		case 69: goto st137;
		case 101: goto st137;
	}
	goto st0;
st137:
	if ( ++p == pe )
		goto _test_eof137;
case 137:
	switch( (*p) ) {
		case 68: goto st295;
		case 100: goto st295;
	}
	goto st0;
st295:
	if ( ++p == pe )
		goto _test_eof295;
case 295:
	switch( (*p) ) {
		case 32: goto tr639;
		case 35: goto tr640;
		case 44: goto tr641;
		case 45: goto tr642;
		case 47: goto tr643;
		case 76: goto tr644;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr639;
	goto st0;
tr256:
#line 114 "lcron.ragel"
	{
        for (i = 0; i < current_len; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st138;
tr324:
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st138;
tr339:
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st138;
tr345:
#line 242 "lcron.ragel"
	{*token_start = '4';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st138;
tr348:
#line 246 "lcron.ragel"
	{*token_start = '8';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st138;
tr352:
#line 250 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '2';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st138;
tr356:
#line 240 "lcron.ragel"
	{*token_start = '2';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st138;
tr361:
#line 239 "lcron.ragel"
	{*token_start = '1';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st138;
tr365:
#line 245 "lcron.ragel"
	{*token_start = '7';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st138;
tr367:
#line 244 "lcron.ragel"
	{*token_start = '6';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st138;
tr372:
#line 241 "lcron.ragel"
	{*token_start = '3';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st138;
tr374:
#line 243 "lcron.ragel"
	{*token_start = '5';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st138;
tr378:
#line 249 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '1';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st138;
tr382:
#line 248 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '0';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st138;
tr386:
#line 247 "lcron.ragel"
	{*token_start = '9';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st138;
tr389:
#line 146 "lcron.ragel"
	{
        increment_interval = atoi(token_start);
    }
#line 150 "lcron.ragel"
	{
        while (increment_start < current_len) {
            current[increment_start] |= MARK_NORMAL;
            increment_start += increment_interval;
        }
    }
	goto st138;
tr395:
#line 242 "lcron.ragel"
	{*token_start = '4';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st138;
tr400:
#line 246 "lcron.ragel"
	{*token_start = '8';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st138;
tr406:
#line 250 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '2';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st138;
tr412:
#line 240 "lcron.ragel"
	{*token_start = '2';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st138;
tr419:
#line 239 "lcron.ragel"
	{*token_start = '1';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st138;
tr425:
#line 245 "lcron.ragel"
	{*token_start = '7';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st138;
tr429:
#line 244 "lcron.ragel"
	{*token_start = '6';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st138;
tr436:
#line 241 "lcron.ragel"
	{*token_start = '3';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st138;
tr440:
#line 243 "lcron.ragel"
	{*token_start = '5';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st138;
tr446:
#line 249 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '1';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st138;
tr452:
#line 248 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '0';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st138;
tr458:
#line 247 "lcron.ragel"
	{*token_start = '9';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st138;
st138:
	if ( ++p == pe )
		goto _test_eof138;
case 138:
#line 5682 "lcron.c"
	switch( (*p) ) {
		case 42: goto st108;
		case 49: goto tr313;
		case 65: goto tr315;
		case 68: goto tr316;
		case 70: goto tr317;
		case 74: goto tr318;
		case 77: goto tr319;
		case 78: goto tr320;
		case 79: goto tr321;
		case 83: goto tr322;
		case 97: goto tr315;
		case 100: goto tr316;
		case 102: goto tr317;
		case 106: goto tr318;
		case 109: goto tr319;
		case 110: goto tr320;
		case 111: goto tr321;
		case 115: goto tr322;
	}
	if ( 50 <= (*p) && (*p) <= 57 )
		goto tr314;
	goto st0;
tr245:
#line 89 "lcron.ragel"
	{
        current = months;
        current_len = MONTHS_LEN;
        current_offset = MONTHS_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st139;
tr313:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st139;
st139:
	if ( ++p == pe )
		goto _test_eof139;
case 139:
#line 5728 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr323;
		case 44: goto tr324;
		case 45: goto tr325;
		case 47: goto tr326;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 50 )
			goto st175;
	} else if ( (*p) >= 9 )
		goto tr323;
	goto st0;
tr325:
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st140;
tr396:
#line 242 "lcron.ragel"
	{*token_start = '4';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st140;
tr401:
#line 246 "lcron.ragel"
	{*token_start = '8';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st140;
tr407:
#line 250 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '2';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st140;
tr413:
#line 240 "lcron.ragel"
	{*token_start = '2';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st140;
tr420:
#line 239 "lcron.ragel"
	{*token_start = '1';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st140;
tr426:
#line 245 "lcron.ragel"
	{*token_start = '7';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st140;
tr430:
#line 244 "lcron.ragel"
	{*token_start = '6';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st140;
tr437:
#line 241 "lcron.ragel"
	{*token_start = '3';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st140;
tr441:
#line 243 "lcron.ragel"
	{*token_start = '5';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st140;
tr447:
#line 249 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '1';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st140;
tr453:
#line 248 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '0';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st140;
tr459:
#line 247 "lcron.ragel"
	{*token_start = '9';}
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st140;
st140:
	if ( ++p == pe )
		goto _test_eof140;
case 140:
#line 5847 "lcron.c"
	switch( (*p) ) {
		case 49: goto tr328;
		case 65: goto tr330;
		case 68: goto tr331;
		case 70: goto tr332;
		case 74: goto tr333;
		case 77: goto tr334;
		case 78: goto tr335;
		case 79: goto tr336;
		case 83: goto tr337;
		case 97: goto tr330;
		case 100: goto tr331;
		case 102: goto tr332;
		case 106: goto tr333;
		case 109: goto tr334;
		case 110: goto tr335;
		case 111: goto tr336;
		case 115: goto tr337;
	}
	if ( 50 <= (*p) && (*p) <= 57 )
		goto tr329;
	goto st0;
tr328:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st141;
st141:
	if ( ++p == pe )
		goto _test_eof141;
case 141:
#line 5880 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr338;
		case 44: goto tr339;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 50 )
			goto st142;
	} else if ( (*p) >= 9 )
		goto tr338;
	goto st0;
tr329:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st142;
st142:
	if ( ++p == pe )
		goto _test_eof142;
case 142:
#line 5901 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr338;
		case 44: goto tr339;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr338;
	goto st0;
tr330:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st143;
st143:
	if ( ++p == pe )
		goto _test_eof143;
case 143:
#line 5919 "lcron.c"
	switch( (*p) ) {
		case 80: goto st144;
		case 85: goto st146;
		case 112: goto st144;
		case 117: goto st146;
	}
	goto st0;
st144:
	if ( ++p == pe )
		goto _test_eof144;
case 144:
	switch( (*p) ) {
		case 82: goto st145;
		case 114: goto st145;
	}
	goto st0;
st145:
	if ( ++p == pe )
		goto _test_eof145;
case 145:
	switch( (*p) ) {
		case 32: goto tr344;
		case 44: goto tr345;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr344;
	goto st0;
st146:
	if ( ++p == pe )
		goto _test_eof146;
case 146:
	switch( (*p) ) {
		case 71: goto st147;
		case 103: goto st147;
	}
	goto st0;
st147:
	if ( ++p == pe )
		goto _test_eof147;
case 147:
	switch( (*p) ) {
		case 32: goto tr347;
		case 44: goto tr348;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr347;
	goto st0;
tr331:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st148;
st148:
	if ( ++p == pe )
		goto _test_eof148;
case 148:
#line 5977 "lcron.c"
	switch( (*p) ) {
		case 69: goto st149;
		case 101: goto st149;
	}
	goto st0;
st149:
	if ( ++p == pe )
		goto _test_eof149;
case 149:
	switch( (*p) ) {
		case 67: goto st150;
		case 99: goto st150;
	}
	goto st0;
st150:
	if ( ++p == pe )
		goto _test_eof150;
case 150:
	switch( (*p) ) {
		case 32: goto tr351;
		case 44: goto tr352;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr351;
	goto st0;
tr332:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st151;
st151:
	if ( ++p == pe )
		goto _test_eof151;
case 151:
#line 6013 "lcron.c"
	switch( (*p) ) {
		case 69: goto st152;
		case 101: goto st152;
	}
	goto st0;
st152:
	if ( ++p == pe )
		goto _test_eof152;
case 152:
	switch( (*p) ) {
		case 66: goto st153;
		case 98: goto st153;
	}
	goto st0;
st153:
	if ( ++p == pe )
		goto _test_eof153;
case 153:
	switch( (*p) ) {
		case 32: goto tr355;
		case 44: goto tr356;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr355;
	goto st0;
tr333:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st154;
st154:
	if ( ++p == pe )
		goto _test_eof154;
case 154:
#line 6049 "lcron.c"
	switch( (*p) ) {
		case 65: goto st155;
		case 85: goto st157;
		case 97: goto st155;
		case 117: goto st157;
	}
	goto st0;
st155:
	if ( ++p == pe )
		goto _test_eof155;
case 155:
	switch( (*p) ) {
		case 78: goto st156;
		case 110: goto st156;
	}
	goto st0;
st156:
	if ( ++p == pe )
		goto _test_eof156;
case 156:
	switch( (*p) ) {
		case 32: goto tr360;
		case 44: goto tr361;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr360;
	goto st0;
st157:
	if ( ++p == pe )
		goto _test_eof157;
case 157:
	switch( (*p) ) {
		case 76: goto st158;
		case 78: goto st159;
		case 108: goto st158;
		case 110: goto st159;
	}
	goto st0;
st158:
	if ( ++p == pe )
		goto _test_eof158;
case 158:
	switch( (*p) ) {
		case 32: goto tr364;
		case 44: goto tr365;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr364;
	goto st0;
st159:
	if ( ++p == pe )
		goto _test_eof159;
case 159:
	switch( (*p) ) {
		case 32: goto tr366;
		case 44: goto tr367;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr366;
	goto st0;
tr334:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st160;
st160:
	if ( ++p == pe )
		goto _test_eof160;
case 160:
#line 6120 "lcron.c"
	switch( (*p) ) {
		case 65: goto st161;
		case 97: goto st161;
	}
	goto st0;
st161:
	if ( ++p == pe )
		goto _test_eof161;
case 161:
	switch( (*p) ) {
		case 82: goto st162;
		case 89: goto st163;
		case 114: goto st162;
		case 121: goto st163;
	}
	goto st0;
st162:
	if ( ++p == pe )
		goto _test_eof162;
case 162:
	switch( (*p) ) {
		case 32: goto tr371;
		case 44: goto tr372;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr371;
	goto st0;
st163:
	if ( ++p == pe )
		goto _test_eof163;
case 163:
	switch( (*p) ) {
		case 32: goto tr373;
		case 44: goto tr374;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr373;
	goto st0;
tr335:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st164;
st164:
	if ( ++p == pe )
		goto _test_eof164;
case 164:
#line 6169 "lcron.c"
	switch( (*p) ) {
		case 79: goto st165;
		case 111: goto st165;
	}
	goto st0;
st165:
	if ( ++p == pe )
		goto _test_eof165;
case 165:
	switch( (*p) ) {
		case 86: goto st166;
		case 118: goto st166;
	}
	goto st0;
st166:
	if ( ++p == pe )
		goto _test_eof166;
case 166:
	switch( (*p) ) {
		case 32: goto tr377;
		case 44: goto tr378;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr377;
	goto st0;
tr336:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st167;
st167:
	if ( ++p == pe )
		goto _test_eof167;
case 167:
#line 6205 "lcron.c"
	switch( (*p) ) {
		case 67: goto st168;
		case 99: goto st168;
	}
	goto st0;
st168:
	if ( ++p == pe )
		goto _test_eof168;
case 168:
	switch( (*p) ) {
		case 84: goto st169;
		case 116: goto st169;
	}
	goto st0;
st169:
	if ( ++p == pe )
		goto _test_eof169;
case 169:
	switch( (*p) ) {
		case 32: goto tr381;
		case 44: goto tr382;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr381;
	goto st0;
tr337:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st170;
st170:
	if ( ++p == pe )
		goto _test_eof170;
case 170:
#line 6241 "lcron.c"
	switch( (*p) ) {
		case 69: goto st171;
		case 101: goto st171;
	}
	goto st0;
st171:
	if ( ++p == pe )
		goto _test_eof171;
case 171:
	switch( (*p) ) {
		case 80: goto st172;
		case 112: goto st172;
	}
	goto st0;
st172:
	if ( ++p == pe )
		goto _test_eof172;
case 172:
	switch( (*p) ) {
		case 32: goto tr385;
		case 44: goto tr386;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr385;
	goto st0;
tr257:
#line 142 "lcron.ragel"
	{
        increment_start = 0;
    }
	goto st173;
tr326:
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st173;
tr397:
#line 242 "lcron.ragel"
	{*token_start = '4';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st173;
tr402:
#line 246 "lcron.ragel"
	{*token_start = '8';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st173;
tr408:
#line 250 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '2';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st173;
tr414:
#line 240 "lcron.ragel"
	{*token_start = '2';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st173;
tr421:
#line 239 "lcron.ragel"
	{*token_start = '1';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st173;
tr427:
#line 245 "lcron.ragel"
	{*token_start = '7';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st173;
tr431:
#line 244 "lcron.ragel"
	{*token_start = '6';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st173;
tr438:
#line 241 "lcron.ragel"
	{*token_start = '3';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st173;
tr442:
#line 243 "lcron.ragel"
	{*token_start = '5';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st173;
tr448:
#line 249 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '1';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st173;
tr454:
#line 248 "lcron.ragel"
	{*token_start = '1'; *(token_start + 1) = '0';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st173;
tr460:
#line 247 "lcron.ragel"
	{*token_start = '9';}
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st173;
st173:
	if ( ++p == pe )
		goto _test_eof173;
case 173:
#line 6379 "lcron.c"
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr387;
	goto st0;
tr387:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st174;
st174:
	if ( ++p == pe )
		goto _test_eof174;
case 174:
#line 6393 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr388;
		case 44: goto tr389;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st174;
	} else if ( (*p) >= 9 )
		goto tr388;
	goto st0;
tr246:
#line 89 "lcron.ragel"
	{
        current = months;
        current_len = MONTHS_LEN;
        current_offset = MONTHS_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st175;
tr314:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st175;
st175:
	if ( ++p == pe )
		goto _test_eof175;
case 175:
#line 6426 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr323;
		case 44: goto tr324;
		case 45: goto tr325;
		case 47: goto tr326;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr323;
	goto st0;
tr247:
#line 89 "lcron.ragel"
	{
        current = months;
        current_len = MONTHS_LEN;
        current_offset = MONTHS_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st176;
tr315:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st176;
st176:
	if ( ++p == pe )
		goto _test_eof176;
case 176:
#line 6458 "lcron.c"
	switch( (*p) ) {
		case 80: goto st177;
		case 85: goto st179;
		case 112: goto st177;
		case 117: goto st179;
	}
	goto st0;
st177:
	if ( ++p == pe )
		goto _test_eof177;
case 177:
	switch( (*p) ) {
		case 82: goto st178;
		case 114: goto st178;
	}
	goto st0;
st178:
	if ( ++p == pe )
		goto _test_eof178;
case 178:
	switch( (*p) ) {
		case 32: goto tr394;
		case 44: goto tr395;
		case 45: goto tr396;
		case 47: goto tr397;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr394;
	goto st0;
st179:
	if ( ++p == pe )
		goto _test_eof179;
case 179:
	switch( (*p) ) {
		case 71: goto st180;
		case 103: goto st180;
	}
	goto st0;
st180:
	if ( ++p == pe )
		goto _test_eof180;
case 180:
	switch( (*p) ) {
		case 32: goto tr399;
		case 44: goto tr400;
		case 45: goto tr401;
		case 47: goto tr402;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr399;
	goto st0;
tr248:
#line 89 "lcron.ragel"
	{
        current = months;
        current_len = MONTHS_LEN;
        current_offset = MONTHS_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st181;
tr316:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st181;
st181:
	if ( ++p == pe )
		goto _test_eof181;
case 181:
#line 6532 "lcron.c"
	switch( (*p) ) {
		case 69: goto st182;
		case 101: goto st182;
	}
	goto st0;
st182:
	if ( ++p == pe )
		goto _test_eof182;
case 182:
	switch( (*p) ) {
		case 67: goto st183;
		case 99: goto st183;
	}
	goto st0;
st183:
	if ( ++p == pe )
		goto _test_eof183;
case 183:
	switch( (*p) ) {
		case 32: goto tr405;
		case 44: goto tr406;
		case 45: goto tr407;
		case 47: goto tr408;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr405;
	goto st0;
tr249:
#line 89 "lcron.ragel"
	{
        current = months;
        current_len = MONTHS_LEN;
        current_offset = MONTHS_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st184;
tr317:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st184;
st184:
	if ( ++p == pe )
		goto _test_eof184;
case 184:
#line 6582 "lcron.c"
	switch( (*p) ) {
		case 69: goto st185;
		case 101: goto st185;
	}
	goto st0;
st185:
	if ( ++p == pe )
		goto _test_eof185;
case 185:
	switch( (*p) ) {
		case 66: goto st186;
		case 98: goto st186;
	}
	goto st0;
st186:
	if ( ++p == pe )
		goto _test_eof186;
case 186:
	switch( (*p) ) {
		case 32: goto tr411;
		case 44: goto tr412;
		case 45: goto tr413;
		case 47: goto tr414;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr411;
	goto st0;
tr250:
#line 89 "lcron.ragel"
	{
        current = months;
        current_len = MONTHS_LEN;
        current_offset = MONTHS_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st187;
tr318:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st187;
st187:
	if ( ++p == pe )
		goto _test_eof187;
case 187:
#line 6632 "lcron.c"
	switch( (*p) ) {
		case 65: goto st188;
		case 85: goto st190;
		case 97: goto st188;
		case 117: goto st190;
	}
	goto st0;
st188:
	if ( ++p == pe )
		goto _test_eof188;
case 188:
	switch( (*p) ) {
		case 78: goto st189;
		case 110: goto st189;
	}
	goto st0;
st189:
	if ( ++p == pe )
		goto _test_eof189;
case 189:
	switch( (*p) ) {
		case 32: goto tr418;
		case 44: goto tr419;
		case 45: goto tr420;
		case 47: goto tr421;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr418;
	goto st0;
st190:
	if ( ++p == pe )
		goto _test_eof190;
case 190:
	switch( (*p) ) {
		case 76: goto st191;
		case 78: goto st192;
		case 108: goto st191;
		case 110: goto st192;
	}
	goto st0;
st191:
	if ( ++p == pe )
		goto _test_eof191;
case 191:
	switch( (*p) ) {
		case 32: goto tr424;
		case 44: goto tr425;
		case 45: goto tr426;
		case 47: goto tr427;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr424;
	goto st0;
st192:
	if ( ++p == pe )
		goto _test_eof192;
case 192:
	switch( (*p) ) {
		case 32: goto tr428;
		case 44: goto tr429;
		case 45: goto tr430;
		case 47: goto tr431;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr428;
	goto st0;
tr251:
#line 89 "lcron.ragel"
	{
        current = months;
        current_len = MONTHS_LEN;
        current_offset = MONTHS_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st193;
tr319:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st193;
st193:
	if ( ++p == pe )
		goto _test_eof193;
case 193:
#line 6721 "lcron.c"
	switch( (*p) ) {
		case 65: goto st194;
		case 97: goto st194;
	}
	goto st0;
st194:
	if ( ++p == pe )
		goto _test_eof194;
case 194:
	switch( (*p) ) {
		case 82: goto st195;
		case 89: goto st196;
		case 114: goto st195;
		case 121: goto st196;
	}
	goto st0;
st195:
	if ( ++p == pe )
		goto _test_eof195;
case 195:
	switch( (*p) ) {
		case 32: goto tr435;
		case 44: goto tr436;
		case 45: goto tr437;
		case 47: goto tr438;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr435;
	goto st0;
st196:
	if ( ++p == pe )
		goto _test_eof196;
case 196:
	switch( (*p) ) {
		case 32: goto tr439;
		case 44: goto tr440;
		case 45: goto tr441;
		case 47: goto tr442;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr439;
	goto st0;
tr252:
#line 89 "lcron.ragel"
	{
        current = months;
        current_len = MONTHS_LEN;
        current_offset = MONTHS_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st197;
tr320:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st197;
st197:
	if ( ++p == pe )
		goto _test_eof197;
case 197:
#line 6786 "lcron.c"
	switch( (*p) ) {
		case 79: goto st198;
		case 111: goto st198;
	}
	goto st0;
st198:
	if ( ++p == pe )
		goto _test_eof198;
case 198:
	switch( (*p) ) {
		case 86: goto st199;
		case 118: goto st199;
	}
	goto st0;
st199:
	if ( ++p == pe )
		goto _test_eof199;
case 199:
	switch( (*p) ) {
		case 32: goto tr445;
		case 44: goto tr446;
		case 45: goto tr447;
		case 47: goto tr448;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr445;
	goto st0;
tr253:
#line 89 "lcron.ragel"
	{
        current = months;
        current_len = MONTHS_LEN;
        current_offset = MONTHS_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st200;
tr321:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st200;
st200:
	if ( ++p == pe )
		goto _test_eof200;
case 200:
#line 6836 "lcron.c"
	switch( (*p) ) {
		case 67: goto st201;
		case 99: goto st201;
	}
	goto st0;
st201:
	if ( ++p == pe )
		goto _test_eof201;
case 201:
	switch( (*p) ) {
		case 84: goto st202;
		case 116: goto st202;
	}
	goto st0;
st202:
	if ( ++p == pe )
		goto _test_eof202;
case 202:
	switch( (*p) ) {
		case 32: goto tr451;
		case 44: goto tr452;
		case 45: goto tr453;
		case 47: goto tr454;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr451;
	goto st0;
tr254:
#line 89 "lcron.ragel"
	{
        current = months;
        current_len = MONTHS_LEN;
        current_offset = MONTHS_START;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st203;
tr322:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st203;
st203:
	if ( ++p == pe )
		goto _test_eof203;
case 203:
#line 6886 "lcron.c"
	switch( (*p) ) {
		case 69: goto st204;
		case 101: goto st204;
	}
	goto st0;
st204:
	if ( ++p == pe )
		goto _test_eof204;
case 204:
	switch( (*p) ) {
		case 80: goto st205;
		case 112: goto st205;
	}
	goto st0;
st205:
	if ( ++p == pe )
		goto _test_eof205;
case 205:
	switch( (*p) ) {
		case 32: goto tr457;
		case 44: goto tr458;
		case 45: goto tr459;
		case 47: goto tr460;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr457;
	goto st0;
tr21:
#line 114 "lcron.ragel"
	{
        for (i = 0; i < current_len; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st206;
tr466:
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st206;
tr473:
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st206;
tr477:
#line 146 "lcron.ragel"
	{
        increment_interval = atoi(token_start);
    }
#line 150 "lcron.ragel"
	{
        while (increment_start < current_len) {
            current[increment_start] |= MARK_NORMAL;
            increment_start += increment_interval;
        }
    }
	goto st206;
st206:
	if ( ++p == pe )
		goto _test_eof206;
case 206:
#line 6961 "lcron.c"
	switch( (*p) ) {
		case 42: goto st6;
		case 49: goto tr463;
		case 50: goto tr464;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr462;
	goto st0;
tr17:
#line 77 "lcron.ragel"
	{
        current = hours;
        current_len = HOURS_LEN;
        current_offset = 0;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st207;
tr462:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st207;
st207:
	if ( ++p == pe )
		goto _test_eof207;
case 207:
#line 6992 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr465;
		case 44: goto tr466;
		case 45: goto tr467;
		case 47: goto tr468;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr465;
	goto st0;
tr467:
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st208;
st208:
	if ( ++p == pe )
		goto _test_eof208;
case 208:
#line 7012 "lcron.c"
	switch( (*p) ) {
		case 49: goto tr470;
		case 50: goto tr471;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr469;
	goto st0;
tr469:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st209;
st209:
	if ( ++p == pe )
		goto _test_eof209;
case 209:
#line 7030 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr472;
		case 44: goto tr473;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr472;
	goto st0;
tr470:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st210;
st210:
	if ( ++p == pe )
		goto _test_eof210;
case 210:
#line 7048 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr472;
		case 44: goto tr473;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st209;
	} else if ( (*p) >= 9 )
		goto tr472;
	goto st0;
tr471:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st211;
st211:
	if ( ++p == pe )
		goto _test_eof211;
case 211:
#line 7069 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr472;
		case 44: goto tr473;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 51 )
			goto st209;
	} else if ( (*p) >= 9 )
		goto tr472;
	goto st0;
tr22:
#line 142 "lcron.ragel"
	{
        increment_start = 0;
    }
	goto st212;
tr468:
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st212;
st212:
	if ( ++p == pe )
		goto _test_eof212;
case 212:
#line 7096 "lcron.c"
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr475;
	goto st0;
tr475:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st213;
st213:
	if ( ++p == pe )
		goto _test_eof213;
case 213:
#line 7110 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr476;
		case 44: goto tr477;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st213;
	} else if ( (*p) >= 9 )
		goto tr476;
	goto st0;
tr18:
#line 77 "lcron.ragel"
	{
        current = hours;
        current_len = HOURS_LEN;
        current_offset = 0;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st214;
tr463:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st214;
st214:
	if ( ++p == pe )
		goto _test_eof214;
case 214:
#line 7143 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr465;
		case 44: goto tr466;
		case 45: goto tr467;
		case 47: goto tr468;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st207;
	} else if ( (*p) >= 9 )
		goto tr465;
	goto st0;
tr19:
#line 77 "lcron.ragel"
	{
        current = hours;
        current_len = HOURS_LEN;
        current_offset = 0;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st215;
tr464:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st215;
st215:
	if ( ++p == pe )
		goto _test_eof215;
case 215:
#line 7178 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr465;
		case 44: goto tr466;
		case 45: goto tr467;
		case 47: goto tr468;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 51 )
			goto st207;
	} else if ( (*p) >= 9 )
		goto tr465;
	goto st0;
tr13:
#line 114 "lcron.ragel"
	{
        for (i = 0; i < current_len; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st216;
tr484:
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st216;
tr490:
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st216;
tr494:
#line 146 "lcron.ragel"
	{
        increment_interval = atoi(token_start);
    }
#line 150 "lcron.ragel"
	{
        while (increment_start < current_len) {
            current[increment_start] |= MARK_NORMAL;
            increment_start += increment_interval;
        }
    }
	goto st216;
st216:
	if ( ++p == pe )
		goto _test_eof216;
case 216:
#line 7238 "lcron.c"
	switch( (*p) ) {
		case 42: goto st4;
		case 48: goto tr481;
	}
	if ( (*p) > 53 ) {
		if ( 54 <= (*p) && (*p) <= 57 )
			goto tr481;
	} else if ( (*p) >= 49 )
		goto tr482;
	goto st0;
tr10:
#line 71 "lcron.ragel"
	{
        current = minutes;
        current_len = MINUTES_LEN;
        current_offset = 0;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st217;
tr481:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st217;
st217:
	if ( ++p == pe )
		goto _test_eof217;
case 217:
#line 7271 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr483;
		case 44: goto tr484;
		case 45: goto tr485;
		case 47: goto tr486;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr483;
	goto st0;
tr485:
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st218;
st218:
	if ( ++p == pe )
		goto _test_eof218;
case 218:
#line 7291 "lcron.c"
	if ( (*p) == 48 )
		goto tr487;
	if ( (*p) > 53 ) {
		if ( 54 <= (*p) && (*p) <= 57 )
			goto tr487;
	} else if ( (*p) >= 49 )
		goto tr488;
	goto st0;
tr487:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st219;
st219:
	if ( ++p == pe )
		goto _test_eof219;
case 219:
#line 7310 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr489;
		case 44: goto tr490;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr489;
	goto st0;
tr488:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st220;
st220:
	if ( ++p == pe )
		goto _test_eof220;
case 220:
#line 7328 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr489;
		case 44: goto tr490;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st219;
	} else if ( (*p) >= 9 )
		goto tr489;
	goto st0;
tr14:
#line 142 "lcron.ragel"
	{
        increment_start = 0;
    }
	goto st221;
tr486:
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st221;
st221:
	if ( ++p == pe )
		goto _test_eof221;
case 221:
#line 7355 "lcron.c"
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr492;
	goto st0;
tr492:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st222;
st222:
	if ( ++p == pe )
		goto _test_eof222;
case 222:
#line 7369 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr493;
		case 44: goto tr494;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st222;
	} else if ( (*p) >= 9 )
		goto tr493;
	goto st0;
tr11:
#line 71 "lcron.ragel"
	{
        current = minutes;
        current_len = MINUTES_LEN;
        current_offset = 0;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st223;
tr482:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st223;
st223:
	if ( ++p == pe )
		goto _test_eof223;
case 223:
#line 7402 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr483;
		case 44: goto tr484;
		case 45: goto tr485;
		case 47: goto tr486;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st217;
	} else if ( (*p) >= 9 )
		goto tr483;
	goto st0;
tr6:
#line 114 "lcron.ragel"
	{
        for (i = 0; i < current_len; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st224;
tr501:
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
	goto st224;
tr507:
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
	goto st224;
tr511:
#line 146 "lcron.ragel"
	{
        increment_interval = atoi(token_start);
    }
#line 150 "lcron.ragel"
	{
        while (increment_start < current_len) {
            current[increment_start] |= MARK_NORMAL;
            increment_start += increment_interval;
        }
    }
	goto st224;
st224:
	if ( ++p == pe )
		goto _test_eof224;
case 224:
#line 7462 "lcron.c"
	switch( (*p) ) {
		case 42: goto st2;
		case 48: goto tr498;
	}
	if ( (*p) > 53 ) {
		if ( 54 <= (*p) && (*p) <= 57 )
			goto tr498;
	} else if ( (*p) >= 49 )
		goto tr499;
	goto st0;
tr2:
#line 65 "lcron.ragel"
	{
        current = seconds;
        current_len = SECONDS_LEN;
        current_offset = 0;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st225;
tr498:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st225;
st225:
	if ( ++p == pe )
		goto _test_eof225;
case 225:
#line 7495 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr500;
		case 44: goto tr501;
		case 45: goto tr502;
		case 47: goto tr503;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr500;
	goto st0;
tr502:
#line 120 "lcron.ragel"
	{
        range_start = atoi(token_start) - current_offset;
    }
	goto st226;
st226:
	if ( ++p == pe )
		goto _test_eof226;
case 226:
#line 7515 "lcron.c"
	if ( (*p) == 48 )
		goto tr504;
	if ( (*p) > 53 ) {
		if ( 54 <= (*p) && (*p) <= 57 )
			goto tr504;
	} else if ( (*p) >= 49 )
		goto tr505;
	goto st0;
tr504:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st227;
st227:
	if ( ++p == pe )
		goto _test_eof227;
case 227:
#line 7534 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr506;
		case 44: goto tr507;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr506;
	goto st0;
tr505:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st228;
st228:
	if ( ++p == pe )
		goto _test_eof228;
case 228:
#line 7552 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr506;
		case 44: goto tr507;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st227;
	} else if ( (*p) >= 9 )
		goto tr506;
	goto st0;
tr7:
#line 142 "lcron.ragel"
	{
        increment_start = 0;
    }
	goto st229;
tr503:
#line 138 "lcron.ragel"
	{
        increment_start = atoi(token_start) - current_offset;
    }
	goto st229;
st229:
	if ( ++p == pe )
		goto _test_eof229;
case 229:
#line 7579 "lcron.c"
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr509;
	goto st0;
tr509:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st230;
st230:
	if ( ++p == pe )
		goto _test_eof230;
case 230:
#line 7593 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr510;
		case 44: goto tr511;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st230;
	} else if ( (*p) >= 9 )
		goto tr510;
	goto st0;
tr3:
#line 65 "lcron.ragel"
	{
        current = seconds;
        current_len = SECONDS_LEN;
        current_offset = 0;
    }
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st231;
tr499:
#line 61 "lcron.ragel"
	{
        token_start = p;
    }
	goto st231;
st231:
	if ( ++p == pe )
		goto _test_eof231;
case 231:
#line 7626 "lcron.c"
	switch( (*p) ) {
		case 32: goto tr500;
		case 44: goto tr501;
		case 45: goto tr502;
		case 47: goto tr503;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st225;
	} else if ( (*p) >= 9 )
		goto tr500;
	goto st0;
st232:
	if ( ++p == pe )
		goto _test_eof232;
case 232:
	switch( (*p) ) {
		case 97: goto st233;
		case 100: goto st240;
		case 104: goto st244;
		case 109: goto st249;
		case 119: goto st261;
		case 121: goto st266;
	}
	goto st0;
st233:
	if ( ++p == pe )
		goto _test_eof233;
case 233:
	if ( (*p) == 110 )
		goto st234;
	goto st0;
st234:
	if ( ++p == pe )
		goto _test_eof234;
case 234:
	if ( (*p) == 110 )
		goto st235;
	goto st0;
st235:
	if ( ++p == pe )
		goto _test_eof235;
case 235:
	if ( (*p) == 117 )
		goto st236;
	goto st0;
st236:
	if ( ++p == pe )
		goto _test_eof236;
case 236:
	if ( (*p) == 97 )
		goto st237;
	goto st0;
st237:
	if ( ++p == pe )
		goto _test_eof237;
case 237:
	if ( (*p) == 108 )
		goto st238;
	goto st0;
st238:
	if ( ++p == pe )
		goto _test_eof238;
case 238:
	if ( (*p) == 108 )
		goto st239;
	goto st0;
st239:
	if ( ++p == pe )
		goto _test_eof239;
case 239:
	if ( (*p) == 121 )
		goto st296;
	goto st0;
st296:
	if ( ++p == pe )
		goto _test_eof296;
case 296:
	goto st0;
st240:
	if ( ++p == pe )
		goto _test_eof240;
case 240:
	if ( (*p) == 97 )
		goto st241;
	goto st0;
st241:
	if ( ++p == pe )
		goto _test_eof241;
case 241:
	if ( (*p) == 105 )
		goto st242;
	goto st0;
st242:
	if ( ++p == pe )
		goto _test_eof242;
case 242:
	if ( (*p) == 108 )
		goto st243;
	goto st0;
st243:
	if ( ++p == pe )
		goto _test_eof243;
case 243:
	if ( (*p) == 121 )
		goto st297;
	goto st0;
st297:
	if ( ++p == pe )
		goto _test_eof297;
case 297:
	goto st0;
st244:
	if ( ++p == pe )
		goto _test_eof244;
case 244:
	if ( (*p) == 111 )
		goto st245;
	goto st0;
st245:
	if ( ++p == pe )
		goto _test_eof245;
case 245:
	if ( (*p) == 117 )
		goto st246;
	goto st0;
st246:
	if ( ++p == pe )
		goto _test_eof246;
case 246:
	if ( (*p) == 114 )
		goto st247;
	goto st0;
st247:
	if ( ++p == pe )
		goto _test_eof247;
case 247:
	if ( (*p) == 108 )
		goto st248;
	goto st0;
st248:
	if ( ++p == pe )
		goto _test_eof248;
case 248:
	if ( (*p) == 121 )
		goto st298;
	goto st0;
st298:
	if ( ++p == pe )
		goto _test_eof298;
case 298:
	goto st0;
st249:
	if ( ++p == pe )
		goto _test_eof249;
case 249:
	switch( (*p) ) {
		case 105: goto st250;
		case 111: goto st256;
	}
	goto st0;
st250:
	if ( ++p == pe )
		goto _test_eof250;
case 250:
	if ( (*p) == 100 )
		goto st251;
	goto st0;
st251:
	if ( ++p == pe )
		goto _test_eof251;
case 251:
	if ( (*p) == 110 )
		goto st252;
	goto st0;
st252:
	if ( ++p == pe )
		goto _test_eof252;
case 252:
	if ( (*p) == 105 )
		goto st253;
	goto st0;
st253:
	if ( ++p == pe )
		goto _test_eof253;
case 253:
	if ( (*p) == 103 )
		goto st254;
	goto st0;
st254:
	if ( ++p == pe )
		goto _test_eof254;
case 254:
	if ( (*p) == 104 )
		goto st255;
	goto st0;
st255:
	if ( ++p == pe )
		goto _test_eof255;
case 255:
	if ( (*p) == 116 )
		goto st297;
	goto st0;
st256:
	if ( ++p == pe )
		goto _test_eof256;
case 256:
	if ( (*p) == 110 )
		goto st257;
	goto st0;
st257:
	if ( ++p == pe )
		goto _test_eof257;
case 257:
	if ( (*p) == 116 )
		goto st258;
	goto st0;
st258:
	if ( ++p == pe )
		goto _test_eof258;
case 258:
	if ( (*p) == 104 )
		goto st259;
	goto st0;
st259:
	if ( ++p == pe )
		goto _test_eof259;
case 259:
	if ( (*p) == 108 )
		goto st260;
	goto st0;
st260:
	if ( ++p == pe )
		goto _test_eof260;
case 260:
	if ( (*p) == 121 )
		goto st299;
	goto st0;
st299:
	if ( ++p == pe )
		goto _test_eof299;
case 299:
	goto st0;
st261:
	if ( ++p == pe )
		goto _test_eof261;
case 261:
	if ( (*p) == 101 )
		goto st262;
	goto st0;
st262:
	if ( ++p == pe )
		goto _test_eof262;
case 262:
	if ( (*p) == 101 )
		goto st263;
	goto st0;
st263:
	if ( ++p == pe )
		goto _test_eof263;
case 263:
	if ( (*p) == 107 )
		goto st264;
	goto st0;
st264:
	if ( ++p == pe )
		goto _test_eof264;
case 264:
	if ( (*p) == 108 )
		goto st265;
	goto st0;
st265:
	if ( ++p == pe )
		goto _test_eof265;
case 265:
	if ( (*p) == 121 )
		goto st300;
	goto st0;
st300:
	if ( ++p == pe )
		goto _test_eof300;
case 300:
	goto st0;
st266:
	if ( ++p == pe )
		goto _test_eof266;
case 266:
	if ( (*p) == 101 )
		goto st267;
	goto st0;
st267:
	if ( ++p == pe )
		goto _test_eof267;
case 267:
	if ( (*p) == 97 )
		goto st268;
	goto st0;
st268:
	if ( ++p == pe )
		goto _test_eof268;
case 268:
	if ( (*p) == 114 )
		goto st238;
	goto st0;
	}
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 
	_test_eof5: cs = 5; goto _test_eof; 
	_test_eof6: cs = 6; goto _test_eof; 
	_test_eof7: cs = 7; goto _test_eof; 
	_test_eof8: cs = 8; goto _test_eof; 
	_test_eof9: cs = 9; goto _test_eof; 
	_test_eof10: cs = 10; goto _test_eof; 
	_test_eof11: cs = 11; goto _test_eof; 
	_test_eof269: cs = 269; goto _test_eof; 
	_test_eof12: cs = 12; goto _test_eof; 
	_test_eof270: cs = 270; goto _test_eof; 
	_test_eof13: cs = 13; goto _test_eof; 
	_test_eof14: cs = 14; goto _test_eof; 
	_test_eof15: cs = 15; goto _test_eof; 
	_test_eof16: cs = 16; goto _test_eof; 
	_test_eof271: cs = 271; goto _test_eof; 
	_test_eof17: cs = 17; goto _test_eof; 
	_test_eof18: cs = 18; goto _test_eof; 
	_test_eof19: cs = 19; goto _test_eof; 
	_test_eof20: cs = 20; goto _test_eof; 
	_test_eof272: cs = 272; goto _test_eof; 
	_test_eof21: cs = 21; goto _test_eof; 
	_test_eof22: cs = 22; goto _test_eof; 
	_test_eof23: cs = 23; goto _test_eof; 
	_test_eof273: cs = 273; goto _test_eof; 
	_test_eof24: cs = 24; goto _test_eof; 
	_test_eof25: cs = 25; goto _test_eof; 
	_test_eof26: cs = 26; goto _test_eof; 
	_test_eof27: cs = 27; goto _test_eof; 
	_test_eof28: cs = 28; goto _test_eof; 
	_test_eof29: cs = 29; goto _test_eof; 
	_test_eof30: cs = 30; goto _test_eof; 
	_test_eof31: cs = 31; goto _test_eof; 
	_test_eof32: cs = 32; goto _test_eof; 
	_test_eof33: cs = 33; goto _test_eof; 
	_test_eof34: cs = 34; goto _test_eof; 
	_test_eof35: cs = 35; goto _test_eof; 
	_test_eof36: cs = 36; goto _test_eof; 
	_test_eof37: cs = 37; goto _test_eof; 
	_test_eof38: cs = 38; goto _test_eof; 
	_test_eof39: cs = 39; goto _test_eof; 
	_test_eof40: cs = 40; goto _test_eof; 
	_test_eof41: cs = 41; goto _test_eof; 
	_test_eof42: cs = 42; goto _test_eof; 
	_test_eof43: cs = 43; goto _test_eof; 
	_test_eof44: cs = 44; goto _test_eof; 
	_test_eof45: cs = 45; goto _test_eof; 
	_test_eof46: cs = 46; goto _test_eof; 
	_test_eof47: cs = 47; goto _test_eof; 
	_test_eof48: cs = 48; goto _test_eof; 
	_test_eof49: cs = 49; goto _test_eof; 
	_test_eof50: cs = 50; goto _test_eof; 
	_test_eof51: cs = 51; goto _test_eof; 
	_test_eof52: cs = 52; goto _test_eof; 
	_test_eof53: cs = 53; goto _test_eof; 
	_test_eof54: cs = 54; goto _test_eof; 
	_test_eof55: cs = 55; goto _test_eof; 
	_test_eof56: cs = 56; goto _test_eof; 
	_test_eof57: cs = 57; goto _test_eof; 
	_test_eof58: cs = 58; goto _test_eof; 
	_test_eof59: cs = 59; goto _test_eof; 
	_test_eof60: cs = 60; goto _test_eof; 
	_test_eof61: cs = 61; goto _test_eof; 
	_test_eof62: cs = 62; goto _test_eof; 
	_test_eof63: cs = 63; goto _test_eof; 
	_test_eof64: cs = 64; goto _test_eof; 
	_test_eof65: cs = 65; goto _test_eof; 
	_test_eof66: cs = 66; goto _test_eof; 
	_test_eof67: cs = 67; goto _test_eof; 
	_test_eof68: cs = 68; goto _test_eof; 
	_test_eof69: cs = 69; goto _test_eof; 
	_test_eof70: cs = 70; goto _test_eof; 
	_test_eof71: cs = 71; goto _test_eof; 
	_test_eof72: cs = 72; goto _test_eof; 
	_test_eof73: cs = 73; goto _test_eof; 
	_test_eof74: cs = 74; goto _test_eof; 
	_test_eof75: cs = 75; goto _test_eof; 
	_test_eof76: cs = 76; goto _test_eof; 
	_test_eof77: cs = 77; goto _test_eof; 
	_test_eof78: cs = 78; goto _test_eof; 
	_test_eof79: cs = 79; goto _test_eof; 
	_test_eof80: cs = 80; goto _test_eof; 
	_test_eof81: cs = 81; goto _test_eof; 
	_test_eof82: cs = 82; goto _test_eof; 
	_test_eof83: cs = 83; goto _test_eof; 
	_test_eof84: cs = 84; goto _test_eof; 
	_test_eof85: cs = 85; goto _test_eof; 
	_test_eof86: cs = 86; goto _test_eof; 
	_test_eof87: cs = 87; goto _test_eof; 
	_test_eof88: cs = 88; goto _test_eof; 
	_test_eof89: cs = 89; goto _test_eof; 
	_test_eof90: cs = 90; goto _test_eof; 
	_test_eof91: cs = 91; goto _test_eof; 
	_test_eof92: cs = 92; goto _test_eof; 
	_test_eof93: cs = 93; goto _test_eof; 
	_test_eof94: cs = 94; goto _test_eof; 
	_test_eof95: cs = 95; goto _test_eof; 
	_test_eof96: cs = 96; goto _test_eof; 
	_test_eof97: cs = 97; goto _test_eof; 
	_test_eof98: cs = 98; goto _test_eof; 
	_test_eof99: cs = 99; goto _test_eof; 
	_test_eof100: cs = 100; goto _test_eof; 
	_test_eof101: cs = 101; goto _test_eof; 
	_test_eof102: cs = 102; goto _test_eof; 
	_test_eof103: cs = 103; goto _test_eof; 
	_test_eof104: cs = 104; goto _test_eof; 
	_test_eof105: cs = 105; goto _test_eof; 
	_test_eof106: cs = 106; goto _test_eof; 
	_test_eof107: cs = 107; goto _test_eof; 
	_test_eof108: cs = 108; goto _test_eof; 
	_test_eof109: cs = 109; goto _test_eof; 
	_test_eof274: cs = 274; goto _test_eof; 
	_test_eof110: cs = 110; goto _test_eof; 
	_test_eof275: cs = 275; goto _test_eof; 
	_test_eof111: cs = 111; goto _test_eof; 
	_test_eof276: cs = 276; goto _test_eof; 
	_test_eof112: cs = 112; goto _test_eof; 
	_test_eof277: cs = 277; goto _test_eof; 
	_test_eof113: cs = 113; goto _test_eof; 
	_test_eof114: cs = 114; goto _test_eof; 
	_test_eof278: cs = 278; goto _test_eof; 
	_test_eof279: cs = 279; goto _test_eof; 
	_test_eof115: cs = 115; goto _test_eof; 
	_test_eof116: cs = 116; goto _test_eof; 
	_test_eof280: cs = 280; goto _test_eof; 
	_test_eof117: cs = 117; goto _test_eof; 
	_test_eof118: cs = 118; goto _test_eof; 
	_test_eof281: cs = 281; goto _test_eof; 
	_test_eof119: cs = 119; goto _test_eof; 
	_test_eof282: cs = 282; goto _test_eof; 
	_test_eof120: cs = 120; goto _test_eof; 
	_test_eof121: cs = 121; goto _test_eof; 
	_test_eof283: cs = 283; goto _test_eof; 
	_test_eof122: cs = 122; goto _test_eof; 
	_test_eof284: cs = 284; goto _test_eof; 
	_test_eof123: cs = 123; goto _test_eof; 
	_test_eof124: cs = 124; goto _test_eof; 
	_test_eof285: cs = 285; goto _test_eof; 
	_test_eof125: cs = 125; goto _test_eof; 
	_test_eof286: cs = 286; goto _test_eof; 
	_test_eof287: cs = 287; goto _test_eof; 
	_test_eof126: cs = 126; goto _test_eof; 
	_test_eof127: cs = 127; goto _test_eof; 
	_test_eof288: cs = 288; goto _test_eof; 
	_test_eof289: cs = 289; goto _test_eof; 
	_test_eof128: cs = 128; goto _test_eof; 
	_test_eof129: cs = 129; goto _test_eof; 
	_test_eof290: cs = 290; goto _test_eof; 
	_test_eof130: cs = 130; goto _test_eof; 
	_test_eof131: cs = 131; goto _test_eof; 
	_test_eof291: cs = 291; goto _test_eof; 
	_test_eof132: cs = 132; goto _test_eof; 
	_test_eof292: cs = 292; goto _test_eof; 
	_test_eof133: cs = 133; goto _test_eof; 
	_test_eof134: cs = 134; goto _test_eof; 
	_test_eof293: cs = 293; goto _test_eof; 
	_test_eof135: cs = 135; goto _test_eof; 
	_test_eof294: cs = 294; goto _test_eof; 
	_test_eof136: cs = 136; goto _test_eof; 
	_test_eof137: cs = 137; goto _test_eof; 
	_test_eof295: cs = 295; goto _test_eof; 
	_test_eof138: cs = 138; goto _test_eof; 
	_test_eof139: cs = 139; goto _test_eof; 
	_test_eof140: cs = 140; goto _test_eof; 
	_test_eof141: cs = 141; goto _test_eof; 
	_test_eof142: cs = 142; goto _test_eof; 
	_test_eof143: cs = 143; goto _test_eof; 
	_test_eof144: cs = 144; goto _test_eof; 
	_test_eof145: cs = 145; goto _test_eof; 
	_test_eof146: cs = 146; goto _test_eof; 
	_test_eof147: cs = 147; goto _test_eof; 
	_test_eof148: cs = 148; goto _test_eof; 
	_test_eof149: cs = 149; goto _test_eof; 
	_test_eof150: cs = 150; goto _test_eof; 
	_test_eof151: cs = 151; goto _test_eof; 
	_test_eof152: cs = 152; goto _test_eof; 
	_test_eof153: cs = 153; goto _test_eof; 
	_test_eof154: cs = 154; goto _test_eof; 
	_test_eof155: cs = 155; goto _test_eof; 
	_test_eof156: cs = 156; goto _test_eof; 
	_test_eof157: cs = 157; goto _test_eof; 
	_test_eof158: cs = 158; goto _test_eof; 
	_test_eof159: cs = 159; goto _test_eof; 
	_test_eof160: cs = 160; goto _test_eof; 
	_test_eof161: cs = 161; goto _test_eof; 
	_test_eof162: cs = 162; goto _test_eof; 
	_test_eof163: cs = 163; goto _test_eof; 
	_test_eof164: cs = 164; goto _test_eof; 
	_test_eof165: cs = 165; goto _test_eof; 
	_test_eof166: cs = 166; goto _test_eof; 
	_test_eof167: cs = 167; goto _test_eof; 
	_test_eof168: cs = 168; goto _test_eof; 
	_test_eof169: cs = 169; goto _test_eof; 
	_test_eof170: cs = 170; goto _test_eof; 
	_test_eof171: cs = 171; goto _test_eof; 
	_test_eof172: cs = 172; goto _test_eof; 
	_test_eof173: cs = 173; goto _test_eof; 
	_test_eof174: cs = 174; goto _test_eof; 
	_test_eof175: cs = 175; goto _test_eof; 
	_test_eof176: cs = 176; goto _test_eof; 
	_test_eof177: cs = 177; goto _test_eof; 
	_test_eof178: cs = 178; goto _test_eof; 
	_test_eof179: cs = 179; goto _test_eof; 
	_test_eof180: cs = 180; goto _test_eof; 
	_test_eof181: cs = 181; goto _test_eof; 
	_test_eof182: cs = 182; goto _test_eof; 
	_test_eof183: cs = 183; goto _test_eof; 
	_test_eof184: cs = 184; goto _test_eof; 
	_test_eof185: cs = 185; goto _test_eof; 
	_test_eof186: cs = 186; goto _test_eof; 
	_test_eof187: cs = 187; goto _test_eof; 
	_test_eof188: cs = 188; goto _test_eof; 
	_test_eof189: cs = 189; goto _test_eof; 
	_test_eof190: cs = 190; goto _test_eof; 
	_test_eof191: cs = 191; goto _test_eof; 
	_test_eof192: cs = 192; goto _test_eof; 
	_test_eof193: cs = 193; goto _test_eof; 
	_test_eof194: cs = 194; goto _test_eof; 
	_test_eof195: cs = 195; goto _test_eof; 
	_test_eof196: cs = 196; goto _test_eof; 
	_test_eof197: cs = 197; goto _test_eof; 
	_test_eof198: cs = 198; goto _test_eof; 
	_test_eof199: cs = 199; goto _test_eof; 
	_test_eof200: cs = 200; goto _test_eof; 
	_test_eof201: cs = 201; goto _test_eof; 
	_test_eof202: cs = 202; goto _test_eof; 
	_test_eof203: cs = 203; goto _test_eof; 
	_test_eof204: cs = 204; goto _test_eof; 
	_test_eof205: cs = 205; goto _test_eof; 
	_test_eof206: cs = 206; goto _test_eof; 
	_test_eof207: cs = 207; goto _test_eof; 
	_test_eof208: cs = 208; goto _test_eof; 
	_test_eof209: cs = 209; goto _test_eof; 
	_test_eof210: cs = 210; goto _test_eof; 
	_test_eof211: cs = 211; goto _test_eof; 
	_test_eof212: cs = 212; goto _test_eof; 
	_test_eof213: cs = 213; goto _test_eof; 
	_test_eof214: cs = 214; goto _test_eof; 
	_test_eof215: cs = 215; goto _test_eof; 
	_test_eof216: cs = 216; goto _test_eof; 
	_test_eof217: cs = 217; goto _test_eof; 
	_test_eof218: cs = 218; goto _test_eof; 
	_test_eof219: cs = 219; goto _test_eof; 
	_test_eof220: cs = 220; goto _test_eof; 
	_test_eof221: cs = 221; goto _test_eof; 
	_test_eof222: cs = 222; goto _test_eof; 
	_test_eof223: cs = 223; goto _test_eof; 
	_test_eof224: cs = 224; goto _test_eof; 
	_test_eof225: cs = 225; goto _test_eof; 
	_test_eof226: cs = 226; goto _test_eof; 
	_test_eof227: cs = 227; goto _test_eof; 
	_test_eof228: cs = 228; goto _test_eof; 
	_test_eof229: cs = 229; goto _test_eof; 
	_test_eof230: cs = 230; goto _test_eof; 
	_test_eof231: cs = 231; goto _test_eof; 
	_test_eof232: cs = 232; goto _test_eof; 
	_test_eof233: cs = 233; goto _test_eof; 
	_test_eof234: cs = 234; goto _test_eof; 
	_test_eof235: cs = 235; goto _test_eof; 
	_test_eof236: cs = 236; goto _test_eof; 
	_test_eof237: cs = 237; goto _test_eof; 
	_test_eof238: cs = 238; goto _test_eof; 
	_test_eof239: cs = 239; goto _test_eof; 
	_test_eof296: cs = 296; goto _test_eof; 
	_test_eof240: cs = 240; goto _test_eof; 
	_test_eof241: cs = 241; goto _test_eof; 
	_test_eof242: cs = 242; goto _test_eof; 
	_test_eof243: cs = 243; goto _test_eof; 
	_test_eof297: cs = 297; goto _test_eof; 
	_test_eof244: cs = 244; goto _test_eof; 
	_test_eof245: cs = 245; goto _test_eof; 
	_test_eof246: cs = 246; goto _test_eof; 
	_test_eof247: cs = 247; goto _test_eof; 
	_test_eof248: cs = 248; goto _test_eof; 
	_test_eof298: cs = 298; goto _test_eof; 
	_test_eof249: cs = 249; goto _test_eof; 
	_test_eof250: cs = 250; goto _test_eof; 
	_test_eof251: cs = 251; goto _test_eof; 
	_test_eof252: cs = 252; goto _test_eof; 
	_test_eof253: cs = 253; goto _test_eof; 
	_test_eof254: cs = 254; goto _test_eof; 
	_test_eof255: cs = 255; goto _test_eof; 
	_test_eof256: cs = 256; goto _test_eof; 
	_test_eof257: cs = 257; goto _test_eof; 
	_test_eof258: cs = 258; goto _test_eof; 
	_test_eof259: cs = 259; goto _test_eof; 
	_test_eof260: cs = 260; goto _test_eof; 
	_test_eof299: cs = 299; goto _test_eof; 
	_test_eof261: cs = 261; goto _test_eof; 
	_test_eof262: cs = 262; goto _test_eof; 
	_test_eof263: cs = 263; goto _test_eof; 
	_test_eof264: cs = 264; goto _test_eof; 
	_test_eof265: cs = 265; goto _test_eof; 
	_test_eof300: cs = 300; goto _test_eof; 
	_test_eof266: cs = 266; goto _test_eof; 
	_test_eof267: cs = 267; goto _test_eof; 
	_test_eof268: cs = 268; goto _test_eof; 

	_test_eof: {}
	if ( p == eof )
	{
	switch ( cs ) {
	case 271: 
	case 275: 
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 269: 
	case 270: 
	case 274: 
#line 114 "lcron.ragel"
	{
        for (i = 0; i < current_len; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 287: 
#line 177 "lcron.ragel"
	{
        days_of_week[numbered_day] |= MARK_L_IN_MONTH;
    }
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 296: 
#line 305 "lcron.ragel"
	{call_l_next("0 0 0 1 1 ?");}
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 299: 
#line 306 "lcron.ragel"
	{call_l_next("0 0 0 1 * ?");}
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 300: 
#line 307 "lcron.ragel"
	{call_l_next("0 0 0 ? * 2");}
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 297: 
#line 308 "lcron.ragel"
	{call_l_next("0 0 0 * * ?");}
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 298: 
#line 309 "lcron.ragel"
	{call_l_next("0 0 * * * ?");}
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 272: 
	case 277: 
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 273: 
	case 286: 
#line 146 "lcron.ragel"
	{
        increment_interval = atoi(token_start);
    }
#line 150 "lcron.ragel"
	{
        while (increment_start < current_len) {
            current[increment_start] |= MARK_NORMAL;
            increment_start += increment_interval;
        }
    }
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 276: 
#line 169 "lcron.ragel"
	{
        numbered_week = atoi(token_start) - DAYS_OF_WEEK_START;
    }
#line 173 "lcron.ragel"
	{
        days_of_week[numbered_day] |= MARK_1_IN_MONTH << numbered_week;
    }
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 292: 
#line 266 "lcron.ragel"
	{*token_start = '1';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 290: 
#line 267 "lcron.ragel"
	{*token_start = '2';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 294: 
#line 268 "lcron.ragel"
	{*token_start = '3';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 295: 
#line 269 "lcron.ragel"
	{*token_start = '4';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 293: 
#line 270 "lcron.ragel"
	{*token_start = '5';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 288: 
#line 271 "lcron.ragel"
	{*token_start = '6';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 291: 
#line 272 "lcron.ragel"
	{*token_start = '7';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 289: 
#line 273 "lcron.ragel"
	{*token_start = '1';}
#line 110 "lcron.ragel"
	{
        current[atoi(token_start) - current_offset] |= MARK_NORMAL;
    }
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 282: 
#line 266 "lcron.ragel"
	{*token_start = '1';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 280: 
#line 267 "lcron.ragel"
	{*token_start = '2';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 284: 
#line 268 "lcron.ragel"
	{*token_start = '3';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 285: 
#line 269 "lcron.ragel"
	{*token_start = '4';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 283: 
#line 270 "lcron.ragel"
	{*token_start = '5';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 278: 
#line 271 "lcron.ragel"
	{*token_start = '6';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 281: 
#line 272 "lcron.ragel"
	{*token_start = '7';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
#line 329 "lcron.ragel"
	{res = 1;}
	break;
	case 279: 
#line 273 "lcron.ragel"
	{*token_start = '1';}
#line 124 "lcron.ragel"
	{
        range_end = atoi(token_start) - current_offset;
        if (range_end <= range_start) {
            lua_pushinteger(L, -1);
            return 1;
        }
    }
#line 132 "lcron.ragel"
	{
        for (i=range_start; i<range_end; i++) {
            current[i] |= MARK_NORMAL;
        }
    }
#line 329 "lcron.ragel"
	{res = 1;}
	break;
#line 8582 "lcron.c"
	}
	}

	_out: {}
	}
#line 540 "lcron.ragel"

    /* The parser sets res to 1 on success */
    if (res != 1) {
        lua_pushinteger(L, -1);
        return 1;
    }

    current_time = (time_t) luaL_optint(L, 2, time(NULL));
    current_time++;
    current_time_tm = localtime(&current_time);

    for (year=current_time_tm->tm_year + TM_YEAR_START; year<=YEARS_END; year++, inc_year(current_time_tm)) {
        if (!years[year - YEARS_START]) {
            continue;
        }

        for (month=current_time_tm->tm_mon; month<=MONTHS_END; month++, inc_month(current_time_tm)) {
            if (!months[month]) {
                continue;
            }

            for (day_of_month=current_time_tm->tm_mday; day_of_month<=DAYS_OF_MONTH_END; day_of_month++, inc_mday(current_time_tm)) {
                /* Check if the date is valid */
                broken_time.tm_year = year - TM_YEAR_START;
                broken_time.tm_mon = month;
                broken_time.tm_mday = day_of_month;
                broken_time.tm_hour = 0;
                broken_time.tm_min = 0;
                broken_time.tm_sec = 0;
                broken_time.tm_isdst = -1;
                time_result = mktime(&broken_time);

                if (time_result == (time_t)(-1) || broken_time.tm_mon != month) {
                    continue;
                }

                if (!(days_of_week[broken_time.tm_wday] & MARK_NORMAL)) {
                    if (!is_numbered_week_day(&broken_time, days_of_week)) {
                        continue;
                    }
                }

                if (!days_of_month[day_of_month - DAYS_OF_MONTH_START]) {
                    /* Check if 'L' was used and we're at the last day of month */
                    if (!days_of_month[DAYS_OF_MONTH_LEN] || !is_last_day_of_month(&broken_time)) {
                        if (!is_closest_weekday(&broken_time, days_of_month)) {
                            continue;
                        }
                    }
                }

                for (hour=current_time_tm->tm_hour; hour<HOURS_LEN; hour++, inc_hour(current_time_tm)) {
                    if (!hours[hour]) {
                        continue;
                    }

                    for (minute=current_time_tm->tm_min; minute<MINUTES_LEN; minute++, inc_min(current_time_tm)) {
                        if (!minutes[minute]) {
                            continue;
                        }

                        for (second=current_time_tm->tm_sec; second<SECONDS_LEN; second++) {
                            if (!seconds[second]) {
                                continue;
                            }

                            /* Got it! */
                            broken_time.tm_year = year - TM_YEAR_START;
                            broken_time.tm_mon = month;
                            broken_time.tm_mday = day_of_month;
                            broken_time.tm_hour = hour;
                            broken_time.tm_min = minute;
                            broken_time.tm_sec = second;
                            broken_time.tm_isdst = -1;
                            time_result = mktime(&broken_time);

                            lua_pushinteger(L, difftime(time_result, current_time-1));
                            return 1;
                        }
                    }
                }
            }
        }
    }

    lua_pushinteger(L, 0);
    return 1;
}

static const luaL_reg R[] = {
    {"next", l_next},
    {NULL, NULL},
};

LUALIB_API int luaopen_cron (lua_State *L) {
    luaL_register(L, "cron", R);

    lua_pushliteral(L, "MAX_YEAR");
    lua_pushnumber(L, YEARS_END);
    lua_settable(L, -3);

    return 1;
}

/* vim: set ft=c: */

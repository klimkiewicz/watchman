require 'lcron'
require 'lunit'

module('lcron_testcase', lunit.testcase, package.seeall)

--       January 2001    
--   Su Mo Tu We Th Fr Sa
--       1  2  3  4  5  6
--    7  8  9 10 11 12 13
--   14 15 16 17 18 19 20
--   21 22 23 24 25 26 27
--   28 29 30 31

local basic_time = os.time {
    year = 2001,
    month = 1,
    day = 1,
    hour = 0,
    min = 0,
    sec = 0,
}

function assert_time(s, time, start_time)
    start_time = start_time or basic_time
    local end_time = os.time {
        year = time[1],
        month = time[2],
        day = time[3],
        hour = time[4] or 0,
        min = time[5] or 0,
        sec = time[6] or 0,
    }
    assert_equal(os.difftime(end_time, start_time), s)
end

function assert_cron(cron, time, start_time)
    if (start_time) then
        start_time = os.time {
            year = start_time[1],
            month = start_time[2],
            day = start_time[3],
            hour = start_time[4] or 0,
            min = start_time[5] or 0,
            sec = start_time[6] or 0,
        }
    else
        start_time = basic_time
    end
    assert_time(lcron.next(cron, start_time), time, start_time)
end

function assert_table(t)
    for i, v in ipairs(t) do
        assert_cron(v[1], v[2], v[3])
    end
end

function test_special()
    assert_table {
        {'@yearly', {2002, 1, 1}},
        {'@annually', {2002, 1, 1}},
        {'@monthly', {2001, 2, 1}},
        {'@daily', {2001, 1, 2}},
        -- 2001-01-01 is Monday
        {'@weekly', {2001, 1, 8}},
        {'@weekly', {2001, 1, 8}, {2001, 1, 5}},
        {'@daily', {2001, 1, 2}},
        {'@midnight', {2001, 1, 2}},
        {'@hourly', {2001, 1, 1, 1}},
    }
end

function test_weekdays()
    assert_table {
        {'* * * ? * MON', {2001, 1, 1, 0, 0, 1}},
        {'* * * ? * TUE', {2001, 1, 2}},
        {'* * * ? * mon#1', {2001, 1, 1, 0, 0, 1}},
        {'* * * ? * MON#2', {2001, 1, 8}},
        {'* * * ? * MON#3', {2001, 1, 15}},
        {'* * * ? * MON#4', {2001, 1, 22}},
        {'* * * ? * MON#5', {2001, 1, 29}},
        {'* * * ? * tue#5', {2001, 1, 30}},
        {'* * * ? * MONL', {2001, 1, 29}},
        {'* * * ? * SAT#5', {2001, 3, 31}},
        {'* * * ? * 1#5', {2001, 4, 29}},
        {'* * * ? * TUE#5 2011', {2011, 3, 29}},
        {'* * * ? * L', {2001, 1, 7}},
        {'* * * ? * SAT,sun,4', {2001, 1, 3}},
        {'* * * ? * 1-7', {2001, 1, 1, 0, 0, 1}},
        {'* * * ? * */2', {2001, 1, 2}},
        {'* * * ? * mon/2', {2001, 1, 1, 0, 0, 1}},
    }
end

function test_bulk()
    for year = 1970, lcron.MAX_YEAR do
        for month = 1, 12 do
            for day = 1, 28 do
                assert_cron('* * * ? * *', {year, month, day, 0, 0, 1}, {year, month, day})
            end
        end
    end
end

function test_various()
    assert_table {
        -- From Wikipedia
        {'0 2/3 1,9,22 11-26 1-6 ? 2003', {2003, 1, 11, 1, 2, 0}},
        {'0 0 23 ? * MON-FRI', {2001, 1, 1, 23}},
        {'0 0 23 ? * MON-FRI', {2001, 1, 8, 23}, {2001, 1, 6}},
    }
end

function test_invalid()
    assert_equal(lcron.next('dummy'), -1)
    assert_equal(lcron.next('@dummy'), -1)
    assert_equal(lcron.next('* * * * * *'), -1)
    -- It's actually taken from Wikipedia (for @weekly)
    assert_equal(lcron.next('0 0 0 ? * 0'), -1)
    assert_equal(lcron.next('* * * ? * * ' .. tostring(lcron.MAX_YEAR + 1)), 0)
    assert_equal(lcron.next('* * *'), -1)
    assert_equal(lcron.next('60 * * * * ?'), -1)
    assert_equal(lcron.next('* * * * * ? 2100'), -1)
end


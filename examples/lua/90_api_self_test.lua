-- Non-destructive smoke test for the read-only RFSuite Lua API.
local passed, failed = 0, 0

local function test(name, fn)
    local ok, message = pcall(fn)
    if ok then
        passed = passed + 1
        print("PASS", name)
    else
        failed = failed + 1
        print("FAIL", name, tostring(message))
    end
end

test("millis", function()
    assert(type(rf.millis()) == "number")
    assert(rf.millis() >= 0)
end)

test("status fields", function()
    local s = rf.status()
    assert(type(s) == "table")
    assert(type(s.peak_channel) == "number")
    assert(type(s.peak_level) == "number")
    assert(type(s.confidence) == "number")
    assert(type(s.radios) == "number")
    assert(type(s.frozen) == "boolean")
    assert(type(s.logging) == "boolean")
end)

test("peak channel", function()
    local channel = rf.peak_channel()
    assert(channel >= 0 and channel <= 125)
    local level = rf.level(channel)
    assert(level >= 0 and level <= 100)
end)

test("spectrum shape", function()
    local levels = rf.spectrum()
    assert(type(levels) == "table")
    assert(#levels == 126, "expected 126 values, got " .. tostring(#levels))
    for channel = 0, 125 do
        local level = levels[channel + 1]
        assert(type(level) == "number")
        assert(level >= 0 and level <= 100,
               string.format("CH%d out of range: %s", channel, tostring(level)))
    end
end)

test("invalid channel rejected", function()
    local ok = pcall(function() rf.level(126) end)
    assert(not ok, "rf.level(126) should fail")
end)

local summary = string.format("API SELF TEST: %d PASS, %d FAIL", passed, failed)
print(summary)
if failed > 0 then error(summary) end


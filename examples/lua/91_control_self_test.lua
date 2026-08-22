-- Test reversible analyzer controls and argument validation.
local passed, failed = 0, 0
local initial = rf.status()

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

test("set cursor", function()
    local target = (initial.cursor + 1) % 126
    rf.set_cursor(target)
    assert(rf.status().cursor == target)
    rf.set_cursor(initial.cursor)
end)

test("freeze and resume", function()
    rf.freeze(true)
    assert(rf.status().frozen == true)
    rf.freeze(false)
    assert(rf.status().frozen == false)
    rf.freeze(initial.frozen)
end)

test("invalid cursor rejected", function()
    assert(not pcall(function() rf.set_cursor(-1) end))
    assert(not pcall(function() rf.set_cursor(126) end))
end)

test("invalid band rejected", function()
    assert(not pcall(function() rf.set_band("invalid") end))
end)

test("invalid trace rejected", function()
    assert(not pcall(function() rf.set_trace("invalid") end))
end)

-- Restore the state even if an earlier assertion failed.
rf.set_cursor(initial.cursor)
rf.freeze(initial.frozen)

local summary = string.format("CONTROL SELF TEST: %d PASS, %d FAIL", passed, failed)
print(summary)
if failed > 0 then error(summary) end


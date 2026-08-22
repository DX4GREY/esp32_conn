-- Storage and passive-environment integration test.
-- This appends one line to /RFSuite/log/lua.log.
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

test("SD log append", function()
    rf.log("integration self-test marker")
end)

test("environment start/stop", function()
    assert(initial.radios > 0, "no radio available")
    if not initial.environment_running then
        assert(rf.environment(true), "start failed")
        assert(rf.status().environment_running == true)
        assert(rf.environment(false), "stop failed")
        -- Stop is asynchronous; the Core 0 worker clears the running flag
        -- after leaving its current sampling batch.
    end
end)

-- Preserve a pre-existing environment run.
if initial.environment_running and not rf.status().environment_running then
    rf.environment(true)
end

local summary = string.format("INTEGRATION TEST: %d PASS, %d FAIL", passed, failed)
print(summary)
if failed > 0 then error(summary) end

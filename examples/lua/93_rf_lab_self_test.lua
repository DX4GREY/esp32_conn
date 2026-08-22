-- Controlled-lab RF API self-test.
-- Run only in a shielded setup using the authorized_rf_lab firmware build.
-- The normal analyzer build reports SKIP because active RF is compiled out.

local function stop_safely()
    pcall(function() rf.lab_stop() end)
end

-- Always request cleanup if a later assertion fails.
stop_safely()

local available, result = pcall(function()
    return rf.lab_start("wifi")
end)

if not available then
    stop_safely()
    local message = tostring(result)
    if string.find(message, "disabled", 1, true) then
        print("SKIP", "active RF disabled in analyzer build")
        return
    end
    error("RF start failed: " .. message)
end

local cleanupOk, cleanupError = pcall(function()
    assert(result == true, "lab_start returned false")
    -- Stop immediately: this test validates the API transition, not RF range.
    rf.lab_stop()
end)
stop_safely()
assert(cleanupOk, cleanupError)
print("PASS", "authorized Wi-Fi RF start/stop")

local invalidAccepted = pcall(function()
    rf.lab_start("invalid-target")
end)
stop_safely()
assert(not invalidAccepted, "invalid RF target should be rejected")
print("PASS", "invalid RF target rejected")
print("RF LAB SELF TEST: 2 PASS, 0 FAIL")


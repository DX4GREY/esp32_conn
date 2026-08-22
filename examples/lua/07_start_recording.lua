-- Start a fresh CSV recording session and open the logging screen.
local ok = rf.recording(true)
if ok then
    print("recording started")
    rf.log("recording started by Lua")
    rf.open_screen("logging")
else
    error("could not start recording")
end


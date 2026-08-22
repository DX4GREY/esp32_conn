-- Move the cursor to the current busiest channel and open the inspector.
local channel = rf.peak_channel()
local level = rf.level(channel)
rf.set_cursor(channel)
rf.freeze(true)
print(string.format("inspect CH%d level=%d%%", channel, level))
rf.open_screen("inspect")


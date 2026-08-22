-- Copy this file to /RFSuite/scripts/channel_report.lua on the SD card.
local peak = rf.peak_channel()
local level = rf.level(peak)
local message = string.format("peak channel=%d level=%d%%", peak, level)

print(message)
rf.log(message)

-- Summarize activity in the firmware's Wi-Fi scan range (channels 1..73).
local levels = rf.spectrum()
local total, active, peakChannel, peakLevel = 0, 0, 1, 0

for channel = 1, 73 do
    local level = levels[channel + 1]
    total = total + level
    if level >= 50 then active = active + 1 end
    if level > peakLevel then
        peakLevel, peakChannel = level, channel
    end
end

local average = math.floor(total / 73)
local message = string.format("wifi avg=%d%% active=%d peak=CH%d/%d%%",
                              average, active, peakChannel, peakLevel)
print(message)
rf.log(message)


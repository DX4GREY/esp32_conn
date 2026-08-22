-- Log every channel at or above the chosen activity threshold.
local threshold = 70
local levels = rf.spectrum()
local matches = 0

for index, level in ipairs(levels) do
    if level >= threshold then
        local channel = index - 1
        local message = string.format("ALERT ch=%d level=%d%%", channel, level)
        print(message)
        rf.log(message)
        matches = matches + 1
    end
end

print(string.format("threshold=%d%% matches=%d", threshold, matches))


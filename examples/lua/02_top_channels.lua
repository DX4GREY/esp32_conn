-- Show the ten busiest channels in the current spectrum snapshot.
local levels = rf.spectrum()
local ranked = {}

for index, level in ipairs(levels) do
    ranked[#ranked + 1] = {channel = index - 1, level = level}
end

table.sort(ranked, function(a, b)
    if a.level == b.level then return a.channel < b.channel end
    return a.level > b.level
end)

print("TOP 10 CHANNELS")
for i = 1, math.min(10, #ranked) do
    print(string.format("%2d. CH%03d  %3d%%", i, ranked[i].channel, ranked[i].level))
end


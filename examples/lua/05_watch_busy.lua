-- Add watch markers to the five busiest channels.
local levels = rf.spectrum()
local ranked = {}
for index, level in ipairs(levels) do
    ranked[#ranked + 1] = {channel = index - 1, level = level}
end
table.sort(ranked, function(a, b) return a.level > b.level end)

for i = 1, math.min(5, #ranked) do
    rf.toggle_watch(ranked[i].channel)
    print(string.format("watch CH%d (%d%%)", ranked[i].channel, ranked[i].level))
end
rf.open_screen("spectrum")


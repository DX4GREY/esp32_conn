-- Print a compact analyzer and radio health report.
local s = rf.status()
print(string.format("uptime=%d ms radios=%d sweeps=%d", rf.millis(), s.radios, s.sweeps))
print(string.format("peak=CH%d level=%d%% confidence=%d%%", s.peak_channel, s.peak_level, s.confidence))
print("frozen=" .. tostring(s.frozen) .. " logging=" .. tostring(s.logging))


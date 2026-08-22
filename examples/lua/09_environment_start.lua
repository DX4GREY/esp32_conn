-- Start passive RF environment sampling.
local s = rf.status()
assert(s.radios > 0, "no radio available")
assert(rf.environment(true), "environment analyzer is busy")
print("environment sampling started")
rf.log("environment sampling started by Lua")


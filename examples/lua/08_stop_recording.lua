-- Stop the active CSV recording session.
assert(rf.recording(false), "could not stop recording")
print("recording stopped")
rf.open_screen("logging")


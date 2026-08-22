-- Capture the current average as baseline and switch to the DELTA trace.
rf.freeze(true)
rf.capture_baseline()
rf.clear_max()
rf.set_trace("delta")
rf.freeze(false)
rf.log("delta baseline captured")
rf.open_screen("spectrum")


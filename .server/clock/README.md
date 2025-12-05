## clock functions:

**`startClock()`**  
Records the current CPU time as the starting point for performance measurement. Returns a `clock_tt` value representing the start time.

**`endClock()`**  
Records the current CPU time as the ending point for performance measurement. Returns a `clock_tt` value representing the end time.

**`calculateTime(clock_tt start, clock_tt end)`**  
Calculates and returns the elapsed time between two clock measurements. Takes the start and end `clock_tt` values and returns the time difference in ms.
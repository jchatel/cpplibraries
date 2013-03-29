cpplibraries
============

My C++ libraries, free to grab and modify at will.

It's a straight dump of my own depot, so paths may not be friendly with you and you may find awkward that I got versioned directories (but I can have multiple versions references by many of my projects).

A few things to note that are special in how I develop efficiently

1. All my temporary compiler output files (.obj and so forth) are output to Z: drive, so you will most likely need to map it.
Reason are that I don't clutter my depot with compiler files and that I use a ram disk (allocate some memory to act like a hard disk), so I compile fast
Of course, you can change temporary paths in project files

2. There are some dependencies between libraries as they are core. For example, Allocator is used by every library as the interface


All in all, have fun.


Kind regards,
Jeremy Chatelaine

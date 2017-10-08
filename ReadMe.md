This is the source for the rewrite of BurnItNow.

If you build from source manually instead of using haikuporter
and the recipe at haikuports, BurnItNow depends on:

- cdrecord
- mkisofs
- readcd

BurnItNow expects the "ReadMe.html" and "images" folder in
$(findpaths B_FIND_PATH_DOCUMENTATION_DIRECTORY) in a
"packages/burnitnow" subfolder, e.g.
/boot/home/config/non-packaged/documentation/packages/burnitnow/

----

For the original version of BurnItNow, please have a look at the
"legacy" branch.

OpenSearch Convert

This converts proxied ting items id's to OpenSearch 3.1 format id.
Mostly this module should be run only once.
No direct call to ting library, only NanoSoap dependency making the requests relatively fast.

For most precise results items are requested one by one. This may take ten's oh hours
if ting_object table is really large.
For this purpose each item is requested via a separate AJAX call, so PHP won't throw
fatals regarding max_execution time (see Note p.5).

Special notes here:

1. This will work fine for bookmarks, reservations, loans only if the
id of the item is in the ting_object table. By logic means this item id should be
there since the user somehow accessed and bookmarked/reserved it;

2. InterLibrary loans could be the issue with conversion since those are might not
be requested from current site and their id's are not in the system;

3. Originally openruth has no mechanism of searching items, so if an item was loaned
or reserved in A ting/openruth agency and this item is not found, then the instance switched
to some B agency, most likely the item will still be shown as blank;

4. Pseudo-ting object is not working by any means now and there is no current solution
for this, since ting library was refactored;

5. Conversion process will be time demanding. And by this is meant tens (if not hundreds) of hours
too lookup all the records in the ting_object table. For a ~650k entries and a 800ms request time,
it takes smth like ~144h to query and update them all.

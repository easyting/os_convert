OpenSearch Convert

This converts proxied ting items id's to OpenSearch 3.1 format id.
Mostly this module should be run only once.
No direct call to ting library, only NanoSoap dependency making the requests relatively fast.

For most precise results items are requested one by one. This may take ten's oh hours
if ting_object table is really large.
For this purpose each item is requested via a separate AJAX call, so PHP won't throw
fatals regarding max_execution time.

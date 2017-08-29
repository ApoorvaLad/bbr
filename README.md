# bbr


Once the ns-3 setup is completed, please replace the following files in the respective paths

tcp-congestion-ops.cc,tcp-congestion-ops.h, tcp-socket-base.cc, tcp-socket-base.h, tcp-bbr.c, tcp-bbr.h - source/ns-3.26/src/internet/model

tcp-bulk-send - source/ns-3.26/examples/tcp

Add tcp-bbr.cc and tcp-bbr.h in the source/ns-3.26/src/internet/model folder.
Replace the wscript file in source/ns-3.26/src/internet directory.

To execute the test file, enter the following command from the source/ns-3.26 directory

./waf --run tcp-bulk-send 

#!/bin/bash

#
# Run ns3 (waf) with log environment set.
#

# Output file.
OUT_FILE="out.txt"

# Enable log messages.
#   See: https://www.nsnam.org/docs/manual/html/logging.html
# Format: componenet=log_level|options
# Levels:
#   LOG_NONE
#   LOG_ERROR
#   LOG_WARN
#   LOG_DEBUG
#   LOG_INFO
#   LOG_FUNCTION
#   LOG_LOGIC
#   Note: level_all turns them all on.
# Options:
#   prefix_time - display function/method
#   prefix_func - display time
NS_LOG="TcpBulkSendExample=level_all|prefix_func|prefix_time"
NS_LOG="$NS_LOG:BulkSendApplication=level_all|prefix_func|prefix_time"
NS_LOG="$NS_LOG:PacketSink=level_all|prefix_func|prefix_time"
NS_LOG="$NS_LOG:TcpBbr=level_all|prefix_func|prefix_time"
NS_LOG="$NS_LOG:TcpWestwood=level_all|prefix_func|prefix_time"
export NS_LOG

echo "Environment: $NS_LOG" |& tee $OUT_FILE

./waf --run tcp-bulk-send |& tee -a $OUT_FILE

# Option to not go to console
#./waf --run tcp-bulk-send |& tee -a $OUT_FILE >2 /dev/null






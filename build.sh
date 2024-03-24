#!/bin/bash

# Define the log file path
# LOG_FILE="build.log"

# # Function to print a message to stdout and append it to the log file
# log() {
#     local message="$@"
#     echo "$message"
#     echo "$message" >> "$LOG_FILE"
# }

# # Redirect stderr to stdout and stdout to both stdout and the log file
# exec > >(tee -a "$LOG_FILE") 2>&1

# # Start the build process
# log "Starting build process..."

make clean

cmake .

make

# End of build process
# log "Build process completed."

# # Restore stdout and stderr
# exec >/dev/tty 2>&1

exit 0
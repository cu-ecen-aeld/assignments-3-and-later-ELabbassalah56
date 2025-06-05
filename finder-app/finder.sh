#!/bin/sh
# auther elabbas salah hatata coursera advanced embedded linux
# Ensure exactly 2 arguments are passed
if [ $# -ne 2 ]; then
    #echo "Usage: $0 <directory_path> <search_string>"
    exit 1
fi

filesdir="$1"
searchstr="$2"

# Check if directory exists
if [ ! -d "$filesdir" ]; then
    #echo "Error: Directory Not Found!"
    exit 1
fi

# Function to show animated ".." increasing & decreasing in place
animate_search() {
    while true; do
        echo -ne "Searching in directory: $filesdir "🔍"  \r"
        sleep 0.5
        echo -ne "Searching in directory: $filesdir "🔍🔍" \r"
        sleep 0.5
        echo -ne "Searching in directory: $filesdir "🔍🔍🔍"\r"
        sleep 0.5
        echo -ne "Searching in directory: $filesdir "🔍🔍" \r"
        sleep 0.5
        echo -ne "Searching in directory: $filesdir "🔍"  \r"
        sleep 0.5
    done
}

# Start the animation in the background
animate_search &

# Store the animation process ID
ANIM_PID=$!

# Run the search command
grep -r "$searchstr" "$filesdir"

# Stop the animation
kill $ANIM_PID
wait $ANIM_PID 2>/dev/null

date
# Print search complete message (overwrites the animation)
echo -e "Search complete!                   \r"

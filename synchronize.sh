#!/bin/bash
cd "$(dirname "$0")" # Set working directory to the directory of the script.

echo "Pulling changes from web repository."
git pull

echo "Checking for local changes."
git add .
git update-index --refresh
changes=false
git diff-index --quiet HEAD -- || changes=true

if [ "$changes" = true ]; then
	echo "Found local changes."
	read -p 'Enter a commit message: ' message
	message=${message:-No commit message}
	git commit -a -m "$message"
	git push
fi
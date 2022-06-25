@echo off
setlocal EnableDelayedExpansion

echo Pulling changes from web repository
git pull

echo Checking for local changes
git add .
git update-index --refresh
set /a changes=0
git diff-index --quiet HEAD -- || set /a changes=1

if %changes% == 1 (
	echo Found local changes
	set "message=[no commit message]"
	set /p "message=Enter a commit message: "
	git commit -a -m "!message!"
	git push
)
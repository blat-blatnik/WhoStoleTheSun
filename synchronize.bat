@echo off

echo pulling changes from web repository
git pull

echo checking for local changes
git add .
git update-index --refresh
set /a changes=0
git diff-index --quiet HEAD -- || set /a changes=1

if %changes% geq 1 (
	echo found local changes
	set /p message="Enter a commit message: "
	git commit -a -m "%message%"
	git push
)
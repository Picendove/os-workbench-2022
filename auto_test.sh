#!/bin/bash

projectDir=/home/go/桌面/os/os-workbench-2022/pstree

while inotifywait $projectDir --timefmt '%d/%m/%y %H:%M' --format "%T %f %e" -e MODIFY --exclude '^.*.swp$'
do
	cd $projectDir && gcc pstree.c -o test1 && ./test1
done

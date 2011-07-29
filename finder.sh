for file in `ls`; do
	echo $file
	echo `cat $file | grep -i "skill_menu"`
done

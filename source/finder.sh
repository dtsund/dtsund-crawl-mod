for file in `ls`; do
	echo $file
	echo `cat $file | grep -i "in_shop"`
done

for file in `ls`; do
	echo $file
	echo `cat $file | grep -i "You have lost your"`
done

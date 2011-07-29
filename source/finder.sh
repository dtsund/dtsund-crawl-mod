for file in `ls`; do
	echo $file
	echo `cat $file | grep -i "radiating silence"`
done

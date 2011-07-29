for file in `ls`; do
	echo $file
	echo `cat $file | grep "MONS_SPIRIT"`
done

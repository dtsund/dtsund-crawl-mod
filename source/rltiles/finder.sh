for file in `ls`; do
	echo $file
	echo `cat $file | grep "SELECTIVE_AMNESIA"`
done

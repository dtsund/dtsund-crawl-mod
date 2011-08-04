for file in `ls`; do
	echo $file
	echo `cat $file | grep -i "HALO_RANGE"`
done

rm -rfv .vs
for d in *; do 
	if [ -d $d ]; then
		rm -rfv $d/Debug $d/Release $d/x86 $d/x64 $d/'ESP32(coding only)'
	fi
done

rm -rfv Debug Release x86 x64 'ESP32(coding only)' ARM64

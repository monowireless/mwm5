# remove files
rm -rfv .vs

for f in MSC_SDL2_Screen MWM5 APP_PAL_Recv APP_Twelite_Recv MSC_Scratch MSC_glancer_con TWELITE_Stage _Scratch; do
	rm -rfv $f/Win32 $f/x64 $f/ARM64 $f/Release $f/Debug
done

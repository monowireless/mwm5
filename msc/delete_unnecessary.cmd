@echo off

cd /D %~dp0%
CD

rd /s /q .vs
FOR /d %%i in (MSC_SDL2_Screen MWM5 APP_PAL_Recv APP_Twelite_Recv MSC_Scratch MSC_glancer_con TWELITE_Stage _Scratch) do (
	rd /s /q %%i\Win32 %%i\x64 %%i\ARM64 %%i\Debug %%i\Release
)


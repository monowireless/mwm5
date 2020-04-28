@echo off

rem generate header file from gnu-style makefile.
for /f "tokens=1,2,3" %%a in (%1) do (
  rem echo %%a %%c
  if %%a==VERSION_MAIN echo #define VERSION_MAIN %%c
  if %%a==VERSION_SUB echo #define VERSION_SUB %%c
  if %%a==VERSION_VAR echo #define VERSION_VAR %%c
)

@echo off

START /WAIT doxygen WindowsDoxyfile
cd latex
START /WAIT make.bat
cd ..\
copy .\latex\refman.pdf xmlmill.pdf
copy .\html\xmlmill.chm .

@echo on

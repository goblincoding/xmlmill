@echo off

doxygen WindowsDoxyfile
cd latex
make.bat
cd ..\
copy .\latex\refman.pdf xmlmill.pdf
copy .\html\xmlmill.chm .

@echo on
#!/bin/bash
doxygen UbuntuDoxyfile 
echo "done with doxygen"
cd latex/
echo "starting make"
make 
echo "done with make"
cd ../
cp latex/refman.pdf ./xmlmill.pdf


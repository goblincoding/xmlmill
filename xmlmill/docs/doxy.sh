#!/bin/bash
doxygen UbuntuDoxyfile 
cd latex/
make 
cd ../
cp latex/refman.pdf ./xmlmill.pdf


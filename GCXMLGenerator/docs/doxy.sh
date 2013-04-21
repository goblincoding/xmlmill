#!/bin/bash
/usr/bin/doxygen UbuntuDoxyfile
cd latex
make
cd ../
cp latex/refman.pdf ./XMLMillSourceDoc.pdf


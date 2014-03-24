### XML Mill

If you are interested in the application itself, please grab it from the project's [SourceForge page](http://sourceforge.net/projects/xmlmill/)

##### Note

XML Mill is currently in a pretty stable Beta state and, although I am not actively developing it at the moment, this project is definitely not dead :)

### HELP

More information can be found on the [official website](http://goblincoding.com/xmlmill/) from where you will also be able to access an [introductory video](http://goblincoding.com/xmlmill/xmlmillfeatures/)

### DOCUMENTATION

The Doxygen-generated source documentation is available [here](http://goblincoding.github.io/xmlmill/html)

#### TROUBLESHOOTING THE APPLICATION ON UBUNTU

If you encounter the following error after having downloaded and extracted the tar ball from SourceForge (remember to start the application with the "runXMLMill.sh" script):

"This application failed to start because it could not find or load the Qt platform plugin “xcb”.Available platform plugins are: xcb.Reinstalling the application may fix this problem.
Aborted (core dumped)"

Navigate to the "platforms" directory and execute the "fixdep.sh" script.  This script will install the missing dependencies on your behalf (ensure that you have root access when running this script).

For any other issues, feel free to [contact me](http://goblincoding.com/contact/)

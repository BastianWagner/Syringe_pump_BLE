GUI was coded in python with PyQt5

PyQt5 can be installed with the comand...
> pip install PyQt5



-----------------------------------------------Dark mode notes-----------------------------------------
BreezeStyleSheets:
https://github.com/Alexhuszagh/BreezeStyleSheets

Folder:
- dark 
- light

Files:
- dark.qss
- light.qss
- breeze_resources.py
- breeze.qrc

To compile the stylesheet for use with PyQt5, compile with the following command...
> pyrcc5 breeze.qrc -o breeze_resources.py
----------------------------------------------------------------------------------------------------




-----------------------------------------------OSX Application--------------------------------------
Py2app:
py2app is a Python setuptools command which will allow you to make standalone Mac OS X application bundles and plugins from Python scripts.
NOTE: py2app must be used on OSX to build applications, it cannot create Mac applications on other platforms.

PyQt5 can be installed with the comand...
> pip3 install py2app

Folder:
- dist
- build

File:
- setup.py
- icon.icns

To build app use following comand...
> python setup.py py2app
----------------------------------------------------------------------------------------------------



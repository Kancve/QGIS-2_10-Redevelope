#!/usr/bin/python
"""
***************************************************************************
    plugin_builder.py
 A script to automate creation of a new QGIS plugin using the plugin_template
    --------------------------------------
   Date                 : Sun Sep 16 12:11:04 AKDT 2007
   Copyright            : (C) Copyright 2007 Martin Dobias
   Email                :
 Original authors of Perl version: Gary Sherman and Tim Sutton
***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
"""
import os
import sys
import shutil
import re


def template_file(file):
    return os.path.join('plugin_template', file)


def plugin_file(pluginDir, file):
    return os.path.join(pluginDir, file)

# make sure we are in the plugins directory otherwise the changes this script
# will make will wreak havoc....

myDir = os.getcwd()
print "Checking that we are in the <qgis dir>/src/plugins/ directory....",

pluginsDirectory = os.path.join('src', 'plugins')

if myDir[-len(pluginsDirectory):] == pluginsDirectory:
    print "yes"
else:
    print "no"
    print myDir
    print "Please relocate to the plugins directory before attempting to run this script."
    sys.exit(1)


# get the needed information from the user
print "Enter the directory name under qgis/src/plugins/ where your new plugin will be created."
print "We suggest using a lowercase underscore separated name e.g. clever_plugin"
print "Directory for the new plugin:",

pluginDir = raw_input()

print
print "Enter the name that will be used when creating the plugin library."
print "The name should be entered as a mixed case name with no spaces. e.g. CleverTool"
print "The plugin name will be used in the following ways:"
print "1) it will be 'lower cased' and used as the root of the generated lib name"
print "   e.g. libqgis_plugin_clevertool"
print "2) in its upper cased form it will be used as the basis for class names, in particular"
print "   CleverToolGuiBase <- The base class for the plugins configuration dialog / gui generated by uic"
print "   CleverToolGui     <- The concrete class for the plugins configuration dialog"
print "3) CleverTool     <- The class that includes the plugin loader instructions and"
print "                     and calls to your custom application logic"
print "4) clevertool.h, clevertool.cpp  etc. <- the filenames used to hold the above derived classes"
print "Plugin name:",

pluginName = raw_input()
pluginLCaseName = pluginName.lower()

print
print "Enter a short description (typically one line)"
print "e.g. The clever plugin does clever stuff in QGIS"
print "Plugin description:",
pluginDescription = raw_input()

print
print "Enter a plugin category. Category will help users"
print "to understand where to find plugin. E.g. if plugin"
print "will be available from Vector menu category is Vector"
print "Plugin category:",
pluginCategory = raw_input()

print
print "Enter the name of the application menu that will be created for your plugin"
print "Clever Tools"
print "Menu name:",
menuName = raw_input()

print
print "Enter the name of the menu entry  (under the menu that you have just defined) that"
print "will be used to invoke your plugin. e.g. Clever Plugin"
print "Menu item name:",
menuItemName = raw_input()

# print a summary of what's about to happen
# and ask if we should proceed
print
print "Summary of plugin parameters:"
print "---------------------------------------------"
print "Plugin directory:         ", pluginDir
print "Name of the plugin:       ", pluginName
print "Description of the plugin:", pluginDescription
print "Category of the plugin:   ", pluginCategory
print "Menu name:                ", menuName
print "Menu item name:           ", menuItemName
print
print "Warning - Proceeding will make changes to CMakeLists.txt in this directory."
print "Create the plugin? [y/n]:",

createIt = raw_input()

if createIt.lower() != 'y':
    print "Plugin creation cancelled, exiting"
    sys.exit(2)

# create the plugin and modify the build files

# create the new plugin directory
os.mkdir(pluginDir)

# copy files to appropriate names
shutil.copy(template_file('CMakeLists.txt'), pluginDir)
shutil.copy(template_file('README.whatnext'), os.path.join(pluginDir, 'README'))
shutil.copy(template_file('plugin.qrc'),      os.path.join(pluginDir, pluginLCaseName + '.qrc'))
shutil.copy(template_file('plugin.png'),      os.path.join(pluginDir, pluginLCaseName + '.png'))
shutil.copy(template_file('plugin.cpp'),      os.path.join(pluginDir, pluginLCaseName + '.cpp'))
shutil.copy(template_file('plugin.h'),        os.path.join(pluginDir, pluginLCaseName + '.h'))
shutil.copy(template_file('plugingui.cpp'),   os.path.join(pluginDir, pluginLCaseName + 'gui.cpp'))
shutil.copy(template_file('plugingui.h'),     os.path.join(pluginDir, pluginLCaseName + 'gui.h'))
shutil.copy(template_file('pluginguibase.ui'),    os.path.join(pluginDir, pluginLCaseName + 'guibase.ui'))

# Substitute the plugin specific vars in the various files
# This is a brute force approach but its quick and dirty :)
#

files = [plugin_file(pluginDir, 'CMakeLists.txt'),
         plugin_file(pluginDir, 'README'),
         plugin_file(pluginDir, pluginLCaseName + '.qrc'),
         plugin_file(pluginDir, pluginLCaseName + '.cpp'),
         plugin_file(pluginDir, pluginLCaseName + '.h'),
         plugin_file(pluginDir, pluginLCaseName + 'gui.cpp'),
         plugin_file(pluginDir, pluginLCaseName + 'gui.h'),
         plugin_file(pluginDir, pluginLCaseName + 'guibase.ui')]

# replace occurences of [pluginlcasename], [pluginname], [plugindescription], [menuname], [menutiem]
# in template with the values from user
replacements = [('\\[pluginlcasename\\]', pluginLCaseName),
                ('\\[pluginname\\]', pluginName),
                ('\\[plugindescription\\]', pluginDescription),
                ('\\[plugincategory\\]', pluginCategory),
                ('\\[menuname\\]', menuName),
                ('\\[menuitemname\\]', menuItemName)]

for file in files:

    # read contents of the file
    f = open(file)
    content = f.read()
    f.close()

    # replace everything necessary
    for repl in replacements:
        content = re.sub(repl[0], repl[1], content)

    # write changes to the file
    f = open(file, "w")
    f.write(content)
    f.close()


# Add an entry to src/plugins/CMakeLists.txt
f = open('CMakeLists.txt', 'a')
f.write('\nSUBDIRS ('+pluginDir+')\n')
f.close()

print "Your plugin %s has been created in %s, CMakeLists.txt has been modified." % (pluginName, pluginDir)
print
print "Once your plugin has successfully built, please see %s/README for" % pluginDir
print "hints on how to get started."

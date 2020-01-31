
# Change log for JoyShockMapper
Most recent updates will appear first.
This is a summary of new features and bugfixes. Read the README to learn how to use the features mentioned here.

## 1.4.1
1.4.1 is a bugfix update, with some minor features as well. Jibb improved flick stick's behaviour when the stick is released slowly, added the BACKSPACE mapping, and made the list of available configs update whenever the tray menu is re-opened. Nicolas added a "Whitelist" toggle to the tray menu.

### Features
* Added BACKSPACE mapping.
* Added buffer between flick stick activation threshold and release threshold.
* Added "Whitelist" toggle to tray menu.
* Tray menu items are refreshed whenever the menu is re-opened.

### Bugfixes
* Fixed combined tap mappings on ZL and ZR not releasing correctly.
* Fixed GYRO_INVERT overlapping ENTER keyboard mapping.
* Fixed outer ring bindings not working properly.
* Configs shortcuts in tray menu now use relative paths instead of absolute paths.
* Minor fixes to the console text.
* Fixed stick directional mappings to buttons not working as they should.

## 1.4.0
In 1.4, Nicolas added simultaneous and chorded press mappings, ring bindings, and options to map a button to inverting gyro input. He also added a HELP shortcut to the latest version of the README and a tray icon (created by Bryan and coloured by Al) that gives easy access to configs and useful commands when JSM is minimised. Jibb added Bluetooth support for the DualShock 4.

### Features
* Simultaneous press - map a pair of inputs pressed at about the same time to a unique output.
* Chorded press - change the mappings of one or more buttons while a particular other button is pressed.
* Bluetooth support for the DualShock 4.
* Ring bindings - have a virtual input apply when either stick is fully pressed or only partially pressed, such as for walking or sprinting.
* Invert gyro - have the gyro mouse inverted (in both axes or one axis of your choice) while pressing a button.
* GYRO\_OFF, GYRO\_ON can appear on the right hand side of regular mappings for combining them with other inputs in interesting ways. Gyro-related mappings bound to a button tap will apply for 0.5s to give them time to be useful.
* Whitelisting - add or remove JoyShockMapper to or from the HIDCerberus whitelist, if it's installed.
* Tray icon - when minimised, JoyShockMapper has a tray icon that can be right-clicked to quickly access configs or useful commands.
* The HELP command will open the README.

## 1.3.0
In 1.3, Nicolas added AutoLoad and dual stage triggers, while Jibb fixed a couple of bugs.

### Features
* AutoLoad - automatically load the appropriate config when an application comes into focus.
* Dual Stage Triggers - map soft presses and full presses of the DualShock 4's triggers to different outputs.

### Bugfixes
* Fixed SL and SR not working properly on Joy-Cons.
* Fixed hold NONE mappings not working.

## 1.2.0
In 1.2, Jibb added features to help with single Joy-Con control as well as more keyboard mappings and better comment support.

### Features
* Gyro axes can be mapped to different mouse axes using MOUSE\_X\_FROM\_GYRO\_AXIS and MOUSE\_Y\_FROM\_GYRO\_AXIS.
* HOME and CAPTURE can now be mapped to any output. Any button can be mapped to CALIBRATE.
* PAGEUP, PAGEDOWN, HOME, END, INSERT, and DELETE keyboard mappings were added.
* /# comments can be added at the end of a line, instead of requiring their own line.

## 1.1.0
In 1.1, Jibb added more ways to enable or disable the gyro, changed the default behaviour of calibration, and fixed a couple of bugs.

### Features
* GYRO\_OFF and GYRO\_ON can be set to LEFT\_STICK or RIGHT\_STICK so that the gyro can be enabled or disabled depending on whether a given stick is being used.
* Continuous calibration is now disabled when a device is first connected, since sometimes devices don't need to be calibrated on startup.

### Bugfixes
* Fixed d-pad up being the gyro off button when none was set.
* Fixed a crash when a bad command was entered.

## 1.0.2
1.0.2 is a bugfix update.

### Bugfixes
* Fixed a bug where arrow keys couldn't be mapped properly.

## 1.0.1
1.0.1 is a bugfix update.

### Bugfixes
* Statically linked runtime so that users don't have to have any particular MSVC runtimes installed.

## 1.0
JoyShockMapper 1.0 was the first public release of JoyShockMapper, created by Jibb Smart. Its features are too many to list in the changelog, but explore the README to see what it offers!

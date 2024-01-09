
# Change log for JoyShockMapper
Most recent updates will appear first.
This is a summary of new features and bugfixes. Read the README to learn how to use the features mentioned here.

## Known issues
* SDL always merges joycons behind the scene into a single controller. JSM is not able to split them. Use legacy-JSL version to use this feature.

## 3.5.3

### Features
* Update SDL2 to latest 2.28.5

### Bugfixes
* Fixed Instant Press not applying properly
* Fixed a deadlock with releasing a virtual controller

## 3.5.2

### Bugfixes
* Fix toggle (again (again))
* Exempt ADAPTIVE_TRIGGER and RUMBLE from RESET_MAPPINGS
* Fix MOUSE_RING location bug
* Fix armor x pro virtual touchpad button mapping

## 3.5.1

Nick fixes everything he broke with his "improvements"

### Bugfixes
* Restore digital binding of virtual gamepad trigggers X_LT, X_RT, PS_L2 AND PS_R2.
* Fix turbo binding of virtual controller buttons. Use Toggle as well for 50-50 press time instead of 40ms.
* Fix Dualsense Edge new button bindings (left buttons were inverted: LSL and RSR are grips)
* Add VIRTUAL_CONTROLLER and HIDE_MINIMIZED as settings exempted from resetting on RESET_BINDINGS
* Restore scroll stick mode
* Fix Ds4 virtual controller connect with UP stuck
* Fix Ds4 virtual controller touch adding a second ghost touch at 0,0
* Fix Ds4 virtual controller HOME and CAPTURE buttons
* Robustness fix at cleanup
* Fix Armor X Pro PS mode's M2 button

## 3.5.0

Nick updates C++ standard to C++20, Add Setting Manager, Create Stick and Autoload objects, Update ViGEm, SDL2 and JSL dependencies to latest,
which includes Sony Edge support for both SDL and JSL versions. Perform proper polymorphism of virtual gamepads.

### Features
* New HYBRID_AIM stick mode. This is the first feature developped by a community member : somebelse-Schmortii. It combines AIM and MOUSE_AREA. See readme for a ton of options and description
* New virtual DS4 forwarding options: GYRO_OUTPUT=PS_MOTION, TOUCHPAD_MODE=PS_TOUCHPAD

### Bugfixes
* Set flick threshold at 100% (so that it is precisely controllerd by the outer deadzone setting)
* Virtual buttons now work properly with event and action modifiers (toggle, hold, turbo, etc...)
* Fix joycons in SDL2
* Modeshift-ing out of a flick will lock the stick mode in FLICK_ONLY until the flick is completed


## 3.4.0

Nicolas updated SDL2 version to the latest release 2.0.20 and fixed some bugs
NON BACKWARD COMPATIBLE CHANGE: Digital trigger bindings cannot be used at the same time as a virtual controller analog output, but the chord stack is updated to make use of chords and gyro button.
Jibb has made some improvements to vehicle steering, including stick modes for converting an angle on the stick to a single-axis offset on a virtual stick, a large rotation of the stick to a single-axis offset on a virtual stick, and a way to map MOTION_STICK to a single steering axis.

### Features
* Any STICK_MODE can now be set to LEFT_ANGLE_TO_X, LEFT_ANGLE_TO_Y, RIGHT_ANGLE_TO_X, or RIGHT_ANGLE_TO_Y to convert the angle of the stick to a virtual stick offset in just that axis. Requires a virtual controller to be set. This can be useful for improved steering precision in games that only use one axis. For example, setting LEFT_STICK_MODE to LEFT_ANGLE_TO_X will let you point the stick forward to get out of the inner deadzone without actually steering left or right, and then make small adjustments left or right for gentle steering. Stick magnitude is still taken into account, so at any time you can choose to just keep the stick in the X axis and it'll behave the same as traditional single-axis stick steering.
* Any STICK_MODE can now be set to LEFT_WIND_X or RIGHT_WIND_X, letting you wind that stick (rotate it flick-stick-style) to move that virtual stick across a single axis. This can be used to map very large movements to small stick movements, which can be useful for playing driving simulators. Releasing the stick will let the virtual stick pull back to its neutral position.
* MOTION_STICK_MODE can be set to LEFT_STEER_X or RIGHT_STEER_X to map leaning the controller left and right to the X axis on a virtual controller stick. For steering, this can work better than setting MOTION_STICK_MODE to a regular stick since it naturally handles holding your controller upright, flat, or anything in-between.

### Bugfixes
* Autoload could not get started again, after being stopped
* A controller trigger key could not get pressed
* The full pull binding never registered when using analog controller triggers
* Fix hold time being applied on the first turbo hit
* Fix to direction keys not working in some games.
* Fix ViGEm rumbling not working


## 3.3.0

Jibb added basic support for mapping gyro and flick stick to virtual controller sticks.
Nicolas is adding Nielk1's trigger effect to JSM.

### Features
* GYRO_OUTPUT can be set to RIGHT_STICK to convert gyro to stick instead of mouse. Find it in the README to see other settings available, including "UNDEADZONE", "UNPOWER", and "VIRTUAL_SCALE".
* FLICK_STICK_OUTPUT can also be set to RIGHT_STICK to fake flick stick in a game that's only reading from the controller. It will usually be less precise than mouse-flick-stick.
* Tune virtual flick stick and gyro aiming with VIRTUAL_STICK_CALIBRATION instead of REAL_WORLD_CALIBRATION and IN_GAME_SENS.
* ZL and ZR bindings now work with alongside virtual stick bindings (setting ZL_MODE or ZR_MODE to a virtual conroller trigger).
* New settings LEFT_TRIGGER_EFFECT and RIGHT_TRIGGER_EFFECT can be set to RESISTANCE, BOW, GALLOPING, SEMI_AUTOMATIC, AUTOMATIC or MACHINE. Type LEFT_TRIGGER_EFFECT HELP for details

### Bugfixes
* Fixed virtual DS4 left stick having a heavy bias to the corners.
* Analog output of triggers are registered as chords to enable gyro button and chords but not digital mappings
* Fixing the oldest bug in the history of JSM: numpad and navigational buttons depending on num lock

## 3.2.3

### Bugfixes
* Fixed RSL and RSR not registering

## 3.2.3

Nicolas is adding separate axis inversion settings for each stick. Like other XY settings, one or two parameters can be provided if X and Y axis don't have the same value. Legacy STICK_AXIS_X and STICK_AXIS_Y are preserved but operate on the new settings.
Nicolas also pushed some bugfixes

### Features
* New settings LEFT_STICK_AXIS, RIGHT_STICK_AXIS, MOTION_STICK_AXIS and TOUCH_STICK_AXIS can be set to STANDARD or INVERTED. A different vertical value can be set by providing a second value.

### Bugfixes
* Missing TOUCH_DEADZONE_INNER and TOUCH_RING_MODE commands
* Multiple sources of virtual controller stick and trigger inputs will add up instead of override.
* README didn't point to the official readme
* Don't send adaptive trigger data to non DS controllers... herp de derp

## 3.2.2

Jibb fixed some bugs related to motion stick and lean bindings.
Nicolas fixed some bugs too and makes use of the official SDL2 repo and GamepadMotionHelpers main branch
TauAkiou helped fix some linux build and updated the README
Github user mmmaisel added some better error handling of the linux build

### Features
An active toggle on the DualSense mic button will turn on the mic light until that toggle is cleared by another toggle of MIC or that binding being released.

### Bugfixes
* SET_MOTION_STICK_NEUTRAL should now behave correctly instead of accumulating errors over time.
* Motion stick and lean bindings should respect gravity more readily. This will also improve WORLD_TURN and WORLD_LEAN behaviour for those who use either of those GYRO_SPACE settings.
* GYRO_SMOOTH_TIME hasn't been working correctly since moving to a fixed tick rate (3.0). This has now been fixed.
* Stick aim inversion didn't work becuase it was inverted twice
* Linux easter egg fix (What is VK_NONAME? What does it do?)
* Linux error handling improvement (thanks mmmaisel)
* RIGHT_RING_MODE was confounded with LEFT_RING_MODE
* Fix the numkeys sending the alternate binding (home end pageup pagedown ...) and vice versa
* README linux notes updated (Thanks TauAkiou)

## 3.2.1

**Backwards compatibility note!** The GYRO_SPACE options WORLD_TURN and WORLD_LEAN have been renamed PLAYER_TURN and PLAYER_LEAN. "Player space", which is was introduced in 3.2.0 (and has been improved here), is different to "world space". WORLD_TURN and WORLD_LEAN now refer to slightly different options that use the gravity direction more strictly, and as a result are more prone to error.

### Bugfixes
* PLAYER_TURN and PLAYER_LEAN behaviours have been improved.
* Virtual Xbox and DS4 triggers can be bound to digital buttons.
* Easter Egg. Can you find it?

## 3.2.0

Jibb added the new GYRO_SPACE setting for more one-size-fits-all gyro aiming. Default behaviour is unchanged, but set to WORLD_TURN to try the new algorithm (or WORLD_LEAN if you prefer to lean your controller side to side to turn the camera). He also added the option for automatic gyro calibration.

### Features

* GYRO_SPACE can be set to LOCAL (default), WORLD_TURN (recommended), or WORLD_LEAN.
* AUTO_CALIBRATE_GYRO can be set to ON to activate automatic calibration, which will try and detect when the controller is held still or put down and recalibrate automatically.

### Bugfixes

* Fixed crash when SDL ignores a connected controller
* Fixed crash when ZRF gets pressed

## 3.1.1

Fix linux build (Thanks TauAkiou)
Add Addaptive trigger calibration procedure and settings

### Features

* New Settings: LEFT_TRIGGER_OFFSET, LEFT_TRIGGER_RANGE, RIGHT_TRIGGER_OFFSET, RIGHT_TRIGGER_RANGE
* New macro command CALIBRATE_TRIGGERS starts a trigger calibration procedure. See README

### Bugfixes

* Fix instant presses not releasing
* Fix TOUCH binding flickering when the pad would be clicked but not touched

## 3.1.0

SDL build of JSM has no label, and JSL build is marked as "legacy"
Dualsense Adaptive trigger support, MIC button support
DS and DS4 touchpad support. See features below
HidHide support and improved error logs
DBL_PRESS_WINDOW is now a modeshift-able setting and hold time is not restricted by it anymore
The first press of double press enables mapping. See README
Tons of refactoring and code improvements in preparation for unit tests
Gestures from the beta has been disabled

### Features

* Touchpad mouse, grid (1-25 buttons) and stick available
* New settings related to touchpad features
  * TOUCHPAD_MODE sets to either MOUSE or GRID_AND_STICK
  * GRID_SIZE takes rows and colums (max grid 25 buttons)
  * TOUCH_STICK_MODE is a regular stick mode for a touch stick
  * TOUCH_STICK_DEADZONE_INNER, TOUCH_STICK_RADIUS, TOUCH_RING_MODE
  * TOUCHPAD_SENS controls the mouse mode sensitivity
  * ADAPTIVE_TRIGGER switch can turn trigger resistance off: hair trigger is not available on DS when Adaptive Triggers are on.
* New MIC binding used by DS only
* Dualsense applies adaptive trigger setting based on trigger mode and state
* HidHide support: hidhide remembers applications, so you don't need to add to whitelist at every start
* DBL_PRESS_WINDOW default value is now same os hold press : 150 ms

### Bugfixes

* Gyro active toggle didn't clear after another gyro release
* An active toggle hides subsequent presses of the same key
* Controller type didn't report properly
* Tray menu loads a relative path to minimize pathing issue
* Fixed an issue in the beta where adaptive triggers were "ticking"

## 3.0.2

Reverting blocked key presses and bugfix

### Bugfixes

* Crash occurs when another software creates vigem controllers and SDL2 tries to connect to it.

## 3.0.1

Bugfixes and console improvements

### Features

* CLEAR command cleans up the console window
* JSM blocks key presses into it's own terminal window

### Bugfixes

* Inverted stick Y axis on the JSL version of JSM
* Turbo press does not release without the start press binding


## 3.0.0
Jibb added JSL-specific features to SDL2 so that JSM could use SDL2 instead. This means support for many non-gyro controllers, including Xbox, Stadia, and almost every common generic PC controller. Also made it so that low report-rate controllers (eg Switch controllers) are sampled multiple times for smooth gyro on high refresh rate monitors.

**SDL2 is now the official version of JSM going forward.** JSM will also be released with JSL for the time being in case there are features there unavailable in SDL2 yet.

Nicolas added ViGEm support for virtual xbox and DS4 for buttons, triggers and sticks as well as rumble forwarding.

Added colored console lines

Added rumble commands and DS4 light bar setting and TOUCH binding

Added DS4 dual stage mode for toupad touch and click.

Added hair trigger soft press as a negative threshold and trigger modes SCROLL_WHEEL and NO_SKIP_EXCLUSIVE

Added ability to hide JSM from the taskbar when minimized, with a checkbox in the tray icon

Added help strings for button mapping

Added argument to RECONNECT_CONTROLLERS [MERGE|SPLIT] used to determine joycon behaviour

Added handler to modeshift a gyro button to NONE\ as no button since NONE is used to remove the modeshift.

Handle drag n drop files into the console better

Improve command error handling

Improved README separation

Improved desktop recommended config


### Features
* New Bindings: TOUCH for the touch pad and dual stage mode for TOUCH and CAPTURE bindings
* New Mappings for ViGEm controller bindings. See README
* Added Mappings SMALL_RUMBLE, BIG_RUMBLE and Rhhhh (h being hex digits)
* Added settings STICK_DEADZONE_INNER|OUTER, TICK_TIME (aka polling period), LIGHT_BAR, SCROLL_SENS, VIRTUAL_CONTROLLER, RUMBLE ( = ON|OFF) and TOUCHPAD_DUAL_STAGE_MODE
* Added JOYCON_SIDEWAYS as a controller orientation
* Added Stick Mode SCROLL_WHEEL
* Added TriggerMode NO_SKIP_EXCLUSIVE
* Assigning a negative value to trigger threshold enables hair trigger
* New setting HIDE\_MINIMIZED will hide JSM when set to ON. OFF is default
* Support for many non-gyro controllers: Xbox, Stadia, GameCube, PS3 (without motion), and many generic PC controllers via SDL2

## 2.2.0
Nicolas added more keybinds. Robin fixed issues with building on Linux and improved PlayStation controller support.

### Features
* DualSense is now also supported when connected by Bluetooth.
* New keybinds include: volume control, media control, windows keys and context menu key.

### Bugfixes
* DualShock 4's gyro should now be correctly activated even when connected by Bluetooth.

## 2.1.0
Jibb added experimental support for the DualSense controller (PlayStation 5) when connected by USB.

### Features
* The DualSense controller is now supported if connected by USB.

## 2.0.3
More bugfixes for bugs introduced by bugfixes, thanks to Nicolas. :P

### Bugfixes
* Gyro button fix. Bug was introduced by joycon fix. Sorry about that.

## 2.0.2
More bugfixes thanks to Nicolas.

### Bugfixes
* The HELP command will now work correctly even after looking at the HELP for a specific command.
* GYRO\_ON / GYRO\_OFF behaviour with split Joy-Cons has been improved.
* Fix display of descriptive Mappings.
* Fix issue where multiple toggles on a single button wouldn't work properly.
* Updated the "2.0.0" section of the CHANGELOG to show that stick sensitivity can be set separately in each axis.

## 2.0.1
Nicolas fixed some small hiccups with the new binding modifiers.

### Bugfixes
* The hold binding modifier should now be recognised correctly.
* Restored CALIBRATE automatically working as a toggle when a tap modifier is used.

## 2.0.0
Nicolas refactored the command processing engine of JSM as well as some of the button processing logic. This enables a richer command line interface and new button mapping features.
These changes will also make adding more settings less error prone and provide a more consistent feeling in the interface.

Nicolas also expanded the mapping options greatly, allowing a button to map to multiple key presses, a key press on press and on release, toggling, and more. He also made hold/simultaneous/double-press times configurable, and made it so any JSM command (including file loading) can be bound to a button press by putting it in quotes. The JSM directory can be set, which should help fix problems for those accessing JSM through a shortcut. A *onreset.txt* file can be automatically loaded when RESET\_MAPPINGS is called, and the current working directory can be set from a command line argument when running JSM.

Jibb added motion stick options, exposing the orientation of the controller as a third stick. This third stick can do everything a regular stick can -- MOUSE\_AREA, FLICK\_STICK, or trigger key presses. He also added separate lean mappings so the controller can be leaned left or right to trigger key inputs. Stick modes can be made to work correctly when holding the controller sideways or backwards, and there's a new trackball binding option to help reset controller position. A *onstartup.txt* file can be automatically loaded with preferred settings, and a SLEEP command can help automate calibration.

Roy Straver added an optional forward deadzone for flick stick to help with engaging the stick for rotation when no flick is desired, and the option to make flick time proportional to the size of the flick when using flick stick.

Garrett added separate X and Y sensitivities for traditional stick aiming.

### Features
* All commands can now display help, or display their current value, have value filtering and notification.
* Bindings can now have an action modifier (toggle/instant) and/or an event modifier (on press/release/tap/hold/turbo).
* A button mapping can now enable multiple key presses.
* WHITELIST\_SHOW displays a link to the HIDCerberus console instead of opening it (security risk).
* HELP command was renamed to README (it displays a web link to the latest README).
* New HELP command shows a list of all commands or the help of all queried commands.
* New settings added for button timings, HOLD_PRESS_TIME, SIM_PRESS_WINDOW and DBL_PRESS_WINDOW.
* New setting JSM_DIRECTORY should help solve pathfinding issues with AutoLoad, by making the current working directory changeable.
* Added possibility to bind any JSM command as an action by entering it within quotes. This enables the possibility to load a file on button press.
* MOTION\_STICK\_MODE treats the whole controller's orientation as a stick, and can be set to anything LEFT\_STICK\_MODE can be set to.
* LEAN\_LEFT and LEAN\_RIGHT can map leaning the controller left or right to key inputs.
* LEFT\_STICK\_DEADZONE\_\* and RIGHT\_STICK\_DEADZONE\_\* can be set independently.
* CONTROLLER\_ORIENTATION changes the behaviour of sticks to work correctly when holding the controller sideways or backwards.
* GYRO\_TRACKBALL can be bound as an alternative to GYRO\_OFF, GYRO\_INVERT, etc. This will maintain the gyro's last velocity while held, and will decay according to the TRACKBALL\_DECAY setting.
* FLICK\_DEADZONE\_ANGLE defines a no-flick zone at the front of the stick for flick stick.
* FLICK\_TIME\_EXPONENT determines how the size of the flick angle affects the time it takes to complete a flick.
* If a file called *onstartup.txt* is found in the working directory, its contents will be loaded on startup. Use this for disabling AutoLoad or automatically whitelist and reconnect controllers.
* If a file called *onreset.txt* is found in the working directory, its contents will be loaded right after startup and whenever RESET_MAPPINGS is called.
* SLEEP will wait for a given number of seconds (up to 10).
* Added support for the keypad's operator keys, decimal key and caps lock key.
* STICK\_SENS can accept two different sensitivities to set X and Y to different sensitivities.

### Bugfixes
* JSM should no longer sometimes get the mouse stuck on the side or top of the screen.

## 1.6.1
Lots of internal changes for developers. JSM can now be built for Linux, thanks to Romeo Calota. Since this is only developer-facing for now, this is still just a bug-fix update rather than a feature update. But if you're up for it, check out the Linux instructions in the README!

Regarding bugs, Nicolas fixed some bugs with disabling gyro and stick behaviour during mode shifts. Jibb added support for wired Switch Pro Controller (technically a new feature but the lack of support was a stumbling block for many new users) and made some changes to the DualShock 4 Bluetooth support that will hopefully fix issues some users have been having.

### Bugfixes
* Improved support for DualShock 4 and Switch Pro Controller.
* Fixed an issue with stick behaviour and mode shift.
* Fixed d-pad up always acting as a gyro off button even when it wasn't assigned.

## 1.6.0
Nicolas added modeshifts to JSM and all setting variables are now encapsulated in a structure. Therefore, to use any setting you need to query it through the accessor in Joyshock. All settings are now "optional" but should always be set in the base structure by the reset() function. Instances that are part of the modeshift map can have nullopt to indicate no alternate value for the setting when that modeshift is active. Nicolas also added MOUSE_AREA for a stick mode, which is useful for using mouse wheels in game where the wheel is not centered on the screen (in which case MOUSE_RING would work). See README for details.

### Features
* Added the ability to chord any setting (except autoload, and including NO_GYRO_BUTTON).
* Modeshifts are removed by assigning NONE.
* Added MOUSE_AREA as a stick mode.

## 1.5.1
Nicolas changed the tray icon to always be displayed. 

### Bugfixes
* The one second pause when exiting JSM was not necessary.

## 1.5.0
Nicolas added double press bindings, improved chorded mapping behaviour when combined with taps and holds, and refactored a lot of code to prepare for some future changes. Also added support for mouse buttons forward and back, separate horizontal and vertical gyro sensitivities, and changed the way logs are displayed. Jibb added new ways to configue flick stick: customise flick stick's smoothing, disable some of its features, or snap to angles. Also made it so ring bindings work alongside any stick mode and added the MOUSE\_RING stick mode for 2D twin-stick aiming.

### Features
* Added ability to assign Double Press mappings to a button, by entering the button chorded with itself (eg: S,S = SPACE).
* Chords are now active when the controller button is down, instead of waiting for a bounded input to be resolved (such as taps and holds).
* Support mouse buttons 4 and 5 (back and forward).
* Horizontal and vertical gyro sensitivities can now be set independently by including a second number for the Y sensitivity (first will be used for X).
* Added MOUSE\_RING stick mode to let you use the stick to point the mouse in a direction relative to the centre of the screen.
* Added new stick modes enabling only the flick (FLICK\_ONLY) or rotation (ROTATE\_ONLY) with flick stick.
* Added FLICK\_SNAP\_MODE and FLICK\_SNAP\_STRENGTH for those who'd prefer flick stick snapped to cardinal (or intercardinal) directions with the initial flick.
* LEFT\_RING\_MODE and RIGHT\_RING\_MODE = INNER/OUTER allows setting ring bindings regardless of what LEFT_STICK_MODE and RIGHT_STICK_MODE are.
* Added the option to override the smoothing threshold for flick stick with ROTATE\_SMOOTH\_OVERRIDE. The smoothing window is still small, but it might soften things for those who found Switch sticks too twitchy.
* Change some behind the scene mapping of commands to windows virtual key codes.

## 1.4.2
1.4.2 is a bugfix update. Nicolas fixed a crash and combo (aka Simultaneous and Chorded) presses not clearing properly, as well as some under-the-hood tweaks. Jibb tweaked communication with Bluetooth DualShock 4.

### Bugfixes
* Fixed crash when left clicking the tray icon.
* RESET\_MAPPINGS should now clear combo mappings.
* Setting combo presses to NONE should now clear previous bindings.
* Changes to how Bluetooth works with DualShock 4 controllers might fix issues some had with gyro not working.

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
In 1.4, Nicolas added simultaneous and chorded press mappings, ring bindings, and options to map a button to inverting gyro input. He also added a HELP shortcut to the latest version of the README and a tray icon (created by Bryan and coloured by Contributer) that gives easy access to configs and useful commands when JSM is minimised. Jibb added Bluetooth support for the DualShock 4.

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
JoyShockMapper 1.0 was the first public release of JoyShockMapper, created by Jibb Smart. It includes the original implementation of flick stick and the first example of gyro controls meeting the standards detailed on GyroWiki (including advanced options like two-sensitivity acceleration and soft-tiered smoothing). Other features include tap and hold button bindings, rich traditional stick aiming options, and a "real world calibration" setting that made it possible to use the same stick and gyro settings across different games.

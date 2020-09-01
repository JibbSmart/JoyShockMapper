
# JoyShockMapper
The Sony PlayStation DualShock 4, Nintendo Switch JoyCons (used in pairs), and Nintendo Switch Pro Controller have much in common. They have many of the features expected of modern game controllers. They also have an incredibly versatile and underutilised input that their biggest rival (Microsoft's Xbox One controller) doesn't have: a 3-axis gyroscope (from here on, “gyro”).

My goal with JoyShockMapper is to enable you to play PC games with DS4, JoyCons, and Pro Controllers even better than you can on their respective consoles -- and demonstrate that more games should use these features in these ways.

**Download JoyShockMapper to use right away [here](https://github.com/JibbSmart/JoyShockMapper/releases)**!

For developers, this is also a reference implementation for using [JoyShockLibrary](https://github.com/jibbsmart/JoyShockLibrary) to read inputs from DualShock 4, JoyCons, and Pro Controller in your games. It's also a reference implementation for many of the best practices described on [GyroWiki](http://gyrowiki.jibbsmart.com).

JoyShockMapper works on Windows and uses JoyShockLibrary to read inputs from controllers. JoyShockMapper should now be able to be built on and for Linux. See the instructions for that below. Please let us know if you have any trouble with this.

## Contents
* **[Installation for Devs](#installation-for-devs)**
* **[Installation for Players](#installation-for-players)**
* **[Quick Start](#quick-start)**
* **[Commands](#commands)**
  * **[Digital Inputs](#1-digital-inputs)**
    * **[Tap & Hold](#11-tap--hold)**
	* **[Binding Modifiers](#12-binding-modifiers)**
	* **[Simultaneous Press](#13-simultaneous-press)**
	* **[Chorded Press](#14-chorded-press)**
	* **[Double Press](#15-double-press)**
	* **[Gyro Button](#16-gyro-button)**
  * **[Analog Triggers](#2-analog-triggers)**
  * **[Stick Mouse Inputs](#3-stick-mouse-inputs)**
    * **[Motion Stick](#31-motion-stick)**
  * **[Gyro Mouse Inputs](#4-gyro-mouse-inputs)**
  * **[Real World Calibration](#5-real-world-calibration)**
  * **[Modeshifts](#6-modeshifts)**
  * **[Miscellaneous Commands](#7-miscellaneous-commands)**
* **[Known and Perceived Issues](#known-and-perceived-issues)**
* **[Troubleshooting](#troubleshooting)**
* **[Credits](#credits)**
* **[Helpful Resources](#helpful-resources)**
* **[License](#license)**

## Installation for Devs
JoyShockMapper was written in C++ and is built using CMake.

The project is structured into a set of platform-agnostic headers, while platform-specific source files can be found in their respective subdirectories.
The following files are platform-agnostic:
1. ```include/JoyShockMapper.h``` - This header provides important type definitions that can be shared across the whole project. No variables are defined here, only constants.
2. ```include/InputHelpers.h``` - This is platform agnostic declaration of wrappers for OS function calls and features.
3. ```include/PlatformDefinitions.h``` - This is a set of declarations that create a common ground when dealing with platform-specific types and definitions.
4. ```include/TrayIcon.h``` - This is a self contained module used to display in Windows an icon in the system tray with a contextual menu.
5. ```include/Whitelister.h``` - This is another self contained Windows specific module that uses a socket to communicate with HIDCerberus and whitelist JSM, the Linux implementation, currently, is a stub.
6. ```include/CmdRegistry.h``` - This header defines the base command type and the CmdRegistry class that processes them.
7. ```include/JSMAssignment.hpp``` - Header for the templated class assignment commands
8. ```include/JSMVariable.hpp``` - Header for the templated core variable class and a few derivatives.
9. ```src/main.cpp``` - This does just about all the main logic of the application. The core processing logic should be kept in the other files as much as possible, and have the JSM specific logic in this file.
10. ```src/CmdRegistry.cpp``` - Implementation for the command line processing entry point.
11. ```src/operators.cpp``` - Implementation of all streaming and comparison operators for custom types declared in ```JoyShockMapper.h```

The Windows implementation can be found in the following files:
1. ```src/win32/InputHelpers.cpp```
2. ```src/win32/PlatformDefinitions.cpp```
3. ```src/win32/Whitelister.cpp.cpp```
4. ```include/win32/WindowsTrayIcon.h```
5. ```src/win32/WindowsTrayIcon.cpp```

The Linux implementation can be found in the following files:
1. ```src/linux/InputHelpers.cpp```
2. ```src/linux/PlatformDefinitions.cpp```
3. ```src/linux/Whitelister.cpp.cpp```
4. ```include/linux/StatusNotifierItem.h```
5. ```src/linux/StatusNotifierItem.cpp```

Generate the project by runnning the following in a command prompt at the project root:
- Windows:
  * ```mkdir build && cd build```
  * To create a Visual Studio x86 configuration: ```cmake .. -G "Visual Studio 16 2019" -A Win32 .```
  * To create a Visual Studio x64 configuration: ```cmake .. -G "Visual Studio 16 2019" -A x64 .```
- Linux:
  * ```mkdir build && cd build```
  * ```cmake .. -DCMAKE_CXX_COMPILER=clang++ && cmake --build .```

### Linux specific notes
In order to build on Linux, the following dependencies must be met, with their respective development packages:
- gtk+3
- libappindicator3
- libevdev

Due to a [bug](https://stackoverflow.com/questions/49707184/explicit-specialization-in-non-namespace-scope-does-not-compile-in-gcc) in GCC, the project in its current form will only build with Clang.

JoyShockMapper was initially developed for Windows, this has the side-effect of some types used through the code-base are Windows specific. These have been redefined on Linux. This was done to keep changes to the core logic of the application to a minimum, and lower the chance of causing regressions for existing users.

The application requires ```rw``` access to ```/dev/uinput```, and ```/dev/hidraw[0-n]``` (the actual device depends on the node allocated by the OS). This can be achieved by ```chown```-ing the required device nodes to the user running the application, or by applying the udev rules found in ```dist/linux/50-joyshockmapper.rules```, adding your user to the input group, and restarting the computer for the changes to take effect. More info on udev rules can be found at https://wiki.archlinux.org/index.php/Udev#About_udev_rules.

The application will work on both X11 and Wayland, though focused window detection only works on X11.

## Installation for Players
The latest version of JoyShockMapper can always be found [here](https://github.com/JibbSmart/JoyShockMapper/releases). All you have to do is run JoyShockMapper.exe.

Included is a folder called GyroConfigs. This includes templates for creating new configurations for 2D and 3D games, and configuration files that include the settings used for simple [Real World Calibration](#4-real-world-calibration).

## Quick Start
1. Connect your DualShock 4, JoyCons, or Pro Controller. Bluetooth support for the DualShock 4 is experimental for now, as is USB support for Switch controllers.

2. Run the JoyShockMapper executable, and you should see a console window welcoming you to JoyShockMapper.
    * If you want to connect your controller after starting JoyShockMapper, you can use the command RECONNECT\_CONTROLLERS to connect these controllers.
	* The ```HELP``` command will show you the list of all available commands. You can display help from multiple commands at once by typing ```HELP [cmd1] [cmd2] ...```
	* All settings can display their current value by typing entering it's name, and display their help by entering ```[cmd] HELP```
	* Bring up the latest version of this README in your browser by entering ```README```

3. Drag in a configuration file and hit enter to load all the settings in that file.
    * Configuration files are just text files. Every command in a text file is a command you can type directly into the console window yourself. See "Commands" below for a comprehensive guide to JoyShockMapper's commands.
    * Example configuration files are included in the GyroConfigs folder.
    * If the file path is very, very long, or contains unusual characters, **this may not work**. In that case, type in the relative path (eg: GyroConfigs/config.txt) and hit enter.

4. If you're using a configuration that utilises gyro controls, the gyro will need to be calibrated (to be told what "not moving" is). See "Gyro Mouse Inputs" under "Commands" below for more info on that, but here's the short version:
	* Put all controllers down on a still surface;
	* Enter the command RESTART\_GYRO\_CALIBRATION to begin calibrating them;
	* After just a couple of seconds, enter the command FINISH\_GYRO\_CALIBRATION to finish calibrating them.

5. A good configuration file has also been calibrated to map sensitivity settings to useful real world values. This makes it easy to have consistent settings between different games that have different scales for their own sensitivity settings. See [Real World Calibration](#4-real-world-calibration) below for more info on that.

6. JoyShockMapper can automatically load a configuration file for your games each time the game window enters focus. Drop the file in the **AutoLoad** folder, next to the executable. JoyShockMapper will look for a name based on the executable name of the program that's in focus. When it goes into focus and AUTOLOAD is enabled (which it is by default), JoyShockMapper will tell you the name of the file it's looking for - case insensitive. You can turn it off by entering the command ```AUTOLOAD = OFF```. You can enable it again with ```AUTOLOAD = ON```. If your files don't seem to get picked up, you can manually set where to look for the AutoLoad folder by entering the command ```JSM_DIRECTORY = C:\Program Files\JSM``` for example

7. Games that have native support for your controller, such as *Apex Legends* with DualShock 4, sometimes don't have the option to ignore your controller. Projects like HIDGuardian / HIDCerberus allow you to mask devices from all application except those that you *whitelist*. If you do have HIDCerberus installed and running, JoyShockMapper can be added to the whitelist by entering the command ```WHITELIST_ADD```, removed with the command ```WHITELIST_REMOVE```, and you can display the HIDCerberus configuration page in your browser by entering ```WHITELIST_SHOW```.

## Commands
Commands can be executed by simply typing them into the JoyShockMapper console windows and hitting 'enter'. You can also put a bunch of commands into a text file and, by typing in the path to the file in JoyShockMapper (either the full path or the path relative to the JoyShockMapper executable) and hitting 'enter', execute all those commands. I refer to such a file as a "configuration file". In Windows, you can also drag and drop a file from Explorer into the JoyShockMapper console window to enter the full path of that file.

A configuration file can also contain references to other configuration files. This can simplify sharing settings across different games -- if you have preferred stick and gyro settings, you can keep them in a separate file, and have all other configuration files reference that file instead of copying all the stick and gyro settings themselves.

Commands can *mostly* be split into 4 categories:

1. **[Digital Inputs](#1-digital-inputs)**. These are the simplest. Map a button press or stick movement to a key or mouse button. There are many binding options available, such as tap & hold, simultaneous press and chorded press.
2. **[Analog Triggers](#2-analog-triggers)**. The Dualshock 4 controller has 2 analog triggers: L2 and R2. JoyShockMapper can set different bindings on both "soft pull" and "full pull" of the trigger, maximizing use of those triggers. This feature is unavailable to controllers that have digital triggers, like the Nintendo Pro and Joycons.
3. **[Stick Mouse Inputs](#3-stick-mouse-inputs)**. You can move the mouse with stick inputs and/or gyro inputs. Stick mouse has two different modes:
	* **Aim stick**. This is your traditional/legacy stick aiming.
	* **Flick stick**. Map the flick or rotation of a stick to the same rotation in game. More on that later.
4. **[Gyro Mouse Inputs](#4-gyro-mouse-inputs)**. Controlling the mouse with gyro generally provides far more precision than controlling it with a stick. Think of a gyro as a mouse on an invisible, frictionless mousepad. The mousepad extends however far you're comfortable rotating the controller. For games where you control the camera directly, stick mouse inputs provide convenient ways to complete big turns with little precision, while gyro mouse inputs allow you to make more precise, quick movements within a relatively limited range.
5. **[Real World Calibration](#5-real-world-calibration)**. Calibrating correctly makes it possible for *flick stick* to work correctly, for the gyro and aim stick settings to have meaningful real-life values, and for players to share the same settings between different games.
6. **[Modeshifts](#6-modeshifts)**. Various settings can be reconfigured depending on the controller's current button presses, in a way akin to chorded presses. This is handy to handle weapon wheels for example. These are called modeshifts to echo the Steam Input naming convention.
7. **[Miscellaneous Commands](#7-miscellaneous-commands)**. These don't fit in the above categories, but are nevertheless useful.

So let's dig into the available commands.

### 1. Digital Inputs
Digital inputs are really simple. They are structured mostly like the following:

```[Controller Input] = [Key or Mouse Button]```

For example, to map directional pad LEFT to the F1 key, you'd enter:

```LEFT = F1```

One important feature of JoyShockMapper is that a configuration that works for the DualShock 4 works the same for a pair of JoyCons or a Pro Controller. Because JoyCons can have slightly more inputs than the DualShock 4 (the ```SL``` and ```SR``` buttons are unique to the JoyCons), the button names are *mostly* from the Nintendo devices. The main exceptions are the face buttons and the stick-clicks. Because they are named more concisely, the stick-clicks are named after the DualShock 4: ```L3``` and ```R3```.

The face buttons are a more complicated matter.

The Xbox layout has become the defacto layout for PC controllers. Most PC gamers who use some sort of controller will be familiar with the Xbox layout, whether from Xbox controllers, Steam controller, or other 3rd party controllers that can be interpreted by a game as an Xbox controller. Even DualShock 4 users will be somewhat used to interpreting Xbox face button names. Nintendo devices have the *same* face buttons in a *different* layout. X and Y are swapped, and so are A and B. Nintendo's layout has also been around for longer, but is less familiar to PC players.

So the best solution, in my opinion, is to use *neither* layout, and use an unambiguous layout with button names that aren't used by *any* controller, but still have obvious positions: the *cardinal layout*. North, East, South, West, denoted by ```N```, ```E```, ```S```, ```W```, respectively.

So, here's the complete list of digital inputs:

```
UP: Up on the d-pad
DOWN: Down on the d-pad
LEFT: Left on the d-pad
RIGHT: Right on the d-pad
L: L1 or L, the top left shoulder button
ZL: L2 or ZL, the bottom left shoulder button (or trigger)
R: R1 or R, the top right shoulder button
ZR: R2 or ZR, the bottom right shoulder button
ZRF: Full pull binding of right trigger, only on DS4
ZLF: Full pull binding of left trigger, only on DS4
-: Share or -
+: Options or +
HOME: PS or Home
CAPTURE: Touchpad click or Capture
SL: SL, only on JoyCons
SR: SR, only on JoyCons
L3: L3 or Left-stick click
R3: R3 or Right-stick click
N: The North face button, △ or X
E: The East face button, ○ or A
S: The South face button, ⨉ or B
W: The West face button, □ or Y
LUP: Left stick tilted up
LDOWN: Left stick tilted down
LLEFT: Left stick tilted left
LRIGHT: Left stick tilted right
LRING: Left ring binding, either inner or outer.
RUP: Right stick tilted up
RDOWN: Right stick tilted down
RLEFT: Right stick tilted left
RRIGHT: Right stick tilted right
RRING: Right ring binding, either inner or outer.
MUP: Motion stick tilted forward
MDOWN: Motion stick tilted back
MLEFT: Motion stick tilted left
MRIGHT: Motion stick tilted right
MRING: Motion ring binding, either inner or outer.
LEAN_LEFT: Tilt the controller to the left
LEAN_RIGHT: Tilt the controller to the right
```

These can all be mapped to the following keyboard and mouse inputs:

```
0-9: number keys across the top of the keyboard
N0-N9: numpad number keys
F1-F29: F1, F2, F3... etc
A-Z: letter keys
UP, DOWN, LEFT, RIGHT: the arrow keys
LCONTROL, RCONTROL, CONTROL: left Ctrl, right Ctrl, generic Ctrl, respectively
LALT, RALT, ALT: left Alt, right Alt, generic Alt, respectively
LSHIFT, RSHIFT, SHIFT: left Shift, right Shift, generic Shift, respectively
TAB, ESC, ENTER, SPACE, BACKSPACE
PAGEUP, PAGEDOWN, HOME, END, INSERT, DELETE,
LMOUSE, MMOUSE, RMOUSE: mouse left click, middle click and right click respectively
BMOUSE, FMOUSE: mouse back (button 4) click and mouse forward (button 5) click respectively
SCROLLUP, SCROLLDOWN: scroll the mouse wheel up, down, respectively
NONE: No input
CALIBRATE: recalibrate gyro when pressing this input
GYRO_ON, GYRO_OFF: Enable or disable gyro
GYRO_INVERT, GYRO_INV_X, GYRO_INV_Y: Invert gyro, or in just the x or y axes, respectively
GYRO_TRACKBALL, GYRO_TRACK_X, GYRO_TRACK_Y: Keep last gyro input, or in just the x or y axes, respectively
; ' , . / \ [ ] + - `
"any console command": Any console command can be run on button press, including loading a file
```

For example, in a game where R is 'reload' and E is 'use’, you can do the following to map □ to 'reload' and △ to 'use':

```
W = R
N = E
```

Those familiar with Steam Input can implement Action Layers and Action Sets using the quotation marks to load a file on demand. That file can contain partial or full remapping of the controller bindings. This is very useful for having a different scheme for vehicles, menus or characters.

```
HOME = "GTA_driving.txt"  # Load the driving control scheme. That file should bind home to loading the walking scheme file!
```

Take note that the command bound in this way cannot contain quotation marks, and thus cannot contain the binding of a command itself. In this case, you should put the command in a file and load that file.


#### 1.1 Tap & Hold

Since a keyboard has many more inputs available than most controllers, console games will often map multiple actions to the same button while the PC version has those actions mapped to different keys. In order to fit keyboard mappings onto a controller, JoyShockMapper allows you to map taps and holds of a button to different keyboard/mouse inputs. So let's take that same game and make it so we can tap □ to 'reload' or hold □ to 'use':

```
W = R E
```

If you want □ to 'reload' when tapped, but do nothing at all when held, you can do the following:

```
W = R NONE
```

The time to hold the button before enabling the hold binding can be changed by assigning a number of milliseconds to ```HOLD_PRESS_TIME```.

See the tap press and hold press event modifiers below for more details on how keybinds are applied.

#### 1.2 Binding Modifiers

There are two kinds of modifiers that can be applied to key bindings: action modifiers and event modifiers. They are represented by symbols added before and after the key name repectively. And each binding can only ever have one of each. You can however have multiple keys bound to the same events, thus sending multiple key presses at once.

**Action modifiers** come in two kinds: **toggle (^)** and **instant (!)**. Using either of these modifiers will make it so that nothing is bound to the release of the button press.
* ^ Toggle makes it so that the key will alternate between applying and releasing the key press at each press.
* ! Instant sends both the key press and release at the same time

**Event Modifiers** come in five kinds: **tap press (')**, **hold press (_)**, **start press (\\)**, **release press (/)** and **turbo (+)**.
* ' Tap press is the default event modifier for the first key bind when there are multiple of them. It will apply the key press when the button is released if the total press time is less than the ```HOLD_PRESS_TIME```. The key press is released a short time after, with that time being longer for gyro related action and calibration, unless an action modifier instructs otherwise.
* _ Hold press is the default event modifier for the second key bind when there are multiple of them. It will apply the key only after the button is held down for the defined amount of time. The key is released when the button is released as well, unless an action modifier instructs otherwise.
* \\ Start press is the default event modifier when there is only a single key bind. It will apply the key press as soon as the button is pressed and releases the key when the button is released, unless an action modifier instructs otherwise. This can be useful to have a key held while other keys are being activated.
* \/ Release press will apply the binding when the button is released. A binding on release press needs an action modifier to be valid.
* \+ Turbo will apply a key press repeatedly (with consideration of action modifiers), resulting in a fast pulsing of the key. The turbo pulsing starts only after the button has been held for ```HOLD_PRESS_TIME```.

These modifiers can enable you to work around in game tap and holds, or convert one form of press into another. Here's a few example of how you can make use of those modifiers.

```
ZL = ^RMOUSE RMOUSE  # ADS toggle on tap
E  = !C\ !C/         # Convert in game toggle crouch to regular press
UP = !1\ 1           # Convert Batarang throw double press to hold press
W  = R E\            # In Halo MCC, reload on tap but apply E right away to cover for in-game hold processing
-,S = SPACE+         # Turbo press for button mash QTEs. No one likes to button mash :(
R3 = !1\ LMOUSE+ !Q/ # Half life melee button
UP,UP = !ENTER\ LSHIFT\ !G\ !L\ !SPACE\ !H\ !F\ !ENTER/ # Pre recorded message
UP,E = BACKSPACE+    # Erase pre recorded message if I change my mind
```

Take note that the Simultaneous Press and Double Press bindings (but not Chorded Press) below introduce delays in the raising of the events until the right mapping is determined. Time windows are not added but events will be pushed together within a frame or two.

#### 1.3 Simultaneous Press
JoyShockMapper additionally allows you to map simultaneous button presses to different mappings. For example you can bind character abilities on your bumpers and an ultimate ability on both like this:

```
L = LSHIFT # Ability 1
R = E      # Ability 2
L+R = Q    # Ultimate Ability
```

To enable a simultaneous binding, both buttons need to be pressed within a very short time of each other. Doing so will ignore the individual button bindings and apply the specified binding until either of the button is released. Simultaneous bindings also support tap & hold bindings as well as modifiers just like other mappings. This feature is great to make use of the dpad diagonals, or to add JSM specific features like gyro calibration and gyro control without taking away accessible buttons.

The time window in which both buttons need to be pressed can be changed by assigning a different number of milliseconds to ```SIM_PRESS_WINDOW```. This setting cannot be changed by modeshift (covered later).

#### 1.4 Chorded Press
Chorded press works differently from Simultaneous Press, despite being similar at first blush. A chorded press mapping allows you to override a button mapping when the chord button is down. This enables a world of different practical combinations, allowing you to have contextual bindings. Here's an example for Left 4 Dead 2, that would enable you to equip items without lifting the thumb from the left stick.

```
W = R E # Reload / Use
S = SPACE # Jump
E = CONTROL # Crouch
N = T # Voice Chat

L = Q NONE # Other weapon, hold to select with face button.
L,W = 3 # Explosives
L,S = 4 # Pills
L,E = 5 # Medpack
L,N = F # Flashlight
```

A button can be chorded with multiple other buttons. In this case, the latest chord takes precedence over previous chords. This can be understood as a stack of layers being put on top of the binding each time a chord is pressed, where only the top one is active. Notice that you don't need to have NONE as a binding. The chord binding could very well be bound to a button that brings up a weapon wheel for example.

#### 1.5 Double Press
You can also assign the double press of a button to a different binding. Double press notation is the same as chorded button notation, except the button is chorded with itself. It supports taps & holds and modifiers like all previous entries.

```
N = SCROLLDOWN # Cycle weapon
N,N = X # Cycle weapon fire mode
```

The double press binding is applied when a down press occurs within a fifth of a second from a first down press. In that period of time no other binding can be assumed, so regular taps will have the delay introduced. This binding also supports tap & hold bindings as well as modifiers. The time window in which to perform the double press can be changed by assigning a different number of milliseconds to ```DBL_PRESS_WINDOW```. This setting cannot be changed by modeshift (covered later).

#### 1.6 Gyro Button
Lastly, there is one digital input that works differently, because it can overlap with any other input. Well, two inputs, but you'll use at most one of them in a given configuration:

```
GYRO_OFF
GYRO_ON
```

When you assign a button to ```GYRO_ON```, gyro mouse only work while that button is pressed. ```GYRO_OFF``` disables the gyro while the button is pressed. This is a really important feature absent from most games that have gyro aiming -- just as a PC gamer can temporarily "disable" the mouse by lifting it off the mousepad in order to reposition it, a gyro gamer should be able to temporarily disable the gyro in order to reposition it. This binding doesn't affect other mappings associated with that button. This is so that the gyro can be enabled alongside certain in-game actions, or so that the gyro can be disabled or enabled instantly regardless of what taps or holds are mapped to that button.

For games that really need to use all the buttons available on the controller, but one of those inputs is rarely used and can be toggled easily (like crouching, for example), it might make sense to make that input tap-only, and make it double as the gyro-off button when held:

```
E = LCONTROL NONE
GYRO_OFF = E
```

Or if you really can't spare a button for disabling the gyro, you can use LEFT\_STICK or RIGHT\_STICK to disable the gyro while that input is being used:

```GYRO_OFF = RIGHT_STICK # Disable gyro while aiming with stick```

I prefer to be able to use stick aiming (or flick stick) at the same time as aiming with the gyro, but this can still be better than having no way to disable the gyro at all if your game doesn't have an obvious function to tie to enabling gyro aiming (like a dedicated "aim weapon" button as is common in third-person action games).

```GYRO_ON``` is really useful for games where you only sometimes need to aim precisely. If ZL causes your character to aim their bow (like in *Zelda: Breath of the Wild* or *Shadow of Mordor*), maybe that's the only time you want to have gyro aiming enabled:

```
ZL = RMOUSE   # Aim with Bow
GYRO_ON = ZL  # Turn on gyro when ZL is pressed
```

```GYRO_ON``` and ```GYRO_OFF``` can also be bound as an action to particular buttons. Contrary to the command above, this takes the spot of the action binding. But you can still find creative ways with taps & holds or chorded press to bind the right gyro control where you need it.

Take note that taps apply gyro-related bindings for half a second. Another option is inverting the gyro input with ```GYRO_INVERT```. Such a binding can be handy if you play with a single joycon because you don't have a second stick. When that action is enabled, the inversion makes it so that you can recenter the hands by continuing to turn in the opposite direction!

```
SL + SR = GYRO_OFF GYRO_INVERT  # Disable for .5s / Invert axis on simultaneous bumper hold
```

Bound gyro actions like those have priority over the assigned gyro button should they conflict.

The command ```NO_GYRO_BUTTON``` can be used to remove the gyro-on or gyro-off mapping, making gyro always enabled. To have it always disabled, just set ```GYRO_ON = NONE``` or leave ```GYRO_SENS``` at 0.

If you're using ```GYRO_TRACKBALL``` or its single-axis variants, you can use **TRACKBALL\_DECAY** to choose how quickly the trackball effect loses momentum. It can be set to 0 for no decay. Its default value of 1 halves the gyro trackball's momentum over each second. 2 will halve it in 1/2 seconds, 3 in 1/3 seconds, and so on.

### 2. Analog Triggers

The following section only applies to the DS4 controller because it is the only supported controller that has analog triggers.

Analog triggers report a value between 0% and 100% representing how far you are pulling the trigger. Binding a digital button to an analog trigger is done using a threashold value. The button press is sent when the trigger value crosses the threashold value, sitting between 0% and 100%. The default threashold value is 0, meaning the slightest press of the trigger sends the button press. This is great for responsiveness, but could result in accidental presses. The threashold can be customized by running the following command:

```
TRIGGER_THRESHOLD = 0.5   #Send Trigger values at half press
```

The same threashold value is used for both triggers. A value of 1.0 or higher makes the binding impossible to reach, and a value below 0 makes it always pressed.

JoyShockMapper can assign different bindings to the full pull of the trigger, allowing you to have up to 4 bindings on each trigger when considering the hold bindings. The way the trigger handles these bindings is set with the variables ```ZR_MODE``` and ```ZL_MODE```, for R2 and L2 triggers. Once set, you can assign keys to ```ZRF``` and ```ZLF``` to make use of the R2 and L2 full pull bindings respectively. In this context, ```ZL``` and ```ZR``` are called the soft pull binding because they activate before the full pull binding do at 100%. Here is the list of all possible trigger modes.

```
NO_FULL (default): Ignore full pull binding. This mode is enforced on controllers who have digital triggers like the Pro Controller.
NO_SKIP: Never skip the soft pull binding. Full pull binding activates anytime the trigger is fully pressed.
MUST_SKIP: Only send full pull binding on a quick full press of the trigger, ignoring soft pull binding.
MAY_SKIP: Combines NO_SKIP and MUST_SKIP: Soft binding may be skipped on a quick full press, and full pull can be activated on top of soft pull binding.
MUST_SKIP_R: Responsive version of MUST_SKIP. See below.
MAY_SKIP_R: Responsive version of MAY_SKIP. See below.
```

For example, in Call of Duty you have a binding to hold your breath when aiming with a sniper. You can bind ADS on a soft trigger press and hold breath on the full press like this:

```
ZL_MODE = NO_SKIP   # Enable full pull binding, never skip ADS
ZL = RMOUSE         # Aim Down Sights
ZLF = LSHIFT        # Hold breath
```

Using NO_SKIP mode makes it so that LSHIFT is only active if RMOUSE is active as well. Then on the right trigger, you can bind your different attack bindings: use the skipping functionality to avoid having them conflict with eachother like this:

```
ZR_MODE = MUST_SKIP # Enable full pull binding, soft and full bindings are mutually exclusive
ZR = LMOUSE         # Primary Fire
ZRF = V G           # Quick full tap to melee; Quick hold full press to unpin grenade and throw on release
```

Using MUST_SKIP mode makes sure that once you start firing, reaching the full pull will **not** make you stop firing to melee.

The "Responsive" variants of the skip modes enable a different behaviour that can give you a better experience than the original versions in specific circumstances. A typical example is when the soft binding is a mode-like binding like ADS or crouch, and there is no hold or simultaneous press binding on that soft press. The difference is that the soft binding is actived as soon as the trigger crosses the threshold, giving the desired responsive feeling, but gets removed if the full press is reached quickly, thus still allowing you to hip fire for example. This will result in a hopefully negligeable scope glitch but grants a snappier ADS activation.

### 3. Stick Mouse Inputs
Each stick has 7 different operation modes:

```
AIM: traditional stick aiming
FLICK: flick stick
FLICK_ONLY: flick stick without rotation after tilting the stick
ROTATE_ONLY: flick stick rotation without the initial flick
MOUSE_RING: stick angle sets the mouse position on a circle directly around the center of the screen
MOUSE_AREA: stick position sets the cursor in a circular area around the neutral position
NO_MOUSE: don't affect the mouse, use button mappings (default)
```

The mode for the left and right stick are set like so:

```
LEFT_STICK_MODE = NO_MOUSE
RIGHT_STICK_MODE = AIM
```

Regardless of what mode you're in, you can have additional input bound to a partial or full tilt of either stick. For example, you might want to always be pressing LSHIFT when the stick is fully tilted:

```
LEFT_RING_MODE = OUTER # OUTER is default, so this line is optional
LRING = LSHIFT
```

Or you might want to always be pressing LALT when the stick is only partially tilted:

```
LEFT_RING_MODE = INNER
LRING = LALT
```

For backwards compatibility reasons, there are two extra options for ```LEFT_STICK_MODE``` and ```RIGHT_STICK_MODE``` that set the corresponding STICK_MODE and RING_MODE at the same time:

```
INNER_RING: Same as _STICK_MODE = NO_MOUSE and _RING_MODE = INNER
OUTER_RING: Same as _STICK_MODE = NO_MOUSE and _RING_MODE = OUTER
```

Let's have a look at all the different operations modes.

When using the ```AIM``` stick mode, there are a few important commands:

* **STICK\_SENS** (default 360.0 degrees per second) - How fast does the stick move the camera when tilted fully? The default, when calibrated correctly, is 360 degrees per second. Assign a second value if you desire a different vertical sensitivity from the horizontal sensitivity.
* **STICK\_POWER** (default 1.0) - What is the shape of the curve used for converting stick input to camera turn velocity? 1.0 is a simple linear relationship (half-tilting the stick will turn at half the velocity given by STICK\_SENS), 0.5 for square root, 2.0 for quadratic, etc. Minimum value is 0.0, which means any input beyond STICK\_DEADZONE\_INNER will be treated as a full press as far as STICK\_SENS is concerned.
* **STICK\_AXIS\_X** and **STICK\_AXIS\_Y** (default STANDARD) - This allows you to invert stick axes if you wish. Your options are STANDARD (default) or INVERTED (flip the axis).
* **STICK\_ACCELERATION\_RATE** (default 0.0 multiplier increase per second) - When the stick is pressed fully, this option allows you to increase the camera turning velocity over time. The unit for this setting is a multiplier for STICK\_SENS per second. For example, 2.0 with a STICK\_SENS of 100 will cause the camera turn rate to accelerate from 100 degrees per second to 300 degrees per second over 1 second.
* **STICK\_ACCELERATION\_CAP** (default 1000000.0 multiplier) - You may want to set a limit on the camera turn velocity when STICK\_ACCELERATION\_RATE is non-zero. For example, setting STICK\_ACCELERATION\_CAP to 2.0 will mean that your camera turn speed won't accelerate past double the STICK\_SENS setting. This has no effect when STICK\_ACCELERATION\_RATE is zero.
* **STICK\_DEADZONE\_INNER** and **STICK\_DEADZONE\_OUTER** (default 0.15 and 0.1, respectively) - Controller thumbsticks can be a little imprecise. When you release the stick, it probably won't return exactly to the centre. STICK\_DEADZONE\_INNER lets you say how much of the stick's range will be considered "centre". If the stick position is within this distance from the centre, it'll be considered to have no stick input. STICK\_DEADZONE\_OUTER does the same for the outer edge. If the stick position is within this distance from the outer edge, it'll be considered fully pressed. Everything in-between is scaled accordingly. You can set the deadzones individually for each stick with **LEFT\_STICK\_DEADZONE\_INNER**, **LEFT\_STICK\_DEADZONE\_OUTER**, **RIGHT\_STICK\_DEADZONE\_INNER**, **RIGHT\_STICK\_DEADZONE\_OUTER**.

When using the ```FLICK``` stick mode, there is less to configure. There are no deadzones and no sensitivity. When you press the stick in a direction, JoyShockMapper just takes the angle of the stick input and translates it into the same in-game direction relative to where your camera is already facing, before smoothly moving the camera to point in that direction in a small fraction of a second. Once already pressed, rotating the *flick stick* X degrees will then instantly turn the in-game camera X degrees. This provides a very natural way to quickly turn around, respond to gun-fire from off-screen, or make gradual turns without moving the controller.

Since *flick stick* only turns the camera horizontally, it's generally only practical in combination with gyro aiming that can handle vertical aiming.

*Flick stick* will use the above-mentioned STICK\_DEADZONE\_OUTER to decide if the stick has been pressed far enough for a flick or rotation. *Flick stick* relies on REAL\_WORLD\_CALIBRATION to work correctly ([covered later](#4-real-world-calibration), as it affects all inputs that translate to mouse-movements). This is because JoyShockMapper can only point the camera in a given direction by making the *right* mouse movement, and REAL\_WORLD\_CALIBRATION helps JoyShockMapper calculate what that movement should be. A game that natively implements *flick stick* would have no need for calibration. *Flick stick* has a few settings if you really want to mess with it:

* **FLICK\_TIME** (default 0.1 seconds) - When you tilt the stick a direction, how long does it take the camera to complete its turn to face that direction? I find that 0.1 seconds provides a nice, snappy response, while still looking good. Set the value too low and it may look like you're cheating, instantly going from one direction to facing another.  
Keep in mind that, once tilted, rotating the stick will rotate the camera instantly. There’s no need to smooth it out\*; the camera just needs to make the same movement the stick is. FLICK\_TIME only affects behaviour when you first tilt the stick.
* **FLICK\_TIME\_EXPONENT** (default 0.0) - Some people prefer the actual flick time to match the flick angle, and some games don't handle extreme flicks in a short timespan well. This setting scales the FLICK\_TIME based on the flick angle. For any value here, FLICK\_TIME will always match a 180 degree flick, but the mapping between 0 to 180 degrees is affected: a value of 0.0 means no scaling at all (any flick takes FLICK\_TIME seconds), while a value of 1.0 causes a 1:1 mapping between flick angle and flick time (a 90 degree flick takes 0.5 * FLICK\_TIME seconds). Higher values (over)dramatize the differences between small and large flicks.
* **FLICK\_SNAP\_MODE** (default none) - Without practice, sometimes you'll flick to a different angle than you intended. If you want to limit the angles you can flick to, FLICK\_SNAP\_MODE gives you three options. The default, NONE, is no snapping at all. With practice, I expect players will find this most useful, as surprises can come from any angle. But your other options are 4, which snaps the flick to the nearest of directly forward, directly left, directly right, or directly backwards. These are 90° intervals. If you want to be able to snap to 45° intervals, too, you can set FLICK\_SNAP\_MODE to 8.
* **FLICK\_SNAP\_STRENGTH** (default 1.0) - If you have a snap mode other than NONE set, this value gives you the strength of its snapping, ranging from 0 (no snapping) to 1 (full snapping).
* **FLICK\_DEADZONE\_ANGLE** (default 0.0 degrees) - Sometimes you want to prepare for turning quickly without flicking. Pushing the stick perfectly forward is near impossible so you end up turning a little, losing the angle you are trying to hold. This setting creates a deadzone for forward flicks: moving the thumbstick forward within this range will be treated as flicking at a 0 degree angle, basically putting you in rotation mode directly. The value is applied in both left and right directions separately: setting this to 15 creates a total deadzone arc of 30 degrees.

**\*Developer note:** The DualShock 4's stick input resolution is low enough that small *flick stick* rotations can be jittery. JoyShockMapper applies some smoothing just to very small changes in the *flick stick* angle, which is very effective at covering this up. Larger movements are not smoothed at all. This is more thoroughly explained for developers to implement in their own games on [GyroWiki](http://gyrowiki.jibbsmart.com). JoyShockMapper automatically calculates different smoothing thresholds for the DualShock 4 and Switch controllers, but you can override the smoothing threshold by setting ROTATE\_SMOOTH\_OVERRIDE any small number, or to 0 to disable smoothing, or to -1 to return to the automatically calculated threshold.

The ```FLICK_ONLY``` and ```ROTATE_ONLY``` stick modes work the same as flick stick with some features blocked out. The former means you'll get the initial flick, but no subsequent rotation when rotating the stick. The latter means you won't get the initial flick, but subsequent rotations will work.

When using the ```MOUSE_RING``` stick mode, tilting the stick will put the mouse cursor in a position offset from the centre of the screen by your stick position. This mode is primarily intended for twin-stick shooters. To do this, the application needs to know your screen resolution (SCREEN\_RESOLUTION\_X and SCREEN\_RESOLUTION\_Y) and how far you want the cursor to sit from the centre of the screen (MOUSE\_RING\_RADIUS). When this mode is in operation (i.e. the stick is not at rest), all other mouse movements are ignored.

When using the ```MOUSE_AREA``` stick mode, the stick value directly sets the mouse position. So moving the stick rightward gradually all the way to the edge will move the cursor at the same speed for a number of pixel equal to the value of ```MOUSE_RING_RADIUS``` ; and moving the stick back to the middle will move the cursor back again to where it started. Contrary to the previous mode, this mode can operate in conjunction with other mouse inputs, such as gyro.

When using stick mode ```NO_MOUSE```, JSM will use the stick's UP DOWN LEFT and RIGHT bindings in a cross gate layout. There is a small square deadzone to ignore very small stick moves.

```
# Left stick moves
LLEFT = A
LRIGHT = D
LUP = W
LDOWN = S
LEFT_RING_MODE = INNER
LRING = LALT # Walk
```

If you're holding the controller in an unusual orientation (such as for comfort reasons or when using a single JoyCon), you can set **CONTROLLER\_ORIENTATION** to reflect how you're holding the controller:
* **FORWARD** is the default.
* **LEFT** is for when you're holding the controller rotated to its left.
* **RIGHT** is for when you're holding the controller rotated to its right.
* **BACKWARD** is for when you're holding teh controller rotated 180°.

CONTROLLER\_ORIENTATION only affects sticks (including motion stick). It doesn't affect the arrangement of the face buttons, d-pad, etc.

#### 3.1 Motion Stick
Using the motion sensors, you can treat your whole controller as a stick. The "Motion Stick" can do everything that a regular stick can do:
* **MOTION\_STICK\_MODE** (default NO\_MOUSE) - All the same options as LEFT\_STICK\_MODE and RIGHT\_STICK\_MODE.
* **MOTION\_RING\_MODE** (default OUTER) - All the same options as LEFT\_RING\_MODE and RIGHT\_RING\_MODE.
* **MOTION\_DEADZONE\_INNER** (default 15°) - How far the controller needs to be tilted in order to register as non-zero.
* **MOTION\_DEADZONE\_OUTER** (default 135°) - How far from the maximum rotation will be considered a full tilt. The maximum rotation is of course 180°, so the default value of 135° means tilting at or above 45° from the **neutral position** will be considered "full tilt".
* **MLEFT**, **MRIGHT**, **MUP**, **MDOWN** are the motion stick equivalents of left, right, forward, back mappings, respectively.
* This is also affected by **CONTROLLER\_ORIENTATION** described at the end of the previous section.

The gyro needs to be correctly calibrated for motion stick to work best (see calibration commands below under Gyro Mouse Inputs).

By default, the **neutral position** is approximately the position the controller is when left on a flat surface. You can set a different neutral position by entering the command ```SET_MOTION_STICK_NEUTRAL```. When this command is executed, however you're holding the controller at the time will be considered the "neutral" orientation.

A common use for the motion sensors is to map left and right leans of the controller. This isn't quite the same as motion stick -- regardless of whether you hold your controller flat or upright, lean mappings should still work the same. You just need:
* **LEAN\_THRESHOLD** (default 15°) - Leaning the controller more than this angle to the left or right will trigger the **LEAN\_LEFT** or **LEAN\_RIGHT** bindings, respectively.

### 4. Gyro Mouse Inputs
**The first thing you need to know about gyro mouse inputs** is that a controller's gyro will often need calibrating. This just means telling the application where "zero" is. Just like a scale, the gyro needs a point of reference to compare against in order to accurately give a result. This is done by leaving the controller still, or holding it very still in your hands, and finding the average velocity over a short time of staying still. It needs to be averaged over some time because the gyro will pick up a little bit of "noise" -- tiny variations that aren't caused by any real movement -- but this noise is negligible compared to the shakiness of human hands trying to hold a controller still.

When you first connect controllers to JoyShockMapper, they'll all begin "continuous calibration" -- this just means they're collecting the average velocity over a long period of time. This accumulated average is constantly applied to gyro inputs, so if the controller is left still for long enough, you should be able to play without obvious problems.

If you have gyro mouse enabled and the gyro moves across the screen (even slowly) when the controller is lying still on a solid surface, your device needs calibrating. That's okay -- I do it at the beginning of most play sessions, especially with Nintendo devices, which seem to need it more often.

To calibrate your gyro, place your controller on solid surface so that it's not moving at all, and then use the following commands:
* **RESTART\_GYRO\_CALIBRATION** - All connected gyro devices will begin collecting gyro data, remembering the average collected so far and treating it as "zero".
* **FINISH\_GYRO\_CALIBRATION** - Stop collecting gyro data for calibration. JoyShockMapper will use whatever it has already collected from that controller as the "zero" reference point for input from that controller.

It should only take a second or so to get a good calibration for your devices. You can also calibrate each controller separately with buttons mapped to **CALIBRATE**. This is how you using them assuming you use the built-in mappings:

* Tap the PS, Touchpad-click, Home, or Capture button on your controller to restart calibration, or to finish calibration if that controller is already calibrating.
* Hold the PS, Touchpad-click, Home, or Capture button to restart calibration, and it'll finish calibration once you release the controller. **Warning**: I've found that touching the Home button interferes with the gyro input on one of my JoyCons, so if I hold the button to calibrate it, it'll be incorrectly calibrated when I release the button. If you encounter this, it's better to rely on the tapping toggle shortcuts above for each controller, or calibrate all controllers at the same time using the commands above.

The reason gyros need calibrating is that their physical properties (such as temperature) can affect their sense of "zero". Calibrating at the beginning of a play session will usually be enough for the rest of the play session, but it's possible that after the controller warms up it could use calibrating again. You'll be able to tell it needs calibrating if it appears that the gyro's "zero" is incorrect -- when the controller isn't moving, the mouse moves steadily in one direction anyway.

**The second thing you need to know about gyro mouse inputs** is how to choose the sensitivity of the gyro inputs:

* **GYRO\_SENS** (default 0.0) - How does turning the controller turn the in-game camera? A value of 1 means turning the controller will turn the in-game camera the same angle (within the limits of two axes). A value of 2 means turning the controller will turn double the angle in-game. Increasing the GYRO\_SENS gives you more freedom to move around before turning uncomfortably and having to disable the gyro and reposition the controller, but decreasing it will give you more stability to hit small targets.  
For games where you don't turn the camera directly, but instead use the mouse to control an on-screen cursor, a GYRO\_SENS of 1 would mean the controller needs to turn around completely to get from one side of the screen to the other. For games like these, you'll be better off with a GYRO\_SENS of 8 or more, meaning you only need to turn the controller 360/8 = 45 degrees to move from one side of the screen to the other.

A single GYRO\_SENS setting may not be enough to get both the precision you need for small targets and the range you need to comfortably navigate the game without regularly having to disable the gyro and reposition the controller.

JoyShockMapper allows you to say, "When turning slowly, I want this sensitivity. When turning quickly, I want that sensitivity." You can do this by setting two real life speed thresholds and a sensitivity for each of those thresholds. Everything in-between will be linearly interpolated. To do this, use MIN\_GYRO\_THRESHOLD, MAX\_GYRO\_THRESHOLD, MIN\_GYRO\_SENS, and MAX\_GYRO\_SENS:

* **MIN\_GYRO\_THRESHOLD** and **MAX\_GYRO\_THRESHOLD** (default 0.0 degrees per second); **MIN\_GYRO\_SENS** and **MAX\_GYRO\_SENS** (default 0.0) - MIN\_GYRO\_SENS and MAX\_GYRO\_SENS work just like GYRO\_SENS, but MIN\_GYRO\_SENS applies when the controller is turning at or below the speed defined by MIN\_GYRO\_THRESHOLD, and MAX\_GYRO\_SENS applies when the controller is turning at or above the speed defined by MAX\_GYRO\_THRESHOLD. When the controller is turning at a speed between those two thresholds, the gyro sensitivity is interpolated accordingly. The thresholds are in real life degrees per second. For example, if you think about how fast you need to turn the controller for it to turn a quarter circle in one second, that's 90 degrees per second. Setting GYRO\_SENS overrides MIN\_GYRO\_SENS and MAX\_GYRO\_SENS to be the same value. You can set a different **vertical sensitivity** by giving two values to the command separated by a space, instead of just one.

**Finally**, there are a bunch more settings you can tweak if you so desire:

* **GYRO\_AXIS\_X** and **GYRO\_AXIS\_Y** (default STANDARD) - This allows you to invert the gyro directions if you wish. Want a left- gyro turn to translate to a right- in-game turn? Set GYRO\_AXIS\_X to INVERTED. For normal behaviour, set it to STANDARD.
* **MOUSE\_X\_FROM\_GYRO\_AXIS** and **MOUSE\_Y\_FROM\_GYRO\_AXIS** (default Y and X, respectively) - Maybe you want to turn the camera left and right by rolling your controller about its local Z axis instead of turning it about its local Y axis. Or maybe you want to play with a single JoyCon sideways. This is how you do that. Your options are X, Y, Z, and NONE, if you want an axis of mouse movement unaffected by the gyro.
* **GYRO\_CUTOFF\_SPEED** (default 0.0 degrees per second) - Some games attempt to cover up small unintentional movements by setting a minimum speed below which gyro input will be ignored. This is that setting. It's never good. Don't use it. Some games won't even let you change or disable this "feature". I implemented it to see if it could be made good. I left it in there only so you can see for yourself that it's not good, or for you to perhaps show me how it can be.  
It might be mostly harmless for interacting with a simple UI with big-ish buttons, but it's useless if the player will *ever* intentionally turn the controller slowly (such as to track a slow-moving target), because they may unintentionally fall below the cutoff speed. Even a very small cutoff speed might be so high that it's impossible to move the aimer at the same speed as a very slow-moving target.  
One might argue that such a cutoff is too high, and it just needs to be set lower. But if the cutoff speed is small enough that it doesn't make the player's experience worse, it's probably also small enough that it's actually not doing anything.
* **GYRO\_CUTOFF\_RECOVERY** (default 0.0 degrees per second) - In order to avoid the problem that GYRO\_CUTOFF\_SPEED makes it impossible to move the cursor at the same speed as a very slow-moving target, JoyShockMapper smooths over the transition between the cutoff speed and a threshold determined by GYRO\_CUTOFF\_RECOVERY. Originally intended to make GYRO\_CUTOFF\_SPEED not awful, it ends up doing a good job of reducing shakiness even when GYRO\_CUTOFF\_SPEED is set to 0.0, but I only use it (possibly in combination with smoothing, below) as a last resort.
* **GYRO\_SMOOTH\_THRESHOLD** (default 0.0 degrees per second) - Optionally, JoyShockMapper will apply smoothing to the gyro input to cover up shaky hands at high sensitivities. The problem with smoothing is that it unavoidably introduces latency, so a game should *never* have *any* smoothing apply to *any input faster than a very small threshold*. Any gyro movement at or above this threshold will not be smoothed. Anything below this threshold will be smoothed according to the GYRO\_SMOOTH\_TIME setting, with a gradual transition from full smoothing at half GYRO\_SMOOTH\_THRESHOLD to no smoothing at GYRO\_SMOOTH\_THRESHOLD.
* **GYRO\_SMOOTH\_TIME** (default 0.125s) - If any smoothing is applied to gyro input (as determined by GYRO\_SMOOTH\_THRESHOLD), GYRO\_SMOOTH\_TIME is the length of time over which it is smoothed. Larger values mean smoother movement, but also make it feel sluggish and unresponsive. Set the smooth time too small, and it won't actually cover up unintentional movements.

### 5. Real World Calibration
*Flick stick*, aim stick, and gyro mouse inputs all rely on REAL\_WORLD\_CALIBRATION to provide useful values that can be shared between games and with other players. Furthermore, if REAL\_WORLD\_CALIBRATION is set incorrectly, *flick stick* flicks will not correspond to the direction you press the stick at all.

Every game requires a unique REAL\_WORLD\_CALIBRATION value in order for these other settings to work correctly. This actually really simplifies sharing configuration files, because once one person has calculated an accurate REAL\_WORLD\_CALIBRATION value for a given game, they can share it with anyone else for the same game. [GyroWiki](http://gyrowiki.jibbsmart.com) has a database of games and their corresponding REAL\_WORLD\_CALIBRATION (as well as other settings unique to that game). You can use this to avoid having to calculate this value in games you want to play with JoyShockMapper, or if a game isn't in that database, you can help other players by calculating its REAL\_WORLD\_CALIBRATION yourself and contributing it to [GyroWiki](http://gyrowiki.jibbsmart.com)!

For 3D games where the mouse directly controls the camera, REAL\_WORLD\_CALIBRATION is a multiplier to turn a given angle in degrees into a mouse input that turns the camera the same angle in-game.

For 2D games where the mouse directly controls an on-screen cursor, REAL\_WORLD\_CALIBRATION is a multiplier to turn a given fraction of a full turn into a mouse input that moves the same fraction of the full width of the screen (at 1920x1080 resolution in games where resolution affects cursor speed relative to the size of the screen).

Before we get into how to accurately calculate a good REAL\_WORLD\_CALIBRATION value for a given game, we first need to address two other things that can affect mouse sensitivity:

* In-game mouse settings
* Windows mouse settings

Even if REAL\_WORLD\_CALIBRATION is set correctly, your **in-game mouse settings** will change how the camera or cursor responds to mouse movements:

* If you are playing with *mouse acceleration* enabled (a setting only a few games have, and they will usually have it disabled by default), you’ll need to disable it in-game for JoyShockMapper to work accurately.
* Most games have a *mouse sensitivity* setting, which is a simple multiplier for the mouse input. JoyShockMapper can't see this value, so you need to tell it what that value is so it can cancel it out. You can do this by setting ```IN_GAME_SENS``` to the game’s mouse sensitivity.  
For example, for playing with keyboard and mouse, my *Quake Champions* mouse sensitivity is 1.8. So in my *Quake Champions* config files for JoyShockMapper, or whenever I use someone else’s config file, I include the line: ```IN_GAME_SENS = 1.8``` so that JoyShockMapper knows to cancel it out.

There’s one other factor that *some* games need to deal with. **Windows mouse settings**:

* In your Windows mouse settings, there’s an “Enhance pointer precision” option that JoyShockMapper can’t accurately account for. Most gamers play with this option disabled, and it’s preferable for using JoyShockMapper that you disable it, too.
* Windows’ pointer speed setting will also often affect the way the mouse behaves in game, but JoyShockMapper *can* detect Windows’ pointer speed setting and account for it. This is done with the simple command: ```COUNTER_OS_MOUSE_SPEED```.  
The only complication is that some games *aren’t* affected by Windows’ pointer speed settings. Many modern shooters use “raw input” to ignore Windows’ settings so the user is free to use “Enhance pointer precision” and whatever sensitivity settings they want in the operating system without it affecting the game. If you’ve used COUNTER\_OS\_MOUSE\_SPEED and realised you shouldn’t have, the command ```IGNORE_OS_MOUSE_SPEED``` restores default behaviour, which is good for games that use raw input.

In summary, when preparing to share a configuration file for others to use, please consider whether COUNTER\_OS\_MOUSE\_SPEED should be included. When using someone else’s configuration file, please remember to set the file’s IN\_GAME\_SENS to whatever *your* in-game mouse sensitivity is.

Once you've done this, you're ready to **calculate your game's REAL\_WORLD\_CALIBRATION value**.

For *3D games where the mouse directly controls the camera*, the most accurate way to calculate a good REAL\_WORLD\_CALIBRATION value is to enable *flick stick* along with a first-guess REAL\_WORLD\_CALIBRATION value like so:

```
RIGHT_STICK_MODE = FLICK
REAL_WORLD_CALIBRATION = 40
```

Now, in-game, use your mouse to fix your aimer precisely on a small target in front of you. Press your right stick forward, and rotate it until you've completed a full turn, releasing the stick once your aimer is in the same position it started before you pressed the stick.

JoyShockMapper remembers the last *flick stick* flick and rotation you did, so now you can simply enter the following command:

```CALCULATE_REAL_WORLD_CALIBRATION```

This tells JoyShockMapper that your last flick and rotation was exactly one full in-game turn, and that you'd like to know what REAL\_WORLD\_CALIBRATION value you should have. JoyShockMapper will respond with something like, "Recommended REAL\_WORLD\_CALIBRATION: 151.5" (for example). Now you can verify that everything worked correctly by changing your REAL\_WORLD\_CALIBRATION setting like so:

```REAL_WORLD_CALIBRATION = 151.5``` (or whatever value JoyShockMapper gave you).

Now check in-game if this value works by completing a flick stick rotation again. The angle you turn in-game *should* match the angle you turned the stick.

If you want to be even more precise, you can do more than one turn. If, for example, you complete 10 turns in a row without releasing the stick in order to average out any imprecision at the point of releasing the stick, you can add the number of turns after the CALCULATE\_REAL\_WORLD\_CALIBRATION command:

```CALCULATE_REAL_WORLD_CALIBRATION 10```

You can do this with non-integer turns, as well, such as 0.5 for a half turn.

For *2D games where the mouse directly controls an on-screen cursor*, *flick stick* doesn't have a practical use in general gameplay, but it's still the most useful way to calculate your REAL\_WORLD\_CALIBRATION value. Again, make sure your IN\_GAME\_SENS and COUNTER\_OS\_MOUSE\_SPEED are set as needed, and then we'll start by enabling *flick stick* alongside a first guess at the REAL\_WORLD\_CALIBRATION.

```
RIGHT_STICK_MODE = FLICK
REAL_WORLD_CALIBRATION = 1
```

Notice that this time, our first guess REAL\_WORLD\_CALIBRATION value is *1* instead of *40*. 2D cursor games tend to have a much lower REAL\_WORLD\_CALIBRATION value than 3D camera games, and it's better to underestimate your first guess than overestimate, so you can complete more stick turns and get a more accurate result.

For 2D cursor games, one full rotation of the *flick stick* should move the cursor from one side of the screen to the other. Unlike with 3D camera games, the edges of the screen clamp the mouse position, and will interfere with the results if we try to go through them. When calibrating 3D camera games, it's OK if we overshoot our rotation, because we can still move the stick back the way it came until we precisely land on our target, and it'll work fine. But when calibrating 2D cursor games, overshooting in either direction means that some stick input goes through JoyShockMapper, but the corresponding mouse input is ignored in-game.

So, start by manually moving the mouse to the left edge of the screen, then press your right stick forward but slightly to the right, so as to avoid accidentally going slightly to the left (and pressing against the left of the screen). Now rotate the stick clockwise until you barely touch the other side of the screen, and then release the right stick. As before, you can now ask JoyShockMapper for a good REAL\_WORLD\_CALIBRATION as follows:

```CALCULATE_REAL_WORLD_CALIBRATION```

JoyShockMapper will then give you your recommended REAL\_WORLD\_CALIBRATION. It might be something like: "Recommended REAL\_WORLD\_CALIBRATION: 5.3759".

You don't have to tell JoyShockMapper whether you're calibrating for a 2D game or a 3D game. *Flick stick* and other settings rely on a REAL\_WORLD\_CALIBRATION calculated this way for 3D games, but there's no direct translation between the way 3D games work (in angles and rotational velocity) to the way 2D games work (across a 2D plane), so calibrating 2D cursor games as described is simply convention.

With such a calibrated 2D game, you can choose your GYRO\_SENS or other settings by thinking about how much you want to turn your controller to move across the whole screen. A GYRO\_SENS of *1* would require a complete rotation of the controller to move from one side of the screen to the other, which is quite unreasonable! But a GYRO\_SENS of *8* means you only have to turn the controller one eighth of a complete rotation (45 degrees) to move from one side of the other, which is probably quite reasonable.

### 6. Modeshifts

Almost all settings described in previous sections that are assignations (i.e.: uses an equal sign '=') can be chorded like a regular button mapping. This is called a modeshift because you are reconfiguring the controller when specific buttons are enabled. The only *exceptions* are those listed here below.
```
AUTOLOAD
JSM_DIRECTORY
SIM_PRESS_WINDOW
DBL_PRESS_WINDOW
```

Here's some usage examples: in DOOM (2016), you can use the right stick when you bring up a weapon wheel even when using flick stick:

```
RIGHT_STICK_MODE = FLICK # Use flick stick
GYRO_OFF = R3 # Use gyro, disable with stick click

R = Q # Last weapon / Bring up weapon wheel

R,GYRO_ON = NONE # Disable gyro when R is down
R,RIGHT_STICK_MODE = MOUSE_AREA # Select wheel item with stick
```

Other ideas include changing the gyro sensitivity when aiming down sights, or only when fully pulling the trigger.

```
GYRO_SENS = 1 0.8 # Lower vertical sensitivity

ZL_MODE = NO_SKIP # Use full pull
ZL = RMOUSE # ADS
ZLF = NONE # No binding on full pull

ZLF,GYRO_SENS = 0.5 0.4 # Half sensitivity on full pull
```

These commands function exactly like chorded press bindings, whereas if multiple chords are active the latest has priority. Also the chord is active whenever the button is down regardless of whether a binding is active or not. It is also worth noting that a special case is handled on stick mode changes where upon returning to the normal mode by releasing the chord button, the stick input will be ignored until it returns to the center. In the DOOM example above, this prevents an undesirable flick upon releasing the chord.

To remove an existing modeshift you have to assign ```NONE``` to the chord.

```
ZLF,GYRO_SENS = NONE
```

### 7. Miscellaneous Commands
There are a few other useful commands that don't fall under the above categories:

* **RESET\_MAPPINGS** - This will reset all JoyShockMapper's settings to their default values. This way you don't have to manually unset button mappings or other settings when making a big change. It can be useful to always start your configuration files with the RESET\_MAPPINGS command. The only exceptions to this are the calibration state and AUTOLOAD.
* **RECONNECT\_CONTROLLERS** - Controllers connected after JoyShockMapper starts will be ignored until you tell it to RECONNECT\_CONTROLLERS. When this happens, all gyro calibration will reset on all controllers.
* **JOYCON\_GYRO\_MASK** (default IGNORE\_LEFT) - Most games that use gyro controls on Switch ignore the left JoyCon's gyro to avoid confusing behaviour when the JoyCons are held separately while playing. This is the default behaviour in JoyShockMapper. But you can also choose to IGNORE\_RIGHT, IGNORE\_BOTH, or USE\_BOTH.
* **\# comments** - Any line or part of a line that begins with '\#' will be ignored. Use this to organise/annotate your configuration files, or to temporarily remove commands that you may want to add later.
* **SLEEP** - Cause the program to sleep (or wait) for a given number of seconds. The given value must be greater than 0 and less than or equal to 10. Or, omit the value and it will sleep for one second. This command may help automate calibration.
* ***onstartup.txt*** - This is not a command. But if a file called '*onstartup.txt*' is found in the current working directory on startup, its contents will be loaded and executed right away. If you prefer that AutoLoad is disabled, put ```AUTOLOAD = OFF``` in here to have it disabled at startup. If you use HIDGuardian / HIDCerberus and always want to whitelist JSM and then reconnect controllers, just put ```WHITELIST_ADD``` and ```RECONNECT_CONTROLLERS``` in the startup file.

## Troubleshooting
Some third-party devices that work as controllers on Switch or PS4 may not work with JoyShockMapper. It only _officially_ supports first-party controllers. Issues may still arise with those, though. Reach out, and hopefully we can figure out where the problem is.

But first, here are some common problems that are worth checking first.

The JoyShockMapper console will tell you how many devices are connected, and will output information with most inputs (button presses or releases, tilting the stick). However, the only way to test that the gyro is working is to enable it and see if you can move the mouse. The quickest way to check if gyro input is working without loading a config is to just enter the command ```GYRO_SENS = 1``` and then move the controller. Don't forget that you might need to calibrate the gyro if the mouse is moving even when the controller isn't.

Many users of JoyShockMapper rely on tools like HIDGuardian to hide controller input from the game. If JSM isn't recognising your controller, maybe you haven't whitelisted JoyShockMapper.

In some circumstances, the JoyShockMapper console is responding to controller input and the mouse is responding to gyro movements, but the game you're playing isn't responding to it. This can happen when the you launch the game as an administrator. JoyShockMapper must also be launched with administrator rights in order to send keyboard and mouse events to the game.

Some users have found stick inputs to be unresponsive in one or more directions. This can happen if the stick isn't using enough of the range available to it. In this case, increasing STICK\_DEADZONE\_OUTER can help. In the same way, the stick might be reporting a direction as pressed even when it's not touched. This happens when STICK\_DEADZONE\_INNER is too small.

## Known and Perceived Issues
### Polling rate
New mouse and keyboard events are only sent when JoyShockMapper gets a new message from the controller. This means if your game's and display's refresh rates are higher than the controller's poll rate, sometimes the game and display will update without moving the mouse, even if you'd normally expect the mouse to move. The DualShock 4 sends 250 messages a second, which is plenty for even extremely high refresh rate displays. But JoyCons and Pro Controllers send 66.67 messages a second, which means you might encounter stuttering movements when playing (and displaying) above 66.67 frames per second. A future version of JoyShockMapper may work around this problem by repeating messages up to a desired refresh rate.

### Bluetooth connectivity
JoyCons and Pro Controllers normally only communicate by Bluetooth. Some Bluetooth adapters can't keep up with these devices, resulting in **laggy input**. This is especially common when more than one device is connected (such as when using a pair of JoyCons). There is nothing JoyShockMapper or JoyShockLibrary can do about this. JoyShockMapper experimentally supports connecting Switch controllers by USB.

Bluetooth support for the DualShock 4 is new in JoyShockMapper, and isn't working for everyone. Please let me know if you encounter any issues with it.

## Credits
I'm Julian "Jibb" Smart, and I made JoyShockMapper. As of version 1.3, JoyShockMapper has benefited from substantial community contributions. Huge thanks to the following contributors:
* Nicolas (code)
* Bryan Rumsey (icon art)
* Contributer (icon art)
* Sunny Ye (translation)
* Romeo Calota (linux and general portability)

Have a look at the CHANGELOG for a better idea of who contributed what. Nicolas, in particular, regularly contributes a lot of work. He is responsible for a lot of the cool quality-of-life and advanced mapping features.

JoyShockMapper relies a lot on [JoyShockLibrary](https://github.com/jibbsmart/JoyShockLibrary), which it uses to read controller inputs. Check out that project to see what prior work made JoyShockLibrary possible.

## Helpful Resources
* [GyroWiki](http://gyrowiki.jibbsmart.com) - All about good gyro controls for games:
  * Why gyro controls make gaming better;
  * How developers can do a better job implementing gyro controls;
  * How to use JoyShockMapper;
  * User editable collection of user configurations and tips for using JoyShockMapper with a bunch of games.

## License
JoyShockMapper is licensed under the MIT License - see [LICENSE.md](LICENSE.md).

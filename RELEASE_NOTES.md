# Release notes

Release note fot the YPCSWASYN EPICS module.

## Releases:
* __R3.3.2__: 2020-01-30 K. Kim
  * build with yamlLoader R2.1.1, cpsw/framework R4.3.2 

* __R3.3.1__: 2020-01-29 K. Kim
  * build with yamlLoader R2.1.0, cpsw/framework R4.4.1 and buildroot-2019.08

* __R3.3.0__: 2019-12-19 J. Vasquez
  * Update yamlLoader to version R2.0.0, and add support for
    its new feature to support multiple roots.

* __R3.2.1__: 2019-11-18 K. Kim
  * build against asyn R4.32-1.0.0

* __R3.2.0__: 2019-10-29 J. Vasquez
  * build against:
    - cpsw/framework -> R4.3.1
    - yamlLoader     -> R1.1.5

* __R3.1.2__: 2019-09-21 K. Kim
  * build with cpsw/framework R4.2.0 and yamlLoader R1.1.4

* __R3.1.1__: 2019-07-15 J. Vasquez
  * Some templates were missing the ":" after "$(P)", which was
    added back in R3.0.1 to be backward compatible with previous
    versions.

* __R3.1.0__: 2019-06-25 K. Kim
  * build with cpsw/framework R4.1.2 and yamlLoader R1.1.3

* __R3.0.5__: 2019-02-28 J. Vasquez
  * Bug fix: key comparison must be done with std::string::compare
    to search for exact matches.
  * Minor clean ups.

* __R3.0.4__: 2018-11-08 J. Vasquez
  * build against:
    - cpsw/framework -> R3.6.6, which implies:
      - boost -> 1.64.0
      - yaml-cpp -> yaml-cpp-0.5.3_boost-1.64.0
    - asyn -> R4.31.0-0.1.0,
    - yamlLoader -> R1.1.2

* __R3.0.3__: 2018-10-29 K. Kim
  * build against cpsw/framework R3.6.6
                    asyn R4.31.0-0.1.0
                    yamlLoader R1.1.1

* __R3.0.2__: 2018-07-18 K. Kim
  * build against asyn/R4.32-1.0.0

* __R3.0.1__: 2018-06-13 J. Vasquez
  * Add ":" between "$(P)" and "$(R)" macros on the templates use
    with dictionarie (named "Register*.templates"), in order to be
    backward compatible with previous versions.
  * Do not call exit() if sched_setscheduler or mlockall fail.
    This will allow IOC to run even if the user do not have the right
    privileges.

* __R3.0.0__: 2018-04-23 J. Vasquez
  * Added a new mode of autogeneration of PVs using SHA1 hash
    as PV names. Now YCPSWASYNConfig accepts an mode arguments
    (1: PV names using maps as before, 2: using new hashed names).
    See README.autoPVGeneration for more information.
  * The signature of YCPSWASYNConfig changed in order to have fewer
    parameters (See README.configureDriver for more information):
    - It doesn't accept anymore a YAML file and IP address.
      The yamlLoader module must be used instead.
    - The PV name max lenght was a default value of 60, and it can
      be changed using the function "YCPSWASYNSetPvMaxNameLen".
    - The default SCAN value has a default value of "Passive", and
      it can be changed using the function "YCPSWASYNSetDefaultScan".
  * A new function can be used to select the location of the mapping
    files: "YCPSWASYNSetMapFilePath". The default location is "yaml/".
  * A new function can be used to select the destination of the debug
    information files: "YCPSWASYNSetDebugFilePath". The default
    location is "/tmp/".
  * The PV name prefix can now be empty, in this case ":" is not
    addedd to the PV names.
  * Bug fixed when computing bit-size.
  * Improve debug inforamtion style.
  * Code clean up and refactored.

* __R2.0.3__: 2018-03-22 K. Kim
  * bumpup cpsw/framework R3.6.3
  * bumpup yamlLoader     R1.1.0

* __R2.0.2__: 2018-02-15 J. Vasquez
  * Improve performace of the stream handling setting its thread
    as a RT task, and erasing only the part of the buffer that was
    not updated.

* __R2.0.1__: 2018-01-22 J. Vasquez
  * Update ASYN to version R4.31-0.1.0.

* __R2.0.0__: 2017-11-14 J. Vasquez
  * Change save/load default PV to waveforms so they can
    hold longer paths respect to string PVs. With this
    change a defult configuration file can not longer be
    defined as a macro in st.cmd.
  * Add the CPSW root path for save/load configurations.
  * Add subArrays to the stream waveform PVs for easier
    visualization in GUIs.
  * Handle error when saving/loading configurations and
    print number of entries saved/loaded.
  * Verify if entries were actually saved/loaded.
  * Minor code clean up.

* __R1.1.6__: 2017-05-03 J. Vasquez
  * Update to CPSW framework R3.5.4
  * Update to yamlLoader R1.0.3
  * Minor bug fixes

* __R1.1.5__: 2017-04-27 J. Vasquez
  * Fix lookup function for stream interfaces

* __R1.1.4__: 2017-04-05 J. Vasquez
  * Added support for using the yamlLoader module

* __R1.1.3__: 2017-04-05 J. Vasquez
  * Remove Display and Restore sources

* __R1.1.2__: 2017-03-29 J. Vasquez
  * Bug fixed on stream thread handler that was causing random
    segmentation faults.
  * Fixed some memory leaks.
  * Minor improvements to code.

* __R1.1.1__: 2017-03-16 J. Vasquez
  * First stable tag for EPICS base 3.15.
  * This version includes the autogeneraton of PVs as well as the
    possibility to manually generate PVs for selected registers.

* __R1.0.0__: 2017-03-16 J. Vasquez
  * First stable release version



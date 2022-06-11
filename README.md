# clap_test_scan

## Clone

    git clone --recursive https://github.com/rncbc/clap_test_scan

## Build

    cd clap_test_scan

    cmake [-DCMAKE_BUILD_TYPE=Debug] -B build
    cmake --build build

## Usage

Run and enter line with path to plugin file as follows, eg. </path/to/plugin.clap> (leaving out the chevrons, of course):

    build/src/clap_test_scan
    </path/to/plugin.clap>
    ...
    <Ctrl-D>

## Issues

While using the example [clap-plugins](https://github.com/free-audio/clap-plugins),
by entering the default install path above (`/usr/local/lib/clap/clap-plugins.clap`)
a *non-debug* build will crash under `clap_plugin::destroy()` with `free(): invalid pointer`.

A *debug* build (eg. `cmake -DCMAKE_BUILD_TYPE=Debug`...), which have the address sanitizer
(asan) linked in, should work out fine, giving then an expected result output:

    build/src/clap_test_scan
    clap_test_scan: hello.
    </usr/local/lib/clap/clap-plugins.clap>
    clap_test_scan_file("/usr/local/lib/clap/clap-plugins.clap")
    CLAP|CLAP Synth|0:2|1:0|25:0|GUI,EXT,RT|/usr/local/lib/clap/clap-plugins.clap|0|0x6fcbe295
    CLAP|DC Offset|2:2|0:0|1:0|GUI,EXT,RT|/usr/local/lib/clap/clap-plugins.clap|1|0xdc6942da
    CLAP|Transport Info|0:0|0:0|0:0|GUI,EXT,RT|/usr/local/lib/clap/clap-plugins.clap|2|0xa47e1b3
    CLAP|Gain|2:2|0:0|1:0|GUI,EXT,RT|/usr/local/lib/clap/clap-plugins.clap|3|0x3c5c2166
    CLAP|Character Check|0:0|0:0|14:0|GUI,EXT,RT|/usr/local/lib/clap/clap-plugins.clap|4|0x5c371897
    CLAP|CLAP ADSR|0:2|1:0|5:0|GUI,EXT,RT|/usr/local/lib/clap/clap-plugins.clap|5|0xde4c3f6
    CLAP|CLAP SVF|2:2|0:0|6:0|GUI,EXT,RT|/usr/local/lib/clap/clap-plugins.clap|6|0xc7923e09
    <Ctrl-D>
    clap_test_scan: bye.

## Discussion

   [New CLAP plug-in host assessment / development...](https://github.com/free-audio/clap/discussions/92)


Cheers

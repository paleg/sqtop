# sqtop
Console applicaton to display information about currently active client connections for a Squid proxy in a convenient way.

   - [Installing](README.md#installing)
      - [Configuring](README.md#configuring)
         - [Mac OS X](README.md#mac-os-x)
         - [Debug version](README.md#debug-version)
      - [Building/Installing](README.md#buildinginstalling)
      - [Post-installation](README.md#post-installation)
      - [Uninstallation](README.md#uninstallation)
   - [Binaries](README.md#how-to-install-sqtop-from-binaries)
    - [Debian/Ubuntu](README.md#debianubuntu)
    - [FreeBSD](README.md#freebsd)
    - [OS X](README.md#os-x)
   - [Configuration](README.md#configuration)
     - [Squid server configuration](README.md#squid-server-configuration)
     - [Command line arguments](README.md#command-line-arguments)
   - [Usage](README.md#usage)
     - [Interactive mode](README.md#interactive-mode)
     - [Examples](README.md#examples)
     - [Diagnostic](README.md#diagnostic)

## Installing

### Configuring

sqtop uses GNU autotools to automatically configure software source code.

By default:
   - sqtop builds with user interface. This adds dependency on [posix threads](http://en.wikipedia.org/wiki/POSIX_Threads) and [ncurses](http://www.gnu.org/software/ncurses/). You can use **--disable-ui** to build sqtop without user interface (as well as without this dependencies).
   - sqtop builds with resolver. This adds dependency on [posix threads](http://en.wikipedia.org/wiki/POSIX_Threads). You can use **--without-resolver** to build sqtop without resolver (as well as without this dependency).
   - installation prefix is `/usr/local/bin'. You can use standard autotools variables to change this.

```
   $ ./configure
or
   $ ./configure --disable-ui --program-prefix=/opt/local
```

Note: to build with gcc-2.9x, you should export CXXFLAGS=-D_GNU_SOURCE before ./configure

#### Mac OS X

Snow Leopard 10.6.3 are know to have broken ncurses library - functional keys are not working (bug id #7812932). To use sqtop under 10.6.3 you can:
   - copy */usr/lib/libncurses.5.4.dylib* and */usr/lib/libncurses.5.dylib* from a 10.6.2 system to a 10.6.3 system. Do not forget to update dyld cache:
```
   $ sudo update_dyld_shared_cache
```
   - build sqtop agains ncurses from [macports](http://macports.org). Of couse you should have install ncurses from macports first
```
   $ export LDFLAGS=-L/opt/local/lib; ./configure
```

#### Debug version

To obtain version of sqtop with debugging symbols, do:
```
   $ export CXXFLAGS="-g -O0"; ./configure
```
Note: **make install** installs stripped version of binary, so if you want to use binary with debugging symbols use one in **src** subdir, or install it manualy with **cp(1)**.

----
### Building/Installing

As trivial as
```
   $ make
   $ sudo make install
```

It installs two files:
   - sqtop binary
   - man page

----
### Post-installation

If sqtop was built with user interface you can make following aliases for command to just print statistics once (without entering UI mode) like old good squidstat.

For [BASH](http://www.gnu.org/software/bash/):
```
 $ echo 'alias squidstat="sqtop -o"' >> ~/.bashrc
```

For [tcsh](http://www.tcsh.org/):
```
 $ echo 'alias squidstat "sqtop -o"' >> ~/.cshrc
```

----
### Uninstallation

From directory where sqtop was built:
```
   $ sudo make uninstall
```
## How-to install sqtop from binaries

### Debian/Ubuntu

[Download](https://github.com/paleg/sqtop/releases) appropriate deb package and install it with **dpkg**.

### FreeBSD

```
 # cd /usr/ports/net/sqtop/ && make install
```

### OS X

```
$ brew install sqtop
```

## Configuration

sqtop itself does not require any configuration. All configuration parameters can be passed while invoking sqtop, for details see [Usage](README.md#Usage)

### Squid server configuration

Edit your squid.conf to allow cachemgr protocol:

```
acl manager proto cache_object
# replace x.x.x.x with admin ip address
acl admin src x.x.x.x/255.255.255.255
http_access allow manager admin
http_access deny manager
```


Note: if you use any type of authentification (ntlm, basic, etc) above mentioned lines sould be written before any http_access with authentification.

### Command line arguments

```
     --host host (-h host)
             Squid proxy host. Defaults to 127.0.0.1.

     --port port (-p port)
             Squid proxy port. Defaults to 3128.

     --pass password (-P password)
             Squid proxy cachemgr_passwd.

     --hosts hostlist (-H hostlist)
             Comma-separated list of client IP addresses (CIDR notation is supported) to query the Squid proxy for. Hostnames are
             silently ignored.

     --users userlist (-u userlist)
             Comma-separated list of Squid usernames to list active connections for.

     --brief (-b)
             Display brief per-connection information, omits URLs.

     --detail (-d)
             Display detailed information (size, username and average speed) for each URL in each connection.

     --full (-f)
             Display full details (size, username, average speed, delay pool and elapsed time) for each URL in each connection.

     --zero (-z)
             Display zero values instead of silently omitting them.

     --once (-o)
             Disable interactive mode, just print statistics once to stdout.

     --refreshinterval seconds (-r seconds)
             Set the refresh-interval for interactive mode.

     -c
             Do not compact the display of multiple occurrences of the same URL in a single connection.

     -n
             Do not do hostname lookups.

     -S
             Do not strip domain part of hostname.

     --help  Display a brief help text.
```

Note: without options, sqtop tries to connect to a Squid proxy listening at 127.0.0.1 using port 3128.

## Usage

### Interactive mode

If built with support for ncurses(3), sqtop defaults to running in interactive mode, occupying the whole screen, unless the --once option was specified on the command line.

Information about the Squid server currently connected to, the version of sqtop used, as well as eventual error messages are shown at the top of the display.

The bottom of the display keeps various aggregates, including current and average speed, the total number of IPs connected and the total number of connections.

Any option given on the command line can be changed from within interactive mode by pressing the key corresponding to its respective short option character.

In addition to the options given on the command line, sqtop recognizes the following keys when in interactive mode:

```
/           Search for literal substrings in IPs, usernames or URLs.  Regular expressions are not parsed, currently.

SPACE       Stop refreshing.

UP/DOWN, PGUP/PGDN, HOME/END
              Scroll display.

ENTER       Toggle displayed level of detail for the currently selected entry.

?           Display the help screen, including current settings for options, where applicable.

C           Compact long urls to fit them on one line.

s           Toggle mode of display for speed detail between current and average, current only and average only.

o           Toggle connection sort order between size, current speed, average speed and max time.

R           Toggle hosts showing mode between host name only, host ip only, both ip and host name.

q           Quit sqtop
```

### Examples

List all currently active connections in interacive mode from 192.168.2.0/24 and 172.18.118.10 to a Squid proxy running at mysquid.example.com, port 8080 using ZePasswd as cachemgr_passwd:
```
 $ sqtop -h mysquid.example.com -port 8080 -p ZePasswd -H 192.168.2.0/24,172.18.118.10
```

### Diagnostic

The sqtop utility exits 0 on success, and >0 if an error occurs.

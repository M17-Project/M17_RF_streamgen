# M17_RF_streamgen
Generates an M17 RF stream for Codec2-encoded speech files ('c2enc' output).

**M17_RF_streamgen.c** - main program. Just a sketch right now.

**dummy.c** - can generate a pseudorandom bitstream with legitimate sync patterns. This can be used to check receivers.

# Installation

```
make
```

# Nomenclature

# Example
This example records local audio and generates a M17-compliant RF stream into
a file. Codec2 and SoX need to be installed.

```
rec -t raw -r 8000 --buffer 2048 -e signed-integer -b 16 -c 1 - | \
c2enc 3200 - - | m17enc DL1FROM DL1TO - out.raw
```

This directory includes binaries and embeddable source code from Google Protocol Buffers
http://code.google.com/p/protobuf/ 
http://code.google.com/p/protobuf-c/ 

The binaries were build from unmodified protobuf 2.3.0 and protobuf-c 0.15 sources, targeted at:
    - Mac OS X 10.6

The source (see src/) directory, is an extraction of the protobuf-c runtime library. It
has been modified as follows:
    - Use __LITTLE_ENDIAN__ to determine host endian-ness.
    - Fix unused label warnings.

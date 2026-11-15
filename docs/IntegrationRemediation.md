# Checking a CyDSP integration

Build the static archive and consumers with matching public headers. Match
`ProcessSpec` to the blocks supplied by the application, check returned `Status`
values and keep preparation outside the audio callback. See
[library integration](Architecture.md#library-integration),
[module contracts](Modules.md) and [version notes](../VERSION.md).

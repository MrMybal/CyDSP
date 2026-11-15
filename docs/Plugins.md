# Using CyDSP in audio plugins

Link `CyDSP::CyDSP` into the plugin and use its prepared processors from the
plugin's audio callback. The plugin supplies its format adapter, parameter mapping
and interface. See the [processing example](../examples/Processing.cpp),
[module contracts](Modules.md) and
[library integration](Architecture.md#library-integration).

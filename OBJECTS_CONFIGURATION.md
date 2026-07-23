# BACnet Objects Configuration

This repository currently exposes 36 BACnet objects across 5 object types.

## Object Counts

The counts are defined in `main/User_Settings.h`:

| Type | Count | Default slots |
|------|-------|---------------|
| Analog Values | 16 | `AV1`-`AV16` |
| Binary Values | 4 | `BV1`-`BV4` |
| Analog Inputs | 8 | `AI1`-`AI8` |
| Binary Inputs | 4 | `BI1`-`BI4` |
| Binary Outputs | 4 | `BO1`-`BO4` |

Total: `16 + 4 + 8 + 4 + 4 = 36`

## Configuration Sources

### Central User Configuration

`main/User_Settings.h` defines:

- object counts
- logical AI role identifiers
- logical BV role identifiers

`main/User_Settings.c` defines:

- BACnet instance arrays for every object type
- names
- descriptions
- units
- initial values
- active/inactive text for binary objects
- COV increments for analog objects

The object modules consume those arrays at runtime.

## Configurable Instance Arrays

The current code uses configurable BACnet instance arrays:

- `USER_AV_INSTANCES[USER_AV_COUNT]`
- `USER_BV_INSTANCES[USER_BV_COUNT]`
- `USER_AI_INSTANCES[USER_AI_COUNT]`
- `USER_BI_INSTANCES[USER_BI_COUNT]`
- `USER_BO_INSTANCES[USER_BO_COUNT]`

These arrays define the actual BACnet instance numbers exposed on the network.

That means:

- the first configured AI role does not have to remain BACnet instance `1`
- instance numbering can be changed without rewriting object-module logic
- logical sensor mapping stays stable even if BACnet instance numbers are reassigned

## Object Runtime Modules

The current project object implementations are in `main/bacnet/objects/`:

- `analog_value.c`
- `binary_value.c`
- `analog_input.c`
- `binary_input.c`
- `binary_output.c`

These modules:

- create project-specific BACnet objects
- apply defaults from `main/User_Settings.c`
- load saved state from NVS
- save writable properties back to NVS

## NVS Persistence

Object persistence is part of the current design.

The object modules store and restore data such as:

- object names
- object descriptions
- present values
- engineering units where supported
- COV increments for analog objects

Startup persistence policy is controlled by `main/app/app_storage.c` together with `USER_OVERRIDE_NVS_ON_FLASH` in `main/User_Settings.c`.

- `0`: preserve existing NVS data
- `1`: erase NVS on boot and restore compiled defaults

## Current AI1-AI8 Sensor Mapping

The sensor service in `main/app/sensor_service.c` uses logical roles backed by `USER_AI_INSTANCES[]`.

Default logical mapping:

| Logical slot | Default meaning |
|--------------|-----------------|
| AI1 | SEN54 temperature |
| AI2 | SEN54 relative humidity |
| AI3 | SEN54 VOC index |
| AI4 | SEN54 PM1.0 |
| AI5 | SEN54 PM2.5 |
| AI6 | SEN54 PM4.0 |
| AI7 | SEN54 PM10 |
| AI8 | DS18B20 temperature |

The sensor service also uses the first Binary Value role as a control point:

| Logical slot | Default meaning |
|--------------|-----------------|
| BV1 | SEN54 full reset command |

The important distinction is:

- logical role order is fixed by the firmware
- BACnet instance numbers are supplied by the configurable instance arrays

## Where to Change Things

### Change object counts

Edit `main/User_Settings.h`.

Counts must remain consistent with the arrays and with any logic that assumes a minimum number of objects. For example, the sensor service requires at least eight Analog Inputs and one Binary Value for the SEN54 reset command.

### Change BACnet instance numbers

Edit the `USER_*_INSTANCES[]` arrays in `main/User_Settings.c`.

### Change default names, descriptions, units, values, or COV increments

Edit the corresponding `USER_*` arrays in `main/User_Settings.c`.

### Change sensor-to-object mapping semantics

Edit `main/app/sensor_service.c` if you need to change which logical AI role receives each sensor measurement.

## Programmatic Updates

Typical runtime updates use the BACnet object APIs, for example:

```c
Analog_Value_Present_Value_Set(instance, value);
Analog_Input_Present_Value_Set(instance, value);
Binary_Value_Present_Value_Set(instance, BINARY_ACTIVE);
Binary_Input_Present_Value_Set(instance, BINARY_INACTIVE);
Binary_Output_Present_Value_Set(instance, BINARY_ACTIVE);
```

In the current application, `main/app/sensor_service.c` updates sensor-backed AIs using the configured instance numbers returned from `USER_AI_INSTANCES[]`.

## Related Files

- `main/main.c`
- `main/User_Settings.h`
- `main/User_Settings.c`
- `main/app/app_storage.c`
- `main/app/sensor_service.c`
- `main/bacnet/bacnet_app.c`
- `main/bacnet/objects/analog_value.c`
- `main/bacnet/objects/binary_value.c`
- `main/bacnet/objects/analog_input.c`
- `main/bacnet/objects/binary_input.c`
- `main/bacnet/objects/binary_output.c`


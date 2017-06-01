/*
 * CORE_task.h: A single-file library for working with keyboards, pointing 
 * devices and gamepads using RawInput and XInput.
 *
 * This software is dual-licensed to the public domain and under the following 
 * license: You are hereby granted a perpetual, irrevocable license to copy, 
 * modify, publish and distribute this file as you see fit.
 *
 */
#ifndef __CORE_INPUT_H__
#define __CORE_INPUT_H__

/* #define CORE_STATIC to make all function declarations and definitions static.     */
/* This is useful if the library needs to be included multiple times in the project. */
#ifdef  CORE_STATIC
#define CORE_API(_rt)                     static _rt
#else
#define CORE_API(_rt)                     extern _rt
#endif

/* @summary Define the value indicating an unused device handle.
 */
#ifndef CORE_INPUT_DEVICE_HANDLE_NONE
#define CORE_INPUT_DEVICE_HANDLE_NONE     INVALID_HANDLE_VALUE
#endif

/* @summary Define the maximum number of input devices of each type.
 */
#ifndef CORE_MAX_INPUT_DEVICES
#define CORE_MAX_INPUT_DEVICES            4
#endif

/* @summary Define the maximum number of keys that can be reported as down, pressed or released in a single update.
 */
#ifndef CORE_INPUT_MAX_KEYS
#define CORE_INPUT_MAX_KEYS               8
#endif

/* @summary Define the maximum number of buttons that can be reported as down, pressed or released in a single update.
 */
#ifndef CORE_INPUT_MAX_BUTTONS
#define CORE_INPUT_MAX_BUTTONS            8
#endif

/* @summary Define a bitvector used to poll all possible gamepad ports (all bits set.)
 */
#ifndef CORE_INPUT_ALL_GAMEPAD_PORTS
#define CORE_INPUT_ALL_GAMEPAD_PORTS      ~((uint32_t)(0))
#endif

/* @summary Define the value indicating that an input packet was dropped because too many devices of the specified type are attached.
 */
#ifndef CORE_INPUT_DEVICE_TOO_MANY
#define CORE_INPUT_DEVICE_TOO_MANY        ~((uint32_t)(0))
#endif

/* @summary Define the value indicating that a device was not found in the specified device list.
 */
#ifndef CORE_INPUT_DEVICE_NOT_FOUND
#define CORE_INPUT_DEVICE_NOT_FOUND       ~((uint32_t)(0))
#endif

/* @summary Retrieve the alignment of a particular type, in bytes.
 * @param _type A typename, such as int, specifying the type whose alignment is to be retrieved.
 */
#ifndef CORE_AlignOf
#define CORE_AlignOf(_type)                                                    \
    __alignof(_type)
#endif

/* @summary Align a non-zero size up to the nearest even multiple of a given power-of-two.
 * @param _quantity is the size value to align up.
 * @param _alignment is the desired power-of-two alignment.
 t wi*/
#ifndef CORE_AlignUp
#define CORE_AlignUp(_quantity, _alignment)                                    \
    (((_quantity) + ((_alignment)-1)) & ~((_alignment)-1))
#endif

/* @summary For a given address, return the address aligned for a particular type.
 * @param _address The unaligned address.
 * @param _type A typename, such as int, specifying the type whose alignment is to be retrieved.
 */
#ifndef CORE_AlignFor
#define CORE_AlignFor(_address, _type)                                         \
    ((void*)(((uint8_t*)(_address)) + ((((__alignof(_type))-1)) & ~((__alignof(_type))-1))))
#endif

/* Forward-declare types exported by the library */
struct _CORE_INPUT_EVENTS;
struct _CORE_POINTER_EVENTS;
struct _CORE_GAMEPAD_EVENTS;
struct _CORE_KEYBOARD_EVENTS; /* 1072 */
struct _CORE_INPUT_SYSTEM;
struct _CORE_INPUT_SYSTEM_INIT;

typedef struct _CORE_INPUT_SYSTEM_INIT {
    uint32_t                      MaxPointerDevices;      /* The maximum number of supported pointer devices attached to the system at any one time. */
    uint32_t                      MaxGamepadDevices;      /* The maximum number of supported gamepad devices attached to the system at any one time. */
    uint32_t                      MaxKeyboardDevices;     /* The maximum number of supported keyboard devices attached to the system at any one time. */
    uint32_t                      MaxKeyStateChanges;     /* The maximum number of key state changes that can be reported per-keyboard in any event set. */
    uint32_t                      MaxButtonStateChanges;  /* The maximum number of button state changes that can be reported per-pointer or per-gamepad in any event set. */
} CORE_INPUT_SYSTEM_INIT;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

CORE_API(size_t)
CORE_QueryInputSystemMemorySize
(
    void
);

CORE_API(int)
CORE_CreateInputSystem
(
    struct _CORE_INPUT_SYSTEM **input_system,
    CORE_INPUT_SYSTEM_INIT             *init
);

CORE_API(void)
CORE_ResetInputSystem
(
    struct _CORE_INPUT_SYSTEM *input_system
);

CORE_API(void)
CORE_PushRawInputPacket
(
    struct _CORE_INPUT_SYSTEM *input_system, 
    RAWINPUT                  *input_packet
);

CORE_API(void)
CORE_PushRawInputDeviceChange
(
    struct _CORE_INPUT_SYSTEM *input_system, 
    WPARAM                           wparam, 
    LPARAM                           lparam
);

CORE_API(void)
CORE_SimulateKeyPress
(
    struct _CORE_INPUT_SYSTEM *input_system, 
    HANDLE                           device, 
    UINT                             vkcode
);

CORE_API(void)
CORE_SimulateKeyRelease
(
    struct _CORE_INPUT_SYSTEM *input_system, 
    HANDLE                           device, 
    UINT                             vkcode
);

CORE_API(void)
CORE_ConsumeInputEvents
(
    struct _CORE_INPUT_EVENTS *input_events, 
    struct _CORE_INPUT_SYSTEM *input_system,
    uint64_t                      tick_time
);

#ifdef __cplusplus
}; /* extern "C" */
#endif /* __cplusplus */

#endif /* __CORE_INPUT_H__ */

#ifdef CORE_INPUT_IMPLEMENTATION

/* @summary Define the state data associated with a single XInput gamepad device.
 */
typedef struct _CORE__GAMEPAD_STATE {
    uint32_t          LTrigger;
    uint32_t          RTrigger;
    uint32_t          Buttons;
    float             LStick[4];
    float             RStick[4];
} CORE__GAMEPAD_STATE;

/* @summary Define the state data associated with a single RawInput pointer device.
 */
typedef struct _CORE__POINTER_STATE {
    int32_t           Pointer[2];
    int32_t           Relative[3];
    uint32_t          Buttons;
    uint32_t          Flags;
} CORE__POINTER_STATE;

/* @summary Define the state data associated with a single RawInput keyboard device.
 */
typedef struct _CORE__KEYBOARD_EVENTS {
    uint32_t          KeyState[8];
} CORE__KEYBOARD_EVENTS;
/* private implementation */

#endif /* CORE_INPUT_IMPLEMENTATION */


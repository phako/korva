# korva - A media push service

Korva is a D-Bus specification that aims to simplify single-file remote media
sharing such as DLNA Push.

This source package contains a implementation of the server-side of the
specification as well as a simple command-line client.

The only protocol currently implemented is UPnP, Apple's incarnation of media
sharing might follow.

## Interfaces

### org.jensge.Korva.Controller1

The Controller1 interface has four methods:

| Return signature | Method call                                                     |
| ---------------- | --------------------------------------------------------------- |
| `aa{sv}`         | `org.jensge.Korva.Controller1.GetDevices ()`                    |
| `a{sv}`          | `org.jensge.Korva.Controller1.GetDeviceInfo (IN s uid)`         |
| `s`              | `org.jensge.Korva.Controller1.Push (IN a{sv} source, IN s uid)` |
| `b`              | `org.jensge.Korva.Controller1.Unshare (IN s tag)`               |

#### Methods

##### GetDevices

Returns a list of dictionaries containing information about the currently known devices. The mandatory properties are:

###### Return values

| Type | Key         | Description                                                                          |
| ---- | ----------- | ------------------------------------------------------------------------------------ |
| `s`  | UID         | Unique (per protocol) device identifier. For UPnP this is the UDN of the device      |
| `s`  | DisplayName | Name of the device for display in an UI. Guaranteed to be UTF-8                      |
| `s`  | IconURI     | Local URI to an icon for the device. May be provided by the device or a generic icon |
| `s`  | Protocol    | Currently fixed to "UPnP". Might be "AirPlay" or something else in the future        |
| `u`  | Type        | Whether the device is a server or a renderer. Used for Upload.                       |

##### GetDeviceInfo

Returns information about a single device. For description of the return values see GetDevices.

###### Parameters

| Type | Parameter         | Description                                                                          |
| ---- | ----------------- | ------------------------------------------------------------------------------------ |
| `s`  | uid               | An unique identifier for the device as returned by GetDevices                        |


##### Push

Push a media file to a remote device

###### Parameters

| Type     | Parameter         | Description                                                                          |
| -------- | ----------------- | ------------------------------------------------------------------------------------ |
| `a{sv}`  | source            | A dictionary with meta-data about the file to be pushed. The only mandatory key is "URI".                         |
| `s`      | uid               | An unique identifier for the device as returned by GetDevices                        |


The source parameter can contain additional information about the resource to be pushed. Possible values are:

| Type | Key         | Description                                                     | Mandatory |
| ---- | ----------- | --------------------------------------------------------------- | --------- |
| `s`  | URI         | (Local) URI of the media file to push to the remote device      | ✓        |
| `t`  | Size        | Size of the media file (in bytes)                               | ✗        |
| `s`  | Title       | A title for the media file                                      | ✗        |
| `s`  | ContentType | Content type of the file                                        | ✗        |

###### Return values

| Type | Parameter         | Description                                                                               |
| ---- | ----------------- | ----------------------------------------------------------------------------------------- |
| `s`  | tag               | An unique identifier for the operation that can be used with Unshare to stop the playback |

##### Unshare

Explicitly stop playing a previously shared file.

###### Parameters

| Type     | Parameter         | Description                                               |
| -------- | ----------------- | --------------------------------------------------------- |
| `s`      | tag               | An unique identifier for the playback as returned by Push |


#### Errors 

```
org.jensge.Korva.Error.NoSuchDevice
org.jensge.Korva.Error.Timeout
org.jensge.Korva.Error.FileNotFound
org.jensge.Korva.Error.NotCompatible
org.jensge.Korva.Error.InvalidArgs
org.jensge.Korva.Error.NoSuchTransfer
org.jensge.Korva.Error.NotAccessible
```

#### Signals

```
org.jensge.Korva.Controller1.DeviceAvailable(IN a{sv} device)
org.jensge.Korva.Controller1.DeviceUnavailable(IN s UID)
```
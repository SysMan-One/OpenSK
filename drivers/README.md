SkDriver
=========

At the core of OpenSK are host implementations, which are a way of finding a
common language between several different APIs with similar functionality.
By providing a flexible and minimalistic API interface, and a way for the user
to query which features are available for a given driver, we can foster a
minimum-abstraction and encourage writing portable, re-usable high-level code.

The OpenSK driver objects allow for very flexible build rules, though these can
initially seem daunting or complicated. For example, OpenSK supports either
static or dynamic linking. Dynamic linking allows the user to install new
drivers on their system, and all dynamically-linked OpenSK applications will be
able to pick up these new drivers and use them WITHOUT a recompile. Static
linking allows the developer to restrict the application to a list of approved
drivers, and link-in the driver code so that it doesn't change without warning.

Dynamic linking is preferred whenever possible, because it grants more
flexibility to the end-user. However, static linking can have its place if the
developer either doesn't want the user to have to install the OpenSK runtime,
OR the developer doesn't want any changes to the OpenSK runtime to occur without
first being vetted by the developer.

It should be noted, any dependent APIs may still be dynamically linked in. This
is actually recommended so that the driver code can discover whether the API is
present and properly configured on the host machine. (For example, the ALSA
driver does not statically link with ALSA. Not only for licensing reasons, but
also because when statically-compiled, we shouldn't crash if the ALSA server
is not available on the host machine the code is executing on).

Creating a Driver
-----------------

There are several rules and procedures that should be followed when creating a
new driver implementation. Let's begin with a list of requirements.

### Candidacy Requirements

1. An API is a candidate for becoming a driver implementation if and only if the
   API performs any kind of data transfer for one of the supported OpenSK stream
   types.

2. An API is a candidate for becoming a driver implementation if and only if the
   API is not itself an abstraction of other device streaming APIs along the
   same vein of OpenSK. In these cases, the APIs abstracted may be candidates
   for host implementation.

3. An API is a candidate for becoming a driver implementation if and only if the
   API has something to gain from the abstraction. (Most commonly, this takes
   the form of a user being able to consume the API without writing code which
   entirely relies on it - to encourage portable code.)

4. While not required, an API is more appropriate as a driver implementation if
   specifically it consists of talking to hardware on a user's machine. (For
   example, transmitting data to your speakers, from your midi controller, etc.)
   The reason for this is because such functionality is usually OS-specific.

5. While not required, an API is more appropriate as a driver implementation if
   the API itself is not already cross-platform along all of the supported OSes
   that OpenSK supports.

### Implementation Requirements

In order to implement a driver, certain functions must be exported, and certain
functional rules must be obeyed.

1. Do not rely on the user having access to any custom headers.

The only header the user should have to include to interact with your host is
the standard OpenSK header (opensk.h). Do not modify this header, and do not
modify (even temporarily) any data passed into your host from the user which
the host did not itself create.

Note: You may provide a header for custom structures that your driver can
handle, but the key is you should not _require_ these structures to run.

2. Every OpenSK object (<ObjectType>\_T) must have `SK_INTERNAL_OBJECT_BASE;` as
   it's _very first_ field in the structure IF it is a dispatchable object.

This is required so that the loader can store extra information about the
objects within the objects themselves, and so the user can identify the type of
object they have if it has been passed around abstractly with SkObject.

3. Any allocation that you have control over should go through the
   SkAllocationCallbacks functions (skAllocate, skClearAllocate, skReallocate,
   skFree, etc.)

This ensures that the user has as much control as possible over how the
allocations take place, as well as tracking for heap usage known to OpenSK.
This enables scenarios for devices where locality of reference is important,
or maintaining a small heap size is required.

4. Any allocation made should use the appropriate SkSystemAllocationScope based
   on the definitions of the scopes. Most commonly, \*\_DRIVER and \*\_STREAM
   will be used.

This provides the user with information for how long they should expect the
allocation to live. \*\_INSTANCE lives the longest, \*\_COMMAND lives the
shortest. \*\_COMMAND should be avoided if possible, but when necessary is
appropriate.

5. Whenever `skEnumerate*()` functions are called, discovery must occur during
   the given call. Previously discovered objects cannot be freed from the user.

What this means is that when enumerating a value, you cannot cache the previous
enumeration. Nor can you delete previously enumerated objects. Now-invalid
objects should keep state over whether or not they are invalid, and if an
invalid object is used, SK_ERROR_DEVICE_LOST should be returned.

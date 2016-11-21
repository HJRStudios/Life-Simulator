This plugin synchronizes in-editor editing, in real time. As an example, if you move, or create a cube, it will move in your friend's editors too !
This enables you to work faster, and to really collaborate when making a map, once you've tested it, you will never be able to go back to Source Control only !

Features:
- Synchronizes actor's location, rotation and scale
- Synchronizes creating or destroying actors
- Supports static mesh actors' mesh and material
- Extend to support your own types !

/!\ BSP Brush are partially supported, when you create one, it won't be synchronized. However, a big effort has been made to support already created ones, so you can change their shape on size

Credits:
 - Plugin's icon is an edit of the UdpMessaging's one, with, inside screen, the StarterContent's chair
 - EdMode's icon is from UE4's editor content, its StyleName is "DeviceDetails.Share"

Technical:
- One editor module
- Uses TCP sockets
- TCP Server provided (NojeJS)
- Based on map name (works even if project is not the same)
- Supports actor transform (location, rotation, scale)
- Supports creating and deleting actors
- Supports StaticMeshActor's mesh and material
- Supports BSP Brush and BSP properties (size, etc)
- To add new supported classes, create a folder "MapSync" and create a BP class "[YourClassName]Sync", it has to inherit from SyncSerialization class
- Only two functions to override to support your own class (Serialize and Deserialize)

Contact me at hutteau.b@gmail.com to report a bug or to find support :)

Tested on Windows only